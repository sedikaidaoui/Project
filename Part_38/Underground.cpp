#include "Underground.hpp"

#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <src/Globals.hpp>
#include <Windows.h>
#include <atomic>
#include <thread>

namespace Underground {

namespace {
    std::atomic<bool> isRunning{ false };
    std::thread worker;
    bool isFrozen = false;
    Vector3 frozenPos;
    const float kPushDown = 0.7f;
}

void Work() {
    if (isRunning) return;
    isRunning = true;
    isFrozen = false;

    worker = std::thread([] {
        while (isRunning) {
            const uint32_t lp = g_Globals.EspConfig.LocalPlayer;
            const bool enabled = g_Globals.Exploits.UndergroundEnabled;

            if (!enabled || lp == 0) {
                isFrozen = false;
                Sleep(100);
                continue;
            }

            uint32_t rootBone = 0, t1 = 0, t2 = 0, mx = 0;
            Vector3 pos;

            if (!Mem.Read<uint32_t>(lp + Offsets::Bones::Root, rootBone) || rootBone == 0 ||
                !Mem.Read<uint32_t>(rootBone + 0x8, t1) || t1 == 0 ||
                !Mem.Read<uint32_t>(t1 + 0x8, t2) || t2 == 0 ||
                !Mem.Read<uint32_t>(t2 + 0x20, mx) || mx == 0 ||
                !Mem.Read<Vector3>(mx + 0x60, pos)) {
                Sleep(50);
                continue;
            }

            if (!isFrozen) {
                pos.Y -= kPushDown;
                for (int i = 0; i < 50; i++) {
                    Mem.Write<Vector3>(mx + 0x60, pos);
                    Sleep(1);
                }
                frozenPos = pos;
                isFrozen = true;
            }
            else {
                Vector3 now;
                if (Mem.Read<Vector3>(mx + 0x60, now)) {
                    if (Vector3::Distance(now, frozenPos) > 0.5f)
                        isFrozen = false;
                }
                Mem.Write<Vector3>(mx + 0x60, frozenPos);
            }

            Sleep(30);
        }
        isFrozen = false;
    });
    worker.detach();
}

void Stop() {
    isRunning = false;
    isFrozen = false;
}

} // namespace Underground