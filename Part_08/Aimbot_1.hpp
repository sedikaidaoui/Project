#pragma once
#include <cstdint>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Player.h>
#include <src/Globals.hpp>
#include <EspLines/Memory/Memory.hpp>

namespace Aim {
    class Aimbot {
    public:
        static void LegitAimbot();
        static void RubixAimbot();
        static void MouseAimbot();
        static void AimLockAimbot();       // AimbotType == 3
        static void StartAimbot();
        static void StopAimbot();
    private:
        static std::atomic<bool> aimbotRunning;
        static std::thread aimbotThread;
        static bool lastKeyState;
        static Player* lastTarget;
        static uintptr_t g_currentTargetAddr;
        static bool EntityData(uint32_t entity, Player& player, Vector3& mainPos);
        static Vector3 GetHitBoxPosition(const Player& entity);
        static bool FindClosestEnemy(Player& outTarget);
        static bool FindRubixTarget(Player& outTarget);
        static bool FindAimLockTarget(Player& outTarget);   // closest-to-crosshair
        static void AimLockTarget(const Player& target);
        static void ResetAimLock();
    };
}