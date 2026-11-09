#pragma once

#include <atomic>
#include <cstdint>
#include <thread>
#include <EspLines/Player.h>

// Silent Aim V2 - port du "BrutalSilent" de BR MODE BY DEATH ENGINE
namespace SilentAimV2Function {
    class SilentAimV2 {
    public:
        static void Start();
        static void Stop();

    private:
        static void Run();
        static uintptr_t FindBestTargetAddress();
        static bool CopyTarget(uintptr_t address, Player& out);
        static Vector3 CalculateBrutalAim(const Player& target);
        static Vector3 CalculateBrutalLead(const Player& target);
        static void ResetTargetState();

        static std::atomic<bool> running;
        static std::thread worker;
        static uint32_t cachedWeaponId;
        static uintptr_t lastTargetAddress;
        static Vector3 lastTargetPosition;
        static Vector3 targetVelocity;
        static uint64_t lastTargetUpdate;
    };
}
