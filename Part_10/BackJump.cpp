#include <EspLines/Exploits/Exploit/BackJump.hpp>

#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <src/Globals.hpp>


#include <Windows.h>
#include <cstdio>
#include <cstdarg>
#include <cstdint>


namespace {
    constexpr uint32_t kMinValidPtr = 0x10000;


    constexpr uintptr_t kRotSensMin = 0xFD4;
    constexpr uintptr_t kRotSensMax = 0xFD8;
    constexpr uintptr_t kAimRotSensMin = 0xFDC;
    constexpr uintptr_t kAimRotSensMax = 0xFE0;


    constexpr float kDefRotSensMin = 15.0f;
    constexpr float kDefRotSensMax = 35.0f;
    constexpr float kDefAimRotSensMin = 10.0f;
    constexpr float kDefAimRotSensMax = 20.0f;


    constexpr uint32_t kTelaSlots[] = { 0xABFF414, 0xABFF734, 0xABFF3B8, 0xABFF6D8 };


    constexpr uint32_t kScanWindow = 0x400;
    constexpr uint32_t kRetryMs = 3000;


    bool IsValidPtr(uint32_t v) {
        return v >= kMinValidPtr && v != static_cast<uint32_t>(-1);
    }


    void Log(const char* fmt, ...) {
        FILE* f = nullptr;
        char path[MAX_PATH];
        GetTempPathA(MAX_PATH, path);
        lstrcatA(path, "WBR_BackJump.log");
        if (fopen_s(&f, path, "a") != 0 || !f) return;

        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(f, "[%02u:%02u:%02u.%03u] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

        va_list args;
        va_start(args, fmt);
        vfprintf(f, fmt, args);
        va_end(args);
        fputc('\n', f);
        fclose(f);
    }


    bool FloatEq(float a, float b) {
        if (a != a || b != b) return false;
        float d = a - b;
        if (d < 0) d = -d;
        return d <= 0.01f * (b > 1.0f ? b : 1.0f);
    }


    bool FullFingerprint(uint32_t gv, const char** why) {
        if (!IsValidPtr(gv)) { *why = "invalid ptr"; return false; }
        if (!FloatEq(Mem.Read<float>(gv + kRotSensMin), kDefRotSensMin)) { *why = "rotMin"; return false; }
        if (!FloatEq(Mem.Read<float>(gv + kRotSensMax), kDefRotSensMax)) { *why = "rotMax"; return false; }
        if (!FloatEq(Mem.Read<float>(gv + kAimRotSensMin), kDefAimRotSensMin)) { *why = "aimMin"; return false; }
        if (!FloatEq(Mem.Read<float>(gv + kAimRotSensMax), kDefAimRotSensMax)) { *why = "aimMax"; return false; }
        return true;
    }


    bool LightFingerprint(uint32_t gv) {
        return IsValidPtr(gv) &&
               FloatEq(Mem.Read<float>(gv + kRotSensMin), kDefRotSensMin) &&
               FloatEq(Mem.Read<float>(gv + kRotSensMax), kDefRotSensMax);
    }


    bool ResolveFromSlot(uint32_t slot, uint32_t* outGv, const char** why) {
        uint32_t ti = Mem.Read<uint32_t>(Offsets::Il2Cpp + slot);
        if (!IsValidPtr(ti)) { *why = "ti invalid"; return false; }


        uint32_t sf = Mem.Read<uint32_t>(ti + Offsets::StaticClass);
        if (FullFingerprint(sf, why)) {
            Log("  slot 0x%X pattern A: FOUND gv=0x%X (ti=0x%X)", slot, sf, ti);
            *outGv = sf;
            return true;
        }


        uint32_t klass = Mem.Read<uint32_t>(ti);
        if (IsValidPtr(klass)) {
            uint32_t sf2 = Mem.Read<uint32_t>(klass + Offsets::StaticClass);
            if (FullFingerprint(sf2, why)) {
                Log("  slot 0x%X pattern B: FOUND gv=0x%X (ti=0x%X klass=0x%X)", slot, sf2, ti, klass);
                *outGv = sf2;
                return true;
            }
            uint32_t first = Mem.Read<uint32_t>(sf2);
            if (FullFingerprint(first, why)) {
                Log("  slot 0x%X pattern C: FOUND gv=0x%X (ti=0x%X klass=0x%X sf=0x%X)", slot, first, ti, klass, sf2);
                *outGv = first;
                return true;
            }
        }
        return false;
    }


    uint32_t g_gameVar = 0;
    uint64_t g_lastScanAttempt = 0;


    void ScanForGameVar() {
        Log("=== scan start (Il2Cpp=0x%llX InitBase=0x%X LocalPlayer=0x%X) ===",
            (unsigned long long)Offsets::Il2Cpp, (unsigned)Offsets::InitBase,
            (unsigned)g_Globals.EspConfig.LocalPlayer);


        const char* why = "?";
        uint32_t gv = 0;


        for (uint32_t d = 0; d <= kScanWindow && gv == 0; d += 4) {
            if (ResolveFromSlot((uint32_t)(Offsets::InitBase + d), &gv, &why)) break;
            if (d != 0 && ResolveFromSlot((uint32_t)(Offsets::InitBase - d), &gv, &why)) break;
        }


        for (uint32_t slot : kTelaSlots) {
            if (gv != 0) break;
            ResolveFromSlot(slot, &gv, &why);
        }


        if (gv != 0) {
            g_gameVar = gv;
            bool accel = Mem.Read<bool>(gv + Offsets::EnableAccelerationOnFalling);
            bool swapW = Mem.Read<bool>(gv + Offsets::EnableLowFallingSwapWeapon);
            float traceAdj = Mem.Read<float>(gv + 0x674);
            Log("=== scan OK: gameVar=0x%X accel=%d swap=%d traceAdj=%.3f ===", gv, (int)accel, (int)swapW, traceAdj);
        } else {
            Log("=== scan FAILED (%s) — retrying ===", why);
        }
    }
}


void BackJump::Run() {
    static bool lastState = false;
    const bool enabled = g_Globals.Exploits.BackJump;


    if (!enabled && !lastState) return;


    if (Offsets::Il2Cpp == 0) {
        lastState = enabled;
        return;
    }


    // Ruta directa (PRIME ARENA V.18): localPlayer -> GameVariables (0xB0)
    uint32_t gameVar = 0;
    const uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;
    if (localPlayer)
        gameVar = Mem.Read<uint32_t>(localPlayer + Offsets::GameVariables);


    if (gameVar != 0 && LightFingerprint(gameVar)) {
        if (g_gameVar != gameVar) {
            bool accelD = Mem.Read<bool>(gameVar + Offsets::EnableAccelerationOnFalling);
            bool swapD = Mem.Read<bool>(gameVar + Offsets::EnableLowFallingSwapWeapon);
            Log("direct GameVariables OK (fingerprint valido): gameVar=0x%X accel=%d swap=%d",
                gameVar, (int)accelD, (int)swapD);
            g_gameVar = gameVar;
        }
    }
    else {
        if (gameVar != 0) {
            float rMin = Mem.Read<float>(gameVar + kRotSensMin);
            float rMax = Mem.Read<float>(gameVar + kRotSensMax);
            float aMin = Mem.Read<float>(gameVar + kAimRotSensMin);
            float aMax = Mem.Read<float>(gameVar + kAimRotSensMax);
            Log("direct GameVariables NO es GameVarDef (gameVar=0x%X) rotMin=%.2f rotMax=%.2f aimMin=%.2f aimMax=%.2f - buscare por scan",
                gameVar, rMin, rMax, aMin, aMax);
            gameVar = 0;
        }
        // Fallback: scan runtime (TELAPARADA slots) - solo si la ruta directa falla
        if (g_gameVar != 0 && !LightFingerprint(g_gameVar)) {
            Log("cache invalidated (gameVar=0x%X)", g_gameVar);
            g_gameVar = 0;
        }


        if (g_gameVar == 0) {
            uint64_t now = GetTickCount64();
            if (now - g_lastScanAttempt >= kRetryMs) {
                g_lastScanAttempt = now;
                ScanForGameVar();
            }
        }
        gameVar = g_gameVar;
        if (gameVar == 0)
            Log("direct GameVariables empty; scan not ready yet");
    }


    if (gameVar == 0) {
        lastState = enabled;
        return;
    }


    if (enabled) {
        bool accelOnFalling = Mem.Read<bool>(gameVar + Offsets::EnableAccelerationOnFalling);
        bool lowFallingSwapWeapon = Mem.Read<bool>(gameVar + Offsets::EnableLowFallingSwapWeapon);


        if (accelOnFalling == true || lowFallingSwapWeapon == false) {
            Mem.Write<bool>(gameVar + Offsets::EnableAccelerationOnFalling, false);
            Mem.Write<bool>(gameVar + Offsets::EnableLowFallingSwapWeapon, true);
        }
    }
    else {
        if (lastState) {
            Mem.Write<bool>(gameVar + Offsets::EnableAccelerationOnFalling, true);
            Mem.Write<bool>(gameVar + Offsets::EnableLowFallingSwapWeapon, true);
        }
    }


    lastState = enabled;
}