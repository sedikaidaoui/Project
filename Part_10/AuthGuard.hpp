#pragma once
// ============================================================================
//  AuthGuard  —  protection du panel 0xVoid (anti-crack)
//
//  Principe : il ne doit PLUS exister UN SEUL bool a patcher.
//  1) Etat de session = 3 cles liees entre elles (pas un bool)
//  2) Anti-debug    (IsDebuggerPresent / PEB / NtQueryInformationProcess / DR0-DR3)
//  3) Integrite RUNTIME  (.text + .rdata vs baseline prise au demarrage)
//  4) Integrite FICHIER  (sceau dans .data -> hash du DLL sur disque ; le sceau
//                         n'est plus pose automatiquement au build : voir
//                         SECURITY_AUDIT.md, la verification est donc inactive)
//  5) Protections   (page du code en RWX / RW = patch en cours)
//  6) Heartbeat     (voir ui.cpp : revalidation serveur + Lock() en cas d'echec)
//  7) Decoys        (AuthDecoys : 73 fausses portes d'auth)
//
//  Toute anomalie -> Fail() -> g_tampered (l'app se decharge proprement).
//  header-only : pas de .cpp ajouter au vcxproj.
// ============================================================================

#include <windows.h>
#include <TlHelp32.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include "AuthDecoys.hpp"

namespace AuthGuard
{
    // ---------------------------------------------------------------- etat ---
    inline std::atomic<uint32_t>   g_unlocked{ 0 };         // 0 = verrouille
    inline std::atomic<uint32_t>   g_keyA{ 0 }, g_keyB{ 0 }, g_keyC{ 0 };
    inline std::atomic<bool>       g_tampered{ false };
    inline std::atomic<bool>       g_inited{ false };
    inline std::atomic<const char*> g_reason{ nullptr };

    inline uint64_t g_textBase  = 0;                        // baseline .text
    inline uint64_t g_rdataBase = 0;                        // baseline .rdata
    inline std::atomic<long long> g_nextAntiDbg{ 0 };       // GetTickCount64 (ms)
    inline std::atomic<long long> g_nextText{ 0 };
    inline std::atomic<long long> g_nextSeal{ 0 };
    inline std::atomic<uint32_t>  g_decoySink{ 0 };         // garde les decoys en vie

    // ---- sceau d'integrite (VIVANT DANS .data -> le script y ecrit) --------
    //  slot[0] = magie        slot[1] = hash du fichier (hors slot)
    //  slot[2] = reserve      slot[3] = 0 au build, marque apres stamp
    inline constexpr uint64_t kSlotMagic   = 0x0F1E2D3C4B5A6978ULL;
    inline constexpr uint64_t kSlotStamped = 0xA11C0DE5C0DE5EEDULL;
    inline volatile uint64_t  g_sealSlot[4] = { kSlotMagic, 0, 0, 0 };

    // ------------------------------------------------------- introspection ---
    inline void* ModuleBase()
    {
        HMODULE h = nullptr;
        if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                (LPCSTR)&ModuleBase, &h) || !h)
            return nullptr;
        return (void*)h;
    }

    // Empreinte FNV-1a 64 sur une section de CE module (memoire = runtime).
    inline uint64_t HashSection(const char* name)
    {
        const uint8_t* base = (const uint8_t*)ModuleBase();
        if (!base) return 0;

        const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)base;
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;

        const IMAGE_NT_HEADERS* nt = (const IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;

        const IMAGE_SECTION_HEADER* sec = IMAGE_FIRST_SECTION(nt);
        for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i)
        {
            if (strncmp((const char*)sec[i].Name, name, 8) != 0)
                continue;

            const uint8_t* p = base + sec[i].VirtualAddress;
            const size_t  len = sec[i].Misc.VirtualSize;
            uint64_t h = 14695981039346656037ULL;
            for (size_t k = 0; k < len; ++k) { h ^= p[k]; h *= 1099511628211ULL; }
            return h;
        }
        return 0;
    }

    inline uint64_t HashText()  { return HashSection(".text");  }
    inline uint64_t HashRdata() { return HashSection(".rdata"); }

    // --------------------------------------------------------- sceau fichier -
    // Hash FNV-1a 64 du DLL SUR DISQUE, les 32 octets du slot pris comme zeroes.
    // (le script de scellement a ete retire : voir SECURITY_AUDIT.md)
    inline uint64_t HashSealFile()
    {
        char path[MAX_PATH] = {};
        if (!GetModuleFileNameA((HMODULE)ModuleBase(), path, MAX_PATH) || !path[0])
            return 0;

        HANDLE hFile = CreateFileA(path, GENERIC_READ,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
            return 0;

        HANDLE hMap = CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!hMap) { CloseHandle(hFile); return 0; }

        const uint8_t* p = (const uint8_t*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
        if (!p) { CloseHandle(hMap); CloseHandle(hFile); return 0; }

        ULARGE_INTEGER sz;
        sz.QuadPart = 0;
        if (!GetFileSizeEx(hFile, (PLARGE_INTEGER)&sz) || sz.QuadPart < 64 || sz.QuadPart > 0x40000000)
        {
            UnmapViewOfFile(p);
            CloseHandle(hMap);
            CloseHandle(hFile);
            return 0;
        }

        uint64_t result = 0;
        if (sz.QuadPart >= 64)
        {
            // localise [MAGIC][8][8][MARQUE]
            uint64_t off = 0;
            bool found = false;
            for (uint64_t i = 0; i + 32 <= sz.QuadPart; ++i)
            {
                if (*(const uint64_t*)(p + i) == kSlotMagic &&
                    *(const uint64_t*)(p + i + 24) == kSlotStamped)
                { off = i; found = true; break; }
            }

            if (found)
            {
                uint64_t h = 14695981039346656037ULL;
                for (uint64_t i = 0; i < sz.QuadPart; ++i)
                {
                    const uint8_t b = (i >= off && i < off + 32) ? 0 : p[i];
                    h ^= b; h *= 1099511628211ULL;
                }
                result = h;
            }
        }

        UnmapViewOfFile(p);
        CloseHandle(hMap);
        CloseHandle(hFile);
        return result;
    }

    // Le sceau n'est verifie que si le build a ete "stampe" (slot[3] = marque).
    inline void Fail(const char* why);                       // defini plus bas ("verdict")
    inline void VerifySeal()
    {
        if (g_sealSlot[3] != kSlotStamped)
            return;                                          // build non stampe -> pas d'alarme
        const uint64_t h = HashSealFile();
        if (h == 0)
            return;                                          // fichier illisible -> skip
        if (h != g_sealSlot[1])
            Fail("file seal mismatch");                      // DLL modifiee sur disque
    }

    // ------------------------------------------------------------ anti-debug -
    inline bool DebuggerPresent()
    {
        if (IsDebuggerPresent())
            return true;

        BOOL remote = FALSE;
        if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &remote) && remote)
            return true;

#ifdef _WIN64
        // PEB.BeingDebugged (x64 : GS:[0x60])
        const uint8_t* peb = (const uint8_t*)__readgsqword(0x60);
        if (peb && peb[2] != 0)
            return true;
#endif

        typedef LONG(NTAPI* NtQIP_t)(HANDLE, ULONG, PVOID, ULONG, PULONG);
        static NtQIP_t NtQIP = (NtQIP_t)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
        if (NtQIP)
        {
            // ProcessDebugPort = 7  ->  non nul si debogue
            ULONG_PTR port = 0;
            if (NtQIP(GetCurrentProcess(), 7, &port, sizeof(port), nullptr) == 0 && port != 0)
                return true;
            // ProcessDebugObjectHandle = 30
            HANDLE obj = nullptr;
            if (NtQIP(GetCurrentProcess(), 30, &obj, sizeof(obj), nullptr) == 0 && obj != nullptr)
                return true;
        }
        return false;
    }

    // Points d'arret materiels (DR0-DR3) sur les threads de CE processus.
    inline bool HardwareBreakpoints()
    {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snap == INVALID_HANDLE_VALUE) return false;

        const DWORD pid = GetCurrentProcessId();
        THREADENTRY32 te{}; te.dwSize = sizeof(te);
        bool found = false;

        if (Thread32First(snap, &te))
        {
            do {
                if (te.th32OwnerProcessID != pid) continue;
                HANDLE th = OpenThread(THREAD_GET_CONTEXT, FALSE, te.th32ThreadID);
                if (!th) continue;
                CONTEXT c{}; c.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                if (GetThreadContext(th, &c) && (c.Dr0 || c.Dr1 || c.Dr2 || c.Dr3))
                    found = true;
                CloseHandle(th);
                if (found) break;
            } while (Thread32Next(snap, &te));
        }
        CloseHandle(snap);
        return found;
    }

    // La page qui contient notre code doit rester en RX (image).
    inline bool CodeProtectionChanged()
    {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery((LPCVOID)&HashSection, &mbi, sizeof(mbi)))
            return false;

        const DWORD p = mbi.Protect & 0xFF;
        if (p == PAGE_EXECUTE_READWRITE || p == PAGE_READWRITE || p == PAGE_WRITECOPY)
            return true;                                   // quelqu'un a ouvert la page
        if (!(mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ |
                             PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
            return true;                                   // plus du tout executable
        return false;
    }

    // ------------------------------------------------------------- verdict ---
    inline void Fail(const char* why)
    {
        if (g_tampered.exchange(true))
            return;                                        // deja signale
        g_reason.store(why);
        g_unlocked.store(0);
        g_keyA.store(0); g_keyB.store(0); g_keyC.store(0);
    }

    inline void Init()
    {
        if (g_inited.exchange(true))
            return;
        g_textBase  = HashText();                          // baselines DES le demarrage
        g_rdataBase = HashRdata();
        const long long now = (long long)GetTickCount64();
        g_nextAntiDbg.store(now + 1000);
        g_nextText.store(now + 3000);
        g_nextSeal.store(now + 2000);
        VerifySeal();                                      // sceau des la mise en route
    }

    // Appelle UNIQUEMENT apres un vrai succes serveur (ui.cpp).
    inline void Unlock()
    {
        const uint64_t seed = (uint64_t)GetTickCount64() ^ (uint64_t)&Unlock;
        const uint32_t a = (uint32_t)(seed * 2654435761u) ^ 0xA5A5A5A5u;
        const uint32_t b = (uint32_t)((seed >> 17) * 2246822519u);
        g_keyA.store(a); g_keyB.store(b); g_keyC.store(a ^ b ^ 0x5F3759DFu);
        g_unlocked.store(1);
    }

    inline void Lock(const char* why)
    {
        g_reason.store(why);
        g_unlocked.store(0);
        g_keyA.store(0); g_keyB.store(0); g_keyC.store(0);
    }

    inline bool Unlocked()
    {
        if (!g_unlocked.load(std::memory_order_relaxed))
            return false;
        if (g_tampered.load(std::memory_order_acquire))
            return false;
        // predicat opaque : les 3 cles doivent rester coherentes entre elles
        const uint32_t a = g_keyA.load(std::memory_order_relaxed);
        const uint32_t b = g_keyB.load(std::memory_order_relaxed);
        const uint32_t c = g_keyC.load(std::memory_order_relaxed);
        if ((a ^ b ^ c) != 0x5F3759DFu)
            return false;
        if (a == 0)                                         // Lock() passe les 3 a 0
            return false;
        return true;
    }

    inline bool        Tampered()     { return g_tampered.load(std::memory_order_acquire); }
    inline const char* TamperReason() { const char* r = g_reason.load(); return r ? r : "unknown"; }

    // ---------------------------------------------------------------- poll ---
    // A appeler a chaque frame (boucle de rendu).
    inline void Poll()
    {
        if (!g_inited.load(std::memory_order_relaxed))
            return;
        if (g_tampered.load(std::memory_order_acquire))
            return;

        const long long now = (long long)GetTickCount64();

        if (now >= g_nextAntiDbg.load(std::memory_order_relaxed))
        {
            g_nextAntiDbg.store(now + 1500, std::memory_order_relaxed);

            if (DebuggerPresent())       { Fail("debugger detected");    return; }
            if (HardwareBreakpoints())   { Fail("hardware breakpoint");  return; }
            if (CodeProtectionChanged()) { Fail("code page modified");   return; }

            // 73 fausses portes d'auth : execution + melange dans un puit
            g_decoySink.fetch_add(AuthDecoys::RunAll("SECURE_LOGIN"), std::memory_order_relaxed);
        }

        if (now >= g_nextText.load(std::memory_order_relaxed))
        {
            g_nextText.store(now + 4000, std::memory_order_relaxed);

            const uint64_t h = HashText();
            if (h != 0 && g_textBase != 0 && h != g_textBase)
            {
                Fail("code integrity mismatch");            // .text patche en memoire
                return;
            }
            const uint64_t r = HashRdata();
            if (r != 0 && g_rdataBase != 0 && r != g_rdataBase)
            {
                Fail("data integrity mismatch");            // chaines / donnees patchees
                return;
            }
        }

        if (now >= g_nextSeal.load(std::memory_order_relaxed))
        {
            g_nextSeal.store(now + 30000, std::memory_order_relaxed);
            VerifySeal();                                   // DLL modifiee sur disque ?
        }
    }
}
