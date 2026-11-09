#pragma once
#include <thread>
#include <atomic>
#include <chrono>
#include <random>
#include <algorithm>
#include <cmath>
#include <src/Globals.hpp>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <EspLines/Player.h>
#include <Windows.h>

namespace SilentAimLegit {

    static std::atomic<bool> isRunning{ false };
    static std::atomic<bool> isEnabled{ false };
    static std::thread silentAimThread;

    static Player* lastTarget = nullptr;
    static Vector3 lastTargetPos = Vector3(0, 0, 0);
    static int bulletCount = 0;
    static int holdRemaining = 0;
    static Vector3 lastAimPos = Vector3(0, 0, 0);
    static Vector3 smoothedAimPos = Vector3(0, 0, 0);
    static std::mt19937 randomGen(std::random_device{}());

    enum class LockRegion {
        Head,
        Neck,
        Chest
    };

    static LockRegion currentLockRegion = LockRegion::Head;

    inline float RandomFloat() {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(randomGen);
    }

    inline bool OffsetsReady() {
        return Offsets::SilentLegitIsFiring != 0 &&
               Offsets::SilentLegitWeaponInfo != 0 &&
               Offsets::SilentLegitGunTip != 0 &&
               Offsets::SilentLegitBulletHit != 0;
    }

    inline Player* FindClosestEnemy() {
        if (g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0) {
            return nullptr;
        }

        Player* bestTarget = nullptr;
        float minScreenDistance = FLT_MAX;
        Vector2 center(g_Globals.EspConfig.Width / 2.0f, g_Globals.EspConfig.Height / 2.0f);

        SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
        for (auto& pair : g_Globals.EspConfig.Entities) {
            Player* e = &pair.second;

            if (!e->IsKnown || e->IsDead) continue;
            if (g_Globals.AimBot.IgnoreKnocked && e->IsKnocked) continue;
            if (g_Globals.AimBot.IgnoreBots && e->IsBot) continue;

            Vector3 aimPos = e->Head;
            if (aimPos == Vector3::Zero()) continue;

            ImVec2 aim2D = W2S::WorldToScreenImVec2(
                g_Globals.EspConfig.ViewMatrix,
                aimPos,
                g_Globals.EspConfig.Width,
                g_Globals.EspConfig.Height
            );

            if (aim2D.x <= 0 || aim2D.y <= 0) continue;
            if (aim2D.x > g_Globals.EspConfig.Width || aim2D.y > g_Globals.EspConfig.Height) continue;

            float dist3D = Vector3::Distance(g_Globals.EspConfig.MainCamera, aimPos);
            if (dist3D > g_Globals.AimBot.DistanceAim) continue;

            float dx = aim2D.x - center.X;
            float dy = aim2D.y - center.Y;
            float screenDistance = sqrtf(dx * dx + dy * dy);

            if (screenDistance > g_Globals.AimBot.Fov) continue;

            if (screenDistance < minScreenDistance) {
                minScreenDistance = screenDistance;
                bestTarget = e;
            }
        }

        return bestTarget;
    }

    inline void AimAtTarget(Player* target) {
        if (target == nullptr || !OffsetsReady()) return;

        if (target != lastTarget) {
            bulletCount = 0;
            lastTarget = target;
            lastTargetPos = target->Head;
            holdRemaining = 0;
            smoothedAimPos = Vector3(0, 0, 0);
            lastAimPos = Vector3(0, 0, 0);
        }

        uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;
        if (!localPlayer) return;

        bool isShooting = Mem.ReadS<bool>(localPlayer + Offsets::SilentLegitIsFiring);
        if (!isShooting) return;

        uint32_t weaponData = Mem.ReadS<uint32_t>(localPlayer + Offsets::SilentLegitWeaponInfo);
        if (weaponData == 0) return;

        Vector3 startPos = Mem.ReadS<Vector3>(weaponData + Offsets::SilentLegitGunTip);

        Vector3 neckPos = target->Head + Vector3(0, -0.15f, 0);
        Vector3 chestPos = target->Head + Vector3(0, -0.3f, 0);

        if (holdRemaining <= 0) {
            float headRate = (std::clamp)(g_Globals.AimBot.HeadShotRate, 0.0f, 1.0f);
            float remainingRate = 1.0f - headRate;
            float neckRate = remainingRate * 0.6f;
            float chestRate = remainingRate * 0.4f;

            float sum = headRate + neckRate + chestRate;

            if (sum <= 0.0001f) {
                headRate = 1.0f;
                neckRate = 0.0f;
                chestRate = 0.0f;
                sum = 1.0f;
            }

            float r = RandomFloat() * sum;

            if (r < headRate) {
                currentLockRegion = LockRegion::Head;
            }
            else if (r < headRate + neckRate) {
                currentLockRegion = LockRegion::Neck;
            }
            else {
                currentLockRegion = LockRegion::Chest;
            }

            holdRemaining = 3;
        }

        Vector3 aimPosLocal;
        switch (currentLockRegion) {
        case LockRegion::Head:
            aimPosLocal = target->Head;
            break;
        case LockRegion::Neck:
            aimPosLocal = neckPos;
            break;
        case LockRegion::Chest:
        default:
            aimPosLocal = chestPos;
            break;
        }

        if (aimPosLocal == Vector3(0, 0, 0) && lastTargetPos != Vector3(0, 0, 0)) {
            aimPosLocal = lastTargetPos;
        }

        float jitterBaseMin = 0.02f;
        float jitterBaseMax = 0.08f;
        float jitterRange = jitterBaseMin + RandomFloat() * (jitterBaseMax - jitterBaseMin);
        float jitter = 0.20f * jitterRange;

        aimPosLocal = aimPosLocal + Vector3(
            (RandomFloat() - 0.5f) * jitter,
            (RandomFloat() - 0.5f) * jitter,
            (RandomFloat() - 0.5f) * jitter
        );

        float targetSpeed = Vector3::Distance(target->Head, lastTargetPos);
        float offsetMultiplier = (std::clamp)(targetSpeed * 2.0f, 0.0f, 0.12f);

        if (bulletCount >= 1) {
            aimPosLocal = aimPosLocal + Vector3(
                (RandomFloat() - 0.5f) * offsetMultiplier,
                (-RandomFloat()) * offsetMultiplier / 2.0f,
                (RandomFloat() - 0.5f) * offsetMultiplier
            );
        }

        float smoothingFactor = 0.45f;
        if (smoothedAimPos == Vector3(0, 0, 0)) {
            smoothedAimPos = aimPosLocal;
        }
        else {
            float lerpFactor = 1.0f - (1.0f - smoothingFactor) * (1.0f - smoothingFactor);
            smoothedAimPos = Vector3::Lerp(smoothedAimPos, aimPosLocal, lerpFactor);
        }

        lastAimPos = aimPosLocal;

        Vector3 delta = smoothedAimPos - startPos;
        Mem.Write<Vector3>(weaponData + Offsets::SilentLegitBulletHit, delta);

        bulletCount++;
        lastTargetPos = target->Head;
        holdRemaining = (std::max)(0, holdRemaining - 1);
    }

    inline void AggressiveSilentAimLoop() {
        while (isRunning.load()) {
            try {
                if (!isEnabled.load()) {
                    std::this_thread::yield();
                    continue;
                }

                if (!g_Globals.AimBot.SilentAimEnabled || g_Globals.AimBot.SilentAimMode != 1) {
                    std::this_thread::yield();
                    continue;
                }

                if (g_Globals.AimBot.SilentAimBind != 0 &&
                    (GetAsyncKeyState(g_Globals.AimBot.SilentAimBind) & 0x8000) == 0) {
                    std::this_thread::yield();
                    continue;
                }

                if (g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0 ||
                    !g_Globals.EspConfig.Matrix || g_Globals.EspConfig.LocalPlayer == 0) {
                    std::this_thread::yield();
                    continue;
                }

                Player* target = FindClosestEnemy();

                if (target != nullptr) {
                    AimAtTarget(target);
                }
                else {
                    lastTarget = nullptr;
                    bulletCount = 0;
                    holdRemaining = 0;
                }

                std::this_thread::yield();
            }
            catch (...) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    inline void Start() {
        if (!isRunning.load()) {
            isRunning.store(true);
            isEnabled.store(true);
            bulletCount = 0;
            lastTarget = nullptr;
            holdRemaining = 0;
            silentAimThread = std::thread(AggressiveSilentAimLoop);
            silentAimThread.detach();
        }
        else {
            isEnabled.store(true);
        }
    }

    inline void Stop() {
        if (isRunning.load()) {
            isEnabled.store(false);
            lastTarget = nullptr;
            bulletCount = 0;
            holdRemaining = 0;
        }
    }

    inline void Cleanup() {
        if (isRunning.load()) {
            isRunning.store(false);
            isEnabled.store(false);
            lastTarget = nullptr;
            bulletCount = 0;
        }
    }
}
