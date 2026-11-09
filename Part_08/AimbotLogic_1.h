#pragma once
#include <cstdint>
#include <chrono>
#include <random>
#include <shared_mutex>
#include <unordered_map>
#include <src/Globals.hpp>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Player.h>
#include <thread>
#include <atomic>
#include <cmath>

namespace AimLogic {

    inline const Player* FindBestTarget(
        const std::unordered_map<long, Player>& entities,
        const Matrix4x4& viewMatrix,
        int screenW, int screenH)
    {
        if (screenW <= 0 || screenH <= 0) return nullptr;

        ImVec2        center(screenW * 0.5f, screenH * 0.5f);
        float         closestDist = FLT_MAX;
        const Player* best = nullptr;

        SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
        for (const auto& [addr, ent] : entities) {
            if (ent.IsDead || ent.IsKnocked || ent.IsTeam == Player::Bool3::True) continue;
            if (!ent.IsVisible) continue;

            Vector3 targetPoint = ent.Head;
            if (targetPoint == Vector3::Zero()) continue;

            float playerDist = Vector3::Distance(g_Globals.EspConfig.MainCamera, targetPoint);
            if (playerDist > g_Globals.AimBot.DistanceAim) continue;

            ImVec2 screenPoint = W2S::WorldToScreenImVec2(viewMatrix, targetPoint, screenW, screenH);
            if (screenPoint.x < 1.f || screenPoint.y < 1.f) continue;

            float dx = screenPoint.x - center.x;
            float dy = screenPoint.y - center.y;
            float crosshairDist = std::sqrt(dx * dx + dy * dy);

            if (crosshairDist > (float)g_Globals.AimBot.Fov) continue;

            if (crosshairDist < closestDist) {
                closestDist = crosshairDist;
                best = &ent;
            }
        }
        return best;
    }

    static uintptr_t lastLockedTargetAddr = 0;

    inline void ResetLockedTarget() {
        if (lastLockedTargetAddr != 0) {
            uintptr_t lockedAimAddr = lastLockedTargetAddr + Offsets::ColliderINICDNFOFJB;
            Mem.Write<uint32_t>(lockedAimAddr, 0);
            lastLockedTargetAddr = 0;
        }
    }

    inline void AimAtTarget(const Player* target) {
        if (!target || target->Address == 0) return;

        if (lastLockedTargetAddr != 0 && lastLockedTargetAddr != target->Address) {
            ResetLockedTarget();
        }

        uint32_t headCollider = 0;
        if (!Mem.ReadFast2(target->Address + Offsets::ColliderHECFNHJKOMN, &headCollider) || headCollider == 0)
            return;

        uintptr_t lockedAimAddr = target->Address + Offsets::ColliderINICDNFOFJB;
        Mem.Write<uint32_t>(lockedAimAddr, headCollider);
        lastLockedTargetAddr = target->Address;
    }

    static bool wasKeyPressed = false;
    static std::chrono::steady_clock::time_point keyPressStart;
    static int randomizedDelay = 0;

    inline void Tick() {
        if (!g_Globals.AimBot.Enabled) {
            ResetLockedTarget();
            return;
        }
        if (g_Globals.AimBot.AimbotType != 0) {
            ResetLockedTarget();
            return;
        }
        if (!g_Globals.EspConfig.Matrix) {
            ResetLockedTarget();
            return;
        }

        bool isKeyPressed = (GetAsyncKeyState(g_Globals.AimBot.AimbotBind) & 0x8000) != 0;

        if (isKeyPressed && !wasKeyPressed) {
            keyPressStart = std::chrono::steady_clock::now();
            wasKeyPressed = true;
            static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
            std::uniform_int_distribution<int> dist(0, 100);
            randomizedDelay = g_Globals.AimBot.UpdateInterval + dist(rng);
            return;
        }
        else if (!isKeyPressed) {
            if (wasKeyPressed) {
                ResetLockedTarget();
                wasKeyPressed = false;
            }
            return;
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - keyPressStart).count();
        if (elapsed < randomizedDelay) return;

        if (elapsed > 1200) {
            ResetLockedTarget();
            return;
        }

        static auto lastWriteTime = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastWriteTime).count() < 16) {
            return;
        }
        lastWriteTime = now;

        const Player* target = FindBestTarget(
            g_Globals.EspConfig.Entities,
            g_Globals.EspConfig.ViewMatrix,
            g_Globals.EspConfig.Width,
            g_Globals.EspConfig.Height
        );

        if (target) {
            AimAtTarget(target);
        } else {
            ResetLockedTarget();
        }
    }
}

namespace AimbotLegit {
    inline void Initialize() {}
    inline void Process() { AimLogic::Tick(); }
}

// NEW: Silent Mode + AIMLOCK integration
namespace SilentAimIntegration {
    static std::atomic<bool> aimLogicRunning{ false };
    static std::thread aimLogicThread;
    static std::atomic<bool> silentModeRunning{ false };
    static std::thread silentModeThread;
    static std::atomic<bool> aimLockRunning{ false };
    static std::thread aimLockThread;

    inline void AimLogicWork() {
        while (aimLogicRunning) {
            try {
                if (!g_Globals.AimBot.Enabled || g_Globals.AimBot.AimbotType == 3) {
                    AimLogic::ResetLockedTarget();
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                AimLogic::Tick();
                std::this_thread::yield();
            }
            catch (...) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    inline void SilentModeWork() {
        while (silentModeRunning) {
            try {
                if (!g_Globals.AimBot.SilentMode || !g_Globals.AimBot.SilentAimEnabled) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                if (g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0 || !g_Globals.EspConfig.Matrix) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                // Silent mode is handled by SilentAim::SilentModeWork() and SilentAim::AimLockWork()
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
            catch (...) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    inline void AimLockWork() {
        while (aimLockRunning) {
            try {
                if (!g_Globals.AimBot.AimLockToggle) {
                    AimLogic::ResetLockedTarget();
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                if (g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0 || !g_Globals.EspConfig.Matrix) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                // AIMLOCK is handled by SilentAim::AimLockWork() and Aimbot::AimLockTarget()
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
            catch (...) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    inline void StartAimLogic() {
        if (aimLogicRunning) return;
        aimLogicRunning = true;
        aimLogicThread = std::thread(AimLogicWork);
        aimLogicThread.detach();

        silentModeRunning = true;
        silentModeThread = std::thread(SilentModeWork);
        silentModeThread.detach();

        aimLockRunning = true;
        aimLockThread = std::thread(AimLockWork);
        aimLockThread.detach();
    }

    inline void StopAimLogic() {
        aimLogicRunning = false;
        silentModeRunning = false;
        aimLockRunning = false;
        AimLogic::ResetLockedTarget();
        if (aimLogicThread.joinable()) aimLogicThread.join();
        if (silentModeThread.joinable()) silentModeThread.join();
        if (aimLockThread.joinable()) aimLockThread.join();
    }
}
