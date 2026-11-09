#include "RapidFly.hpp"
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <src/Globals.hpp>

#include <Windows.h>
#include <atomic>
#include <thread>
#include <cmath>

namespace RapidFly {

static std::atomic<bool> running{ false };
static std::thread workerThread;
static float flyY = 0.0f;
static bool hasInit = false;

static bool ReadFlyMatrix(uint32_t localPlayer, uint32_t& outMatrix) {
    uint32_t rootBone = Mem.Read<uint32_t>(localPlayer + Offsets::Bones::Root);
    if (!rootBone) return false;

    uint32_t t1 = Mem.Read<uint32_t>(rootBone + 0x8);
    if (!t1) return false;

    uint32_t t2 = Mem.Read<uint32_t>(t1 + 0x8);
    if (!t2) return false;

    uint32_t matrix = Mem.Read<uint32_t>(t2 + 0x20);
    if (!matrix) return false;

    outMatrix = matrix;
    return true;
}

void Run() {
    while (running) {
        try {
            if (!g_Globals.Exploits.RapidFly) {
                hasInit = false;
                Sleep(1);
                continue;
            }

            uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;
            if (!localPlayer) {
                Sleep(12);
                continue;
            }

            uint32_t matrix = 0;
            if (!ReadFlyMatrix(localPlayer, matrix)) {
                Sleep(1);
                continue;
            }

            Vector3 currentPos = Mem.Read<Vector3>(matrix + 0x60);

            if (!hasInit) {
                flyY = currentPos.Y + g_Globals.Exploits.RapidFlyHeight;
                hasInit = true;
            }

            //if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            //    flyY += 0.3f;
            //}
            //else if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
            //    flyY -= 0.3f;
            //}
            //else {
            //    float targetY = currentPos.Y + g_Globals.Exploits.RapidFlyHeight;
            //    float diff = targetY - flyY;
            //    if (fabsf(diff) > 0.05f) {
            //        flyY += diff * 0.15f;
            //    }
            //    else {
            //        flyY = targetY;
            //    }
            //}

            Vector3 writePos = currentPos;
            writePos.Y = flyY;
            Mem.Write<Vector3>(matrix + 0x60, writePos);

            Sleep(1);
        }
        catch (...) {
            Sleep(10);
        }
    }

    hasInit = false;
}

void Start() {
    if (running) return;
    running = true;
    hasInit = false;
    workerThread = std::thread(Run);
    workerThread.detach();
}

void Stop() {
    running = false;
}

bool IsRunning() {
    return running;
}

} // namespace RapidFly