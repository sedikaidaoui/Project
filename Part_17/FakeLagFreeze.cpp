#include "FakeLagFreeze.hpp"

#include <src/Globals.hpp>
#include <Windows.h>

// WINDIVERTEXPORT sans dllimport : toute l'API WinDivert est resolue avec
// GetProcAddress (voir plus bas), donc aucune reference d'import, aucune
// exception de delay-load si WinDivert.dll est absent.
#define WINDIVERTEXPORT extern
#include "../../../windivert.h"
#undef WINDIVERTEXPORT

#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace FakeLagFreeze {

namespace {

// ---------------------------------------------------------------------------
// FREEZE — filtre identique au script Python (FILTER_I) :
//   paquets UDP entrants des ports serveur 10000-10099, IPv4, taille 50..1491
// ---------------------------------------------------------------------------
static const char* kFreezeFilter =
    "inbound and udp.SrcPort >= 10000 and udp.SrcPort <= 10099 and "
    "ip and ip.Protocol == 17 and ip.Length >= 50 and ip.Length <= 1491";

// ---------------------------------------------------------------------------
// TELEKILL LAG — filtre Editorial X - Core v3.7 RENFORCE (version Elite el-forsa) :
//   paquets UDP SORTANTS vers les ports serveur 10000-10099.
//
//   Base d'origine (script "Elite") :
//       (udp.DstPort>=10011 and udp.DstPort<=10020) and udp.PayloadLength>=45
//   -> deux fuites : les ports 10000-10010/10021-10099 passaient libres, et
//      les petits paquets (payload < 45 octets : keep-alives, acks) passaient
//      aussi -> le serveur continuait a recevoir des nouvelles et l'effet
//      "revenait". Ici : PLUS AUCUNE limite de taille, toute la plage de
//      ports du jeu est retenue.
//   (le tag /* ... */ du script d'origine est refuse par WinDivert 1.x,
//    error 87 -> il reste ici, dans le code)
// ---------------------------------------------------------------------------
static const char* kTelekillFilter =
    "outbound and udp and udp.DstPort >= 10000 and udp.DstPort <= 10099";

static const wchar_t* kServiceName = L"WinDivert";

static constexpr size_t kRecvBufSize = 0x10000;

// garde-fous memoire (les paquets les plus vieux sont ejectes en premier)
static constexpr size_t kFreezeMaxPackets   = 8192;
static constexpr size_t kTelekillMaxPackets = 65536;                // x8 : telekill wa3ra
static constexpr size_t kMaxHeldBytes       = 8u * 1024u * 1024u;   // 8 Mo (freeze)
static constexpr size_t kTelekillMaxBytes   = 16u * 1024u * 1024u;  // 16 Mo (telekill)

// --- API WinDivert resolue dynamiquement (decltype = convention d'appel sure) ---
using PFN_WinDivertOpen     = decltype(&WinDivertOpen);
using PFN_WinDivertRecv     = decltype(&WinDivertRecv);
using PFN_WinDivertSend     = decltype(&WinDivertSend);
using PFN_WinDivertClose    = decltype(&WinDivertClose);
using PFN_WinDivertShutdown = decltype(&WinDivertShutdown);
using PFN_WinDivertSetParam = decltype(&WinDivertSetParam);

struct WinDivertApi {
    PFN_WinDivertOpen     Open     = nullptr;
    PFN_WinDivertRecv     Recv     = nullptr;
    PFN_WinDivertSend     Send     = nullptr;
    PFN_WinDivertClose    Close    = nullptr;
    PFN_WinDivertShutdown Shutdown = nullptr;
    PFN_WinDivertSetParam SetParam = nullptr;
};

WinDivertApi      g_api;
HMODULE           g_dll       = nullptr;
std::wstring      g_dllDir;
std::atomic<bool> g_apiLoaded { false };
std::mutex        g_apiMutex;   // LoadApi() + WinDivertOpen() partages (2 threads)

enum class Feature { Freeze, Telekill };

bool FeatureEnabled(Feature feature) {
    return feature == Feature::Freeze ? g_Globals.Exploits.FakeLag_Freeze
                                      : g_Globals.Exploits.FakeLag_Tele;
}

// ------------------------------- chemins ----------------------------------
int s_moduleAnchor = 0;   // adresse dans notre DLL (GetModuleHandleEx)

std::wstring DirName(const std::wstring& file) {
    size_t pos = file.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return std::wstring();
    return file.substr(0, pos);
}

std::wstring ModuleDir() {                      // dossier de la DLL injectee
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(&s_moduleAnchor), &module) || !module)
        return std::wstring();
    wchar_t path[MAX_PATH] = {};
    if (!GetModuleFileNameW(module, path, MAX_PATH)) return std::wstring();
    return DirName(path);
}

std::wstring ExeDir() {                         // dossier de l'hote (HD-Player.exe)
    wchar_t path[MAX_PATH] = {};
    if (!GetModuleFileNameW(nullptr, path, MAX_PATH)) return std::wstring();
    return DirName(path);
}

std::wstring CurrentDir() {
    wchar_t path[MAX_PATH] = {};
    if (!GetCurrentDirectoryW(MAX_PATH, path)) return std::wstring();
    return DirName(path);
}

bool FileExists(const std::wstring& path) {
    if (path.empty()) return false;
    DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

const wchar_t* DriverFileName() {
    return (sizeof(void*) == 8) ? L"WinDivert64.sys" : L"WinDivert32.sys";
}

std::wstring FindDriverFile() {
    const std::wstring dirs[] = { g_dllDir, ModuleDir(), ExeDir(), CurrentDir() };
    for (const auto& dir : dirs) {
        if (dir.empty()) continue;
        std::wstring candidate = dir + L"\\" + DriverFileName();
        if (FileExists(candidate)) return candidate;
    }
    return std::wstring();
}

// WinDivert cherche son driver dans le dossier de l'application : on copie le
// .sys la s'il n'est pas deja present (best effort, echec ignore).
void PrepareDriverFile(std::string& msg) {
    std::wstring source = FindDriverFile();
    if (source.empty()) {
        msg = std::string("WinDivert: ") +
              (sizeof(void*) == 8 ? "WinDivert64.sys" : "WinDivert32.sys") +
              " not found";
        return;
    }
    std::wstring exeDir = ExeDir();
    if (exeDir.empty()) return;
    std::wstring dest = exeDir + L"\\" + DriverFileName();
    if (_wcsicmp(source.c_str(), dest.c_str()) == 0) return;
    if (!FileExists(dest)) CopyFileW(source.c_str(), dest.c_str(), TRUE);
}

// --------------------------- chargement de l'API ---------------------------
// msg : "" si tout va bien, sinon message d'erreur/avertissement.
bool LoadApi(std::string& msg) {
    if (g_apiLoaded.load()) return true;

    std::lock_guard<std::mutex> guard(g_apiMutex);
    if (g_apiLoaded.load()) return true;

    const std::wstring dirs[] = { ModuleDir(), ExeDir(), CurrentDir() };
    HMODULE module = nullptr;
    std::wstring usedPath;

    for (const auto& dir : dirs) {
        if (dir.empty()) continue;
        std::wstring dll = dir + L"\\WinDivert.dll";
        if (!FileExists(dll)) continue;
        module = LoadLibraryW(dll.c_str());
        if (module) { usedPath = dll; break; }
    }
    if (!module) {                                  // recherche standard
        module = LoadLibraryW(L"WinDivert.dll");
        if (module) usedPath = L"WinDivert.dll";
    }
    if (!module) {
        msg = "WinDivert.dll not found (put it next to the DLL/exe)";
        return false;
    }

    WinDivertApi api;
    api.Open     = reinterpret_cast<PFN_WinDivertOpen>(GetProcAddress(module, "WinDivertOpen"));
    api.Recv     = reinterpret_cast<PFN_WinDivertRecv>(GetProcAddress(module, "WinDivertRecv"));
    api.Send     = reinterpret_cast<PFN_WinDivertSend>(GetProcAddress(module, "WinDivertSend"));
    api.Close    = reinterpret_cast<PFN_WinDivertClose>(GetProcAddress(module, "WinDivertClose"));
    api.Shutdown = reinterpret_cast<PFN_WinDivertShutdown>(GetProcAddress(module, "WinDivertShutdown"));
    api.SetParam = reinterpret_cast<PFN_WinDivertSetParam>(GetProcAddress(module, "WinDivertSetParam"));

    if (!api.Open || !api.Recv || !api.Send || !api.Close) {
        FreeLibrary(module);
        msg = "WinDivert.dll: missing exports";
        return false;
    }

    g_dllDir    = DirName(usedPath);
    g_api       = api;
    g_dll       = module;
    g_apiLoaded = true;      // publication apres ecriture de g_api/g_dllDir

    PrepareDriverFile(msg);  // avertissement eventuel (le chargement reussit)
    return true;
}

// Installation/demarrage best-effort du driver (si WinDivert n'a pas pu le faire)
bool EnsureDriver(std::string& msg) {
    std::wstring sys = FindDriverFile();
    if (sys.empty()) {
        msg = std::string("WinDivert: ") +
              (sizeof(void*) == 8 ? "WinDivert64.sys" : "WinDivert32.sys") +
              " not found";
        return false;
    }
    std::string ignored;
    PrepareDriverFile(ignored);

    SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr,
                                       SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
    if (!manager) {
        msg = "WinDivert: OpenSCManager failed (" + std::to_string(GetLastError()) + ")";
        return false;
    }

    bool started = false;
    SC_HANDLE service = OpenServiceW(manager, kServiceName,
                                     SERVICE_START | SERVICE_QUERY_STATUS | SERVICE_CHANGE_CONFIG);
    if (service) {
        ChangeServiceConfigW(service, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
                             SERVICE_ERROR_IGNORE, sys.c_str(), nullptr, nullptr,
                             nullptr, nullptr, nullptr, nullptr);
        started = StartServiceW(service, 0, nullptr);
        if (!started && GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) started = true;
        if (!started) msg = "WinDivert: driver start failed (" +
                            std::to_string(GetLastError()) + ")";
        CloseServiceHandle(service);
    }
    else if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) {
        service = CreateServiceW(manager, kServiceName, kServiceName, SERVICE_ALL_ACCESS,
                                 SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
                                 SERVICE_ERROR_IGNORE, sys.c_str(), nullptr, nullptr,
                                 nullptr, nullptr, nullptr);
        if (service) {
            started = StartServiceW(service, 0, nullptr);
            if (!started && GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) started = true;
            if (!started) msg = "WinDivert: driver start failed (" +
                                std::to_string(GetLastError()) + ")";
            CloseServiceHandle(service);
        }
        else {
            msg = "WinDivert: CreateService failed (" + std::to_string(GetLastError()) + ")";
        }
    }
    else {
        msg = "WinDivert: OpenService failed (" + std::to_string(GetLastError()) + ")";
    }

    CloseServiceHandle(manager);
    return started;
}

// ------------------------------ une capture --------------------------------
struct HeldPacket {
    std::vector<std::uint8_t> data;
    WINDIVERT_ADDRESS         addr = {};
    // instant de capture : sert au plafond de ping (Ghost Lag)
    std::chrono::steady_clock::time_point t{};
};

// Politique de relachement des paquets retenus
//   freeze   : envoi immediat de tout le d'un coup
//   telekill : mode "burst" (script Editorial X - Core) — 5 premiers paquets jetes,
//              bursts de 300 paquets, 7300 us entre deux paquets,
//              1 ms entre deux bursts, depuis un thread dedie
struct ReleasePolicy {
    bool      burst      = false;
    size_t    skip       = 0;
    size_t    burstSize  = 0;
    unsigned  packetUs   = 0;
    unsigned  burstMs    = 0;
};

// Etat d'une retenue (Freeze et Telekill ont chacun la leur, un thread chacun)
struct CaptureState {
    Feature       feature;
    const char*   name;        // libelle affiche dans le menu
    const char*   filter;      // filtre WinDivert
    size_t        maxPackets;
    size_t        maxBytes;
    ReleasePolicy release;

    std::thread         worker;
    std::thread         releaser;      // envoi en burst (telekill)
    std::atomic<bool>   releaseAbort{ false };
    std::atomic<bool>   running    { false };
    std::atomic<bool>   handleOpen { false };
    std::atomic<bool>   noteIsError{ false };
    std::atomic<int>    heldCount  { 0 };
    std::atomic<int>    capReleases{ 0 };      // nb de depassements du plafond de ping evites
    std::atomic<int>    passedCount{ 0 };      // paquets ping-safe deja sortis (jamais retenus)
    std::atomic<int>    passedMaxSize{ 0 };    // plus gros paquet laisse passer
    std::atomic<int>    heldMinSize{ 0 };      // plus petit paquet retenu
    std::atomic<int>    lastError  { 0 };
    std::atomic<HANDLE> sharedHandle{ nullptr };
    std::mutex          noteMutex;
    std::string         note;

    CaptureState(Feature f, const char* n, const char* flt,
                 size_t maxP, size_t maxB, ReleasePolicy pol)
        : feature(f), name(n), filter(flt), maxPackets(maxP), maxBytes(maxB), release(pol) {}
};

CaptureState g_freeze  { Feature::Freeze,   "Freeze",   kFreezeFilter,
                         kFreezeMaxPackets,   kMaxHeldBytes, ReleasePolicy{} };

CaptureState g_telekill{ Feature::Telekill, "Ghost Lag", kTelekillFilter,
                         kTelekillMaxPackets, kTelekillMaxBytes,
                         ReleasePolicy{ true, 5, 300, 7300, 1 } };   // burst Editorial X - Core

void SetNote(CaptureState& s, const std::string& text, bool isError) {
    std::lock_guard<std::mutex> lock(s.noteMutex);
    s.note = text;
    s.noteIsError = isError;
}

std::string GetNote(CaptureState& s) {
    std::lock_guard<std::mutex> lock(s.noteMutex);
    return s.note;
}

// (defini plus bas, utilise aussi par l'envoi en burst)
static void TuneQueue(HANDLE h);

// --------------------------- relachement (burst) ---------------------------
// Script Editorial X - Core ("Elite" renomme Editorial X - Core) : quand la retenue est coupee, les
// paquets ne partent pas d'un coup mais en bursts, avec leur propre handle
// WinDivert ouvert par le thread d'envoi — pendant tout l'envoi le filtre
// reste donc actif et aucun paquet n'est perdu entre les deux handles.
//
//   - les [skip] premiers paquets sont jetes (comme i = 5 du script)
//   - bursts de [burstSize] paquets, [packetUs] us entre 2 paquets
//   - [burstMs] ms entre 2 bursts
//   - si on re-active la retenue ou qu'on coupe la DLL pendant l'envoi,
//     le reste part immediatement (releaseAbort) : rien ne se perd.
void StartRelease(CaptureState& s, std::deque<HeldPacket> packets) {
    if (!s.release.burst || packets.empty()) return;

    // un seul envoi a la fois : on coupe le precedent avant d'en lancer un
    s.releaseAbort.store(true);
    if (s.releaser.joinable()) s.releaser.join();
    s.releaseAbort.store(false);

    s.releaser = std::thread([&s, pkts = std::move(packets)]() mutable {
        const size_t    skip  = s.release.skip;
        const size_t    burst = s.release.burstSize;
        const unsigned  perPk = s.release.packetUs;
        const unsigned  perBt = s.release.burstMs;

        // handle d'envoi dedie (meme filtre) — voir le meme code serialise
        // que l'ouverture du worker : le driver peut s'installer au Open.
        HANDLE opened = nullptr;
        DWORD  openErr = 0;
        {
            std::lock_guard<std::mutex> guard(g_apiMutex);
            opened = g_api.Open(s.filter, WINDIVERT_LAYER_NETWORK, 0, 0);
            if (!opened || opened == INVALID_HANDLE_VALUE) {
                openErr = GetLastError();
                opened  = nullptr;
            }
        }

        if (!opened) {
            SetNote(s, std::string(s.name) + ": release failed (" +
                           std::to_string(openErr) + ")", true);
            return;                       // paquets jetes : sans handle impossible
        }

        // meme tampon pilote que la capture : pendant tout l'envoi en burst
        // les paquets qui arrivent restent entiers dans la file (16 s)
        TuneQueue(opened);

        auto sendOne = [&](HeldPacket& p) {
            UINT sent = 0;
            g_api.Send(opened, p.data.data(), static_cast<UINT>(p.data.size()),
                       &sent, &p.addr);
        };

        size_t i     = (skip < pkts.size()) ? skip : pkts.size();
        size_t total = 0;

        // bursts : 300 paquets / 7300 us / 1 ms (timing du script d'origine)
        while (i < pkts.size() && !s.releaseAbort.load()) {
            size_t end = i + burst;
            if (end > pkts.size()) end = pkts.size();
            for (; i < end && !s.releaseAbort.load(); ++i) {
                sendOne(pkts[i]);
                ++total;
                std::this_thread::sleep_for(std::chrono::microseconds(perPk));
            }
            if (!s.releaseAbort.load() && i < pkts.size())
                std::this_thread::sleep_for(std::chrono::milliseconds(perBt));
        }

        // envoi interrompu (reactivation / arret) : le reste part d'un coup
        for (; i < pkts.size(); ++i) {
            sendOne(pkts[i]);
            ++total;
        }

        g_api.Close(opened);
        SetNote(s, "0x Void - Core: sent " + std::to_string(total) + " packets", false);
    });
}

// Arret demandeur : coupe l'envoi en cours. Le worker est le seul a toucher
// s.releaser, donc aucun join croise (il appelle ca lui-meme a la sortie).
void StopRelease(CaptureState& s) {
    if (!s.releaser.joinable()) return;
    s.releaseAbort.store(true);
    s.releaser.join();
    s.releaseAbort.store(false);
}

// ---------------------------------------------------------------------------
// Tampon pilote au maximum — c'est LA "9owa" de la retenue :
//   * QUEUE_LENGTH : 16384 paquets si le pilote l'accepte, sinon 8192
//     (WinDivert 1.x). Au dela, le pilote jette les paquets en exces : ils
//     n'arrivent ni au serveur ni a nous. Plus le tampon est grand, plus on
//     peut courir loin (90 m et +) sans rien faire sauter.
//   * QUEUE_TIME : 16 s (defaut 2 s). Un paquet en attente est ECRASE au
//     bout de ce delai — avec 2 s, un thread qui ralentit ou l'envoi en
//     burst pendant quelques secondes faisait deja sauter des paquets.
// ---------------------------------------------------------------------------
static void TuneQueue(HANDLE h) {
    if (!g_api.SetParam) return;
    if (!g_api.SetParam(h, WINDIVERT_PARAM_QUEUE_LENGTH, 16384))
        g_api.SetParam(h, WINDIVERT_PARAM_QUEUE_LENGTH, 8192);
    g_api.SetParam(h, WINDIVERT_PARAM_QUEUE_TIME, 16000);
}

// ------------------------------- worker ------------------------------------
void RunCapture(CaptureState& s) {
    std::vector<std::uint8_t> buf(kRecvBufSize);
    std::deque<HeldPacket>    held;
    size_t                    heldBytes = 0;
    HANDLE                    handle = nullptr;

    auto lastDriverTry = std::chrono::steady_clock::time_point{};

    // Reinjection de tous les paquets retenus (ordre conserve)
    auto flush = [&](HANDLE target) {
        if (!target) { held.clear(); heldBytes = 0; s.heldCount.store(0); s.heldMinSize.store(0); return; }
        for (auto& packet : held) {
            UINT sent = 0;
            g_api.Send(target, packet.data.data(),
                       static_cast<UINT>(packet.data.size()), &sent, &packet.addr);
        }
        held.clear();
        heldBytes = 0;
        s.heldCount.store(0);
        s.heldMinSize.store(0);
    };

    auto closeHandle = [&](HANDLE& target) {
        if (target) {
            g_api.Close(target);
            target = nullptr;
        }
        s.sharedHandle.store(nullptr);
        s.handleOpen = false;
        s.capReleases.store(0);        // compteurs remis a zero a la desactivation
        s.passedCount.store(0);
        s.passedMaxSize.store(0);
        s.heldMinSize.store(0);
    };

    // Relachement : burst (telekill, script Editorial X - Core) ou immediat (freeze)
    auto release = [&](HANDLE target) {
        if (s.release.burst && !held.empty()) {
            std::deque<HeldPacket> packets = std::move(held);
            held.clear();                 // jamais de doublon meme si le move
            heldBytes = 0;                // garde des elements cote source
            s.heldCount.store(0);
            s.heldMinSize.store(0);
            StartRelease(s, std::move(packets));
        }
        else {
            flush(target);
        }
    };

    while (s.running.load(std::memory_order_relaxed)) {

        // ---------------- RETENUE OFF : liberer puis fermer ---------------
        if (!FeatureEnabled(s.feature)) {
            if (handle) {
                release(handle);
                closeHandle(handle);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        // ---------------- RETENUE ON : charger l'API ----------------------
        if (!g_apiLoaded.load()) {
            std::string msg;
            if (!LoadApi(msg)) {
                if (!msg.empty()) SetNote(s, msg, true);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
            if (!msg.empty()) SetNote(s, msg, true);   // avertissement driver
        }

        if (!handle) {
            if (!s.running.load(std::memory_order_relaxed)) break;

            // re-active : on coupe l'eventuel envoi en burst en cours
            StopRelease(s);

            // ouverture serialisee : WinDivert peut installer/ demarrer le
            // driver au premier Open, les deux threads ne doivent pas le
            // faire en meme temps.
            HANDLE  opened   = nullptr;
            DWORD   openErr  = 0;
            {
                std::lock_guard<std::mutex> guard(g_apiMutex);
                opened = g_api.Open(s.filter, WINDIVERT_LAYER_NETWORK, 0, 0);
                if (!opened || opened == INVALID_HANDLE_VALUE) {
                    openErr  = GetLastError();
                    opened   = nullptr;
                }
            }
            handle = opened;

            if (!handle) {
                s.lastError  = (int)openErr;
                s.handleOpen = false;

                // premiere erreur -> tente d'installer/demarrer le driver
                auto now = std::chrono::steady_clock::now();
                if (now - lastDriverTry > std::chrono::seconds(5)) {
                    lastDriverTry = now;
                    std::string msg;
                    if (!EnsureDriver(msg) && !msg.empty()) SetNote(s, msg, true);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }

            s.lastError = 0;
            s.handleOpen = true;
            s.sharedHandle.store(handle);
            // tampon pilote au maximum (cf. TuneQueue)
            TuneQueue(handle);
            SetNote(s, "", false);
        }

        if (!s.running.load(std::memory_order_relaxed)) break;

        // ---------------------- reception (bloquante) ---------------------
        UINT recvLen = 0;
        WINDIVERT_ADDRESS addr = {};
        BOOL ok = g_api.Recv(handle, buf.data(), static_cast<UINT>(buf.size()), &recvLen, &addr);

        if (!ok || recvLen == 0) {
            DWORD error = GetLastError();
            if (error != ERROR_OPERATION_ABORTED && error != ERROR_NO_DATA)
                s.lastError = (int)error;
            flush(handle);          // derniere reinjection avant fermeture
            closeHandle(handle);
            if (s.running.load(std::memory_order_relaxed))
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // la retenue a ete coupee pendant la reception : on libere tout
        // (meme chemin que la boucle ci-dessus : burst si telekill)
        if (!FeatureEnabled(s.feature)) {
            HeldPacket packet;
            packet.data.assign(buf.begin(), buf.begin() + recvLen);
            packet.addr = addr;
            heldBytes += packet.data.size();
            held.push_back(std::move(packet));
            release(handle);
            closeHandle(handle);
            continue;
        }

        // ---------------- PING SAFE : les keep-alives ne sont jamais retenus -
        // Le chiffre de ping affiche par le jeu vient des petits paquets
        // periodiques (keep-alive / mesures de RTT). On les envoie TOUT DE
        // SUITE via le meme handle et on ne retient que les gros paquets
        // (mouvement) : l'effet ghost continue, le ping affiche reste normal.
        const int passBytes = g_Globals.Exploits.GhostPingPassBytes;
        if (s.feature == Feature::Telekill && passBytes > 0 &&
            static_cast<int>(recvLen) <= passBytes)
        {
            UINT sent = 0;
            g_api.Send(handle, buf.data(), recvLen, &sent, &addr);
            s.passedCount.fetch_add(1);
            if (static_cast<int>(recvLen) > s.passedMaxSize.load())
                s.passedMaxSize.store(static_cast<int>(recvLen));
            continue;                       // jamais mis dans held
        }

        // -------------------------- retention -----------------------------
        // garde-fous : nb de paquets ET volume (les plus vieux partent d'abord,
        // les paquets recents sont conserves -> le serveur reçoit la position
        // finale au relachement)
        //
        // PLAFOND DE PING (Ghost Lag / Feature::Telekill) :
        // tant que les paquets dorment, le RTT que le jeu affiche monte
        // d'autant. On relache des que la retenue atteint GhostPingCapMs ->
        // l'ecran ne depasse jamais cette valeur (750 ms par defaut). Le
        // handle reste ouvert : la retenue repart aussitot avec un nouveau
        // compte a rebours, donc l'effet ghost continue sans coupure.
        const int pingCapMs = g_Globals.Exploits.GhostPingCapMs;
        if (s.feature == Feature::Telekill && pingCapMs > 0 && !held.empty()) {
            const long long ageMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::steady_clock::now() - held.front().t).count();
            if (ageMs >= pingCapMs) {
                release(handle);        // meme chemin qu'un toggle-off (burst)
                s.capReleases.fetch_add(1);
                SetNote(s, std::string(s.name) + ": ping cap " +
                           std::to_string(pingCapMs) + " ms - released", false);
            }
        }

        while (!held.empty() &&
               (held.size() >= s.maxPackets ||
                heldBytes + recvLen > s.maxBytes)) {
            heldBytes -= held.front().data.size();
            held.pop_front();
        }

        HeldPacket packet;
        packet.data.assign(buf.begin(), buf.begin() + recvLen);
        packet.addr = addr;
        packet.t    = std::chrono::steady_clock::now();
        heldBytes += packet.data.size();
        held.push_back(std::move(packet));
        s.heldCount.store(static_cast<int>(held.size()));

        // taille minimale retenue — affichee dans le menu pour caler le
        // seuil "ping safe" entre les deux families de paquets
        const int szNow = static_cast<int>(recvLen);
        if (s.heldMinSize.load() == 0 || szNow < s.heldMinSize.load())
            s.heldMinSize.store(szNow);
    }

    // ------------------------------- arret --------------------------------
    flush(handle);              // tout part immediatement a la fermeture
    closeHandle(handle);
    StopRelease(s);             // attend le dernier envoi en burst s'il y en a
}

// --------------------------- etat affiche dans le menu ---------------------
std::string BuildStatus(CaptureState& s, bool enabled) {
    if (!s.running.load()) return "";

    if (!g_apiLoaded.load()) {
        if (!enabled) return "";                   // pas encore utilise
        std::string note = GetNote(s);
        return note.empty() ? "WinDivert: loading..." : note;
    }

    if (s.handleOpen.load()) {
        int held = s.heldCount.load();

        // Ping Safe : preuve en direct que les keep-alives sortent bien, et les
        // deux tailles observees pour caler le seuil (entre "max out" et
        // "held min" — c'est la que se trouvent les paquets de mouvement)
        std::string cap;
        if (s.feature == Feature::Telekill) {
            const int passB = g_Globals.Exploits.GhostPingPassBytes;
            if (passB > 0) {
                const int passed = s.passedCount.load();
                const int pmax   = s.passedMaxSize.load();
                const int hmin   = s.heldMinSize.load();
                cap = " | ping-safe <= " + std::to_string(passB) + " B";
                if (passed > 0) {
                    cap += " : " + std::to_string(passed) + " out (max " +
                           std::to_string(pmax) + " B)";
                    if (hmin > 0)
                        cap += ", held min " + std::to_string(hmin) + " B";
                }
            }
            else if (g_Globals.Exploits.GhostPingCapMs > 0) {
                cap = " | cap " + std::to_string(g_Globals.Exploits.GhostPingCapMs) + " ms";
                const int releases = s.capReleases.load();
                if (releases > 0) cap += " x" + std::to_string(releases);
            }
        }

        if (held > 0)
            return std::string(s.name) + " ACTIVE - " + std::to_string(held) +
                   " packets held" + cap;
        return std::string(s.name) + " ACTIVE - capture..." + cap;
    }

    int error = s.lastError.load();
    if (error != 0)
        return "WinDivert error " + std::to_string(error) +
               (error == 5 ? " (run as admin)" : "");

    if (enabled)
        return std::string(s.name) + ": opening WinDivert...";

    std::string note = GetNote(s);
    if (!note.empty()) return note;
    return "";
}

} // namespace

// ---------------------------------------------------------------------------
void Initialize() {
    for (CaptureState* s : { &g_freeze, &g_telekill }) {
        if (s->worker.joinable()) {
            if (s->running.load()) continue;      // deja actif
            s->worker.join();                     // ancien thread termine
        }
        s->running = true;
        s->worker  = std::thread([s] { RunCapture(*s); });
    }
}

void Shutdown() {
    for (CaptureState* s : { &g_freeze, &g_telekill }) {
        if (!s->running.exchange(false)) continue;

        if (s->worker.joinable()) {
            // Reveille un eventuel Recv bloquant : WinDivertShutdown(RECV) fait
            // echouer la reception, le worker libere les paquets puis ferme le
            // handle. WinDivert 1.x n'a pas cette fonction -> CancelIoEx.
            const bool featureOn = FeatureEnabled(s->feature);
            bool seenHandle = false;

            for (int i = 0; i < 200; ++i) {                 // max ~2 s
                HANDLE handle = s->sharedHandle.load();
                if (handle) {
                    seenHandle = true;
                    if (g_api.Shutdown)
                        g_api.Shutdown(handle, WINDIVERT_SHUTDOWN_RECV);
                    else
                        CancelIoEx(handle, nullptr);
                }
                else if (i > 5 && (seenHandle || !featureOn)) {
                    // plus de handle ouvert depuis 60 ms -> le worker n'est pas bloque
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            s->worker.join();
        }

        s->handleOpen = false;
        s->heldCount  = 0;
        SetNote(*s, "", false);
    }
}

std::string StatusText() {
    return BuildStatus(g_freeze, g_Globals.Exploits.FakeLag_Freeze);
}

std::string TelekillStatusText() {
    return BuildStatus(g_telekill, g_Globals.Exploits.FakeLag_Tele);
}

} // namespace FakeLagFreeze
