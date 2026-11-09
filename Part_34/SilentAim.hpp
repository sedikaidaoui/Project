#pragma once
#include <cstdint>
#include <atomic>
#include <thread>
#include <EspLines/Player.h>
#include <src/Globals.hpp>

namespace Aim {
    class SilentAim {
    public:
        static void Start();
        static void Stop();
        static void Work();
        static void SilentModeWork();      // NEW: Silent Mode body thread
        static void AimLockWork();          // NEW: AIMLOCK thread
        static void ResetAimLockState();   // NEW: Reset AIMLOCK state

    private:
        static bool IsValidTarget(const Player& ent);
        static bool FindTarget(Player& outTarget);
        static Vector3 GetBonePosition(const Player& target);
        static void PushFrameUpdate(const Player& target);
        static void SilentModePushFrame(const Player& target);  // NEW
        static void AimLockPushFrame(const Player& target);     // NEW

        static bool GetTargetClosestCrosshair(Player& outTarget);
        static bool GetTarget360(Player& outTarget);
        static bool GetTargetLowestHealth(Player& outTarget);
        static bool GetTargetClosestPlayer(Player& outTarget);

        static std::atomic<bool> running;
        static std::thread workerThread;
        static std::thread silentModeThread;    // NEW
        static std::thread aimLockThread;       // NEW
        static std::atomic<bool> silentModeRunning; // NEW
        static std::atomic<bool> aimLockRunning;    // NEW
    };
}
