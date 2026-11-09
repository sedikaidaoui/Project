#include <EspLines/Aimbot/SilentMem.hpp>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/Quaternion.hpp>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <EspLines/Player.h>
#include <src/Globals.hpp>
#include <Windows.h>
#include <cmath>
#include <cfloat>
#include <thread>
#include <chrono>
#include <atomic>

namespace Aim {

namespace {
    std::atomic<bool> s_running{ false };
    std::thread s_worker;

    bool IsKeyActive()
    {
        const int key = g_Globals.Exploits.SilentMemKey;
        if (key == 0) return false;
        return (GetAsyncKeyState(key) & 0x8000) != 0;
    }

    bool IsValidTarget(const Player& entity)
    {
        if (!entity.IsKnown || entity.IsDead) return false;
        if (g_Globals.Exploits.SilentMemIgnoreKnocked && entity.IsKnocked) return false;
        if (entity.Address == g_Globals.EspConfig.LocalPlayer) return false;
        if (entity.IsTeam == Player::Bool3::True) return false;
        return true;
    }

    bool ProjectHead(const Player& entity, ImVec2& out2D)
    {
        const Vector3 targetPosition = entity.Head;
        if (targetPosition == Vector3::Zero()) return false;

        out2D = W2S::WorldToScreenImVec2(
            g_Globals.EspConfig.ViewMatrix, targetPosition,
            g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
        if (out2D.x < 1.0f || out2D.y < 1.0f) return false;
        return true;
    }

    float PlayerDistance(const Player& entity)
    {
        return Vector3::Distance(g_Globals.EspConfig.MainCamera, entity.Head);
    }

    float CrosshairDistance(const ImVec2& pos2D)
    {
        const Vector2 screenCenter(g_Globals.EspConfig.Width / 2.0f,
                                   g_Globals.EspConfig.Height / 2.0f);
        const float dx = pos2D.x - screenCenter.X;
        const float dy = pos2D.y - screenCenter.Y;
        return std::sqrt(dx * dx + dy * dy);
    }
}

void SilentMem::PushFrame(const Player& target)
{
    const uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;
    if (localPlayer == 0) return;

    const Quaternion playerLook = Quaternion::GetRotationToLocation(
        target.Head, 0.1f, g_Globals.EspConfig.MainCamera);

    if (g_Globals.Exploits.SilentMemAimStyle == 0) {
        // SilentAim1: write bullet hit delta (sAim1/sAim2/sAim3/sAim4)
        bool isFiring = false;
        if (Mem.Read<bool>(localPlayer + Offsets::SilentLegitIsFiring, isFiring) && isFiring) {
            uint32_t weaponData = 0;
            if (Mem.Read<uint32_t>(localPlayer + Offsets::SilentLegitWeaponInfo, weaponData) && weaponData != 0) {
                Vector3 gunTip{};
                if (Mem.Read<Vector3>(weaponData + Offsets::SilentLegitGunTip, gunTip)) {
                    Mem.Write<Vector3>(weaponData + Offsets::SilentLegitBulletHit, target.Head - gunTip);
                }
            }
        }
    }
    else {
        // AimBotRage: write local aim rotation
        Mem.Write<Quaternion>(localPlayer + Offsets::AimRotation, playerLook);
    }
}

bool SilentMem::GetTargetByClosestToCrosshair(Player& outTarget)
{
    Player* best = nullptr;
    float bestCrosshair = FLT_MAX;
    const Vector2 screenCenter(g_Globals.EspConfig.Width / 2.0f,
                               g_Globals.EspConfig.Height / 2.0f);

    SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
    for (auto& pair : g_Globals.EspConfig.Entities) {
        Player& entity = pair.second;
        if (!IsValidTarget(entity)) continue;

        ImVec2 target2D;
        if (!ProjectHead(entity, target2D)) continue;

        const float playerDistance = PlayerDistance(entity);
        if (playerDistance > g_Globals.Exploits.SilentMemMaxDistance) continue;

        const float dx = target2D.x - screenCenter.X;
        const float dy = target2D.y - screenCenter.Y;
        const float crosshairDist = std::sqrt(dx * dx + dy * dy);
        if (crosshairDist >= bestCrosshair || crosshairDist > g_Globals.Exploits.SilentMemFov) continue;

        bestCrosshair = crosshairDist;
        best = &entity;
    }

    if (!best) return false;
    outTarget = *best;
    return true;
}

bool SilentMem::GetTargetByLowestHealth(Player& outTarget)
{
    Player* best = nullptr;
    float lowestHealth = FLT_MAX;

    SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
    for (auto& pair : g_Globals.EspConfig.Entities) {
        Player& entity = pair.second;
        if (!IsValidTarget(entity)) continue;

        ImVec2 target2D;
        if (!ProjectHead(entity, target2D)) continue;

        const float playerDistance = PlayerDistance(entity);
        if (playerDistance > g_Globals.Exploits.SilentMemMaxDistance) continue;

        if (CrosshairDistance(target2D) > g_Globals.Exploits.SilentMemFov) continue;

        if (static_cast<float>(entity.Health) < lowestHealth) {
            lowestHealth = static_cast<float>(entity.Health);
            best = &entity;
        }
    }

    if (!best) return false;
    outTarget = *best;
    return true;
}

bool SilentMem::GetTargetByClosestToPlayer(Player& outTarget)
{
    Player* best = nullptr;
    float closestDistance = FLT_MAX;

    SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
    for (auto& pair : g_Globals.EspConfig.Entities) {
        Player& entity = pair.second;
        if (!IsValidTarget(entity)) continue;

        ImVec2 target2D;
        if (!ProjectHead(entity, target2D)) continue;

        const float playerDistance = PlayerDistance(entity);
        if (playerDistance > g_Globals.Exploits.SilentMemMaxDistance) continue;

        if (CrosshairDistance(target2D) > g_Globals.Exploits.SilentMemFov) continue;

        if (playerDistance < closestDistance) {
            closestDistance = playerDistance;
            best = &entity;
        }
    }

    if (!best) return false;
    outTarget = *best;
    return true;
}

bool SilentMem::GetTarget360(Player& outTarget)
{
    Player* best = nullptr;
    float closestDistance = FLT_MAX;

    SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
    for (auto& pair : g_Globals.EspConfig.Entities) {
        Player& entity = pair.second;
        if (!IsValidTarget(entity)) continue;

        const float playerDistance = PlayerDistance(entity);
        if (playerDistance > g_Globals.Exploits.SilentMemMaxDistance) continue;

        if (playerDistance < closestDistance) {
            closestDistance = playerDistance;
            best = &entity;
        }
    }

    if (!best) return false;
    outTarget = *best;
    return true;
}

bool SilentMem::GetTarget(Player& outTarget)
{
    if (g_Globals.EspConfig.Width < 1 || g_Globals.EspConfig.Height < 1 ||
        !g_Globals.EspConfig.Matrix || g_Globals.EspConfig.LocalPlayer == 0)
        return false;

    switch (g_Globals.Exploits.SilentMemTargetingMode) {
    case 1: return GetTarget360(outTarget);
    case 2: return GetTargetByLowestHealth(outTarget);
    case 3: return GetTargetByClosestToPlayer(outTarget);
    case 0:
    default: return GetTargetByClosestToCrosshair(outTarget);
    }
}

void SilentMem::Work()
{
    while (s_running) {
        try {
            if (!g_Globals.Exploits.SilentMem) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }

            if (!IsKeyActive()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            Player target{};
            if (GetTarget(target)) {
                PushFrame(target);
            }

            if (g_Globals.Exploits.SilentMemAimStyle != 0) {
                const int rawSmooth = g_Globals.Exploits.SilentMemSmooth;
                const int smoothMs = rawSmooth < 1 ? 1 : rawSmooth;
                std::this_thread::sleep_for(std::chrono::milliseconds(smoothMs));
            }

            std::this_thread::yield();
        }
        catch (...) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void SilentMem::Start()
{
    if (s_running) return;
    s_running = true;
    s_worker = std::thread(Work);
    s_worker.detach();
}

void SilentMem::Stop()
{
    s_running = false;
}

} // namespace Aim