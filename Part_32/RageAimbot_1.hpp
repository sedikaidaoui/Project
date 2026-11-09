#pragma once
#include <cstdint>
#include <chrono>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Player.h>
#include <src/Globals.hpp>
#include <EspLines/Memory/Memory.hpp>

// ============================================================================
//  Aim RAGE - EliteXzp AimbotPullThreadFunc port
//
//  - worker thread (~1ms): bone pull -> LookRotation -> Slerp (RageTrackSpeed)
//    -> write AimRotation + Prev/Backup + m_SwapWeaponTime + network
//    ShadowState StartRot/TargetRot (PlayerNetwork 0x1A60 + 0x34/0x44)
//  - visible check: aim-assist lists on the local player (0x420 / 0x43C),
//    skipped when "Track Through Walls" (RageWallTrack) is ON
//  - Process() runs once per frame (Data::Work, render thread): feeds settings,
//    rescans the entity snapshot and publishes the current pull target
//  - StartAimbot() starts the worker exactly once, StopAimbot() signals it
// ============================================================================
namespace Aim {
    class RageAimbot {
    public:
        static Player RageTarget;
        static bool HasRageTarget;
        static bool RageAimActive;
        static bool RotationApplied;
        static std::chrono::steady_clock::time_point FireKeyPressTime;

        static bool IsKeyActive();
        static Vector3 GetHitbox(const Player& entity);
        static Vector3 GetBestMultipointTargetPoint(const Player& entity);
        static bool FindBestTarget(Player& outTarget);

        // RAGE SCAN (per-entity selection, same maths as the reference source)
        static void BeginScan();
        static void ScanEntity(uint32_t Entity, const Vector3& PosHeadInScr, bool isDying);
        static void EndScan();
        static void FrameScan();   // scan independent from the ESP render path

        static void Process();
        static void StartAimbot();
        static void StopAimbot();
    };
}
