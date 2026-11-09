#include "ExtremeAimbot.hpp"
#include <imgui.h>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <EspLines/Math/Quaternion.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Player.h>
#include <src/Globals.hpp>
#include <Windows.h>
#include <cmath>
#include <thread>
#include <chrono>

namespace Aim {

namespace {

    std::atomic<bool> s_running{ false };
    std::thread s_worker;

    bool IsKeyActive()
    {
        if (g_Globals.AimBot.ExtremeWhenShooting)
            return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        if (g_Globals.AimBot.ExtremeLMouse)
            return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        if (g_Globals.AimBot.ExtremeAimKey != 0)
            return (GetAsyncKeyState(g_Globals.AimBot.ExtremeAimKey) & 0x8000) != 0;

        return (GetAsyncKeyState(g_Globals.AimBot.AimbotBind) & 0x8000) != 0;
    }

} // namespace

Vector3 ExtremeAimbot::GetHitbox(const Player& entity)
{
    switch (g_Globals.AimBot.ExtremeHitbox) {
    case 1: return entity.Neck;
    case 2: return entity.LeftShoulder;
    case 3: return entity.RightShoulder;
    case 0:
    default: return entity.Head;
    }
}

bool ExtremeAimbot::FindBestTarget(Player& outTarget)
{
    if (g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0 || !g_Globals.EspConfig.Matrix)
        return false;

    Player* bestTarget = nullptr;
    float bestMetric = FLT_MAX;
    const float centerX = g_Globals.EspConfig.Width * 0.5f;
    const float centerY = g_Globals.EspConfig.Height * 0.5f;
    const float fovRange = static_cast<float>(g_Globals.AimBot.Fov);
    const Vector3 camPos = g_Globals.EspConfig.MainCamera;

    SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
    for (auto& pair : g_Globals.EspConfig.Entities) {
        Player& entity = pair.second;

        if (!entity.IsKnown || entity.IsDead) continue;
        if (g_Globals.AimBot.IgnoreKnocked && entity.IsKnocked) continue;
        if (g_Globals.AimBot.IgnoreBots && entity.IsBot) continue;
        if (entity.IsTeam == Player::Bool3::True) continue;
        if (entity.Address == g_Globals.EspConfig.LocalPlayer) continue;

        if (g_Globals.AimBot.ExtremeIgnoreTrainingBots && !entity.Name.empty()) continue;

        Vector3 targetBone = GetHitbox(entity);
        ImVec2 screenPos = W2S::WorldToScreenImVec2(
            g_Globals.EspConfig.ViewMatrix, targetBone,
            g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
        if (screenPos.x < 0.0f || screenPos.y < 0.0f) continue;

        const float playerDistance = Vector3::Distance(camPos, targetBone);
        if (playerDistance > static_cast<float>(g_Globals.AimBot.ExtremeDistance)) continue;

        const float dx = screenPos.x - centerX;
        const float dy = screenPos.y - centerY;
        const float crosshairDist = sqrtf(dx * dx + dy * dy);

        if (g_Globals.AimBot.ExtremeUseFov && crosshairDist > fovRange) continue;

        const bool useCrosshair = g_Globals.AimBot.ExtremePriorityCrosshair ||
            (!g_Globals.AimBot.ExtremePriorityLowHealth && !g_Globals.AimBot.ExtremePriorityNearest);

        bool better = false;

        if (useCrosshair) {
            better = crosshairDist < bestMetric;
            if (better) bestMetric = crosshairDist;
        }

        if (!better && g_Globals.AimBot.ExtremePriorityLowHealth) {
            better = entity.Health < bestMetric;
            if (better) bestMetric = static_cast<float>(entity.Health);
        }

        if (!better && g_Globals.AimBot.ExtremePriorityNearest) {
            better = playerDistance < bestMetric;
            if (better) bestMetric = playerDistance;
        }

        if (better)
            bestTarget = &entity;
    }

    if (!bestTarget) return false;
    outTarget = *bestTarget;
    return true;
}

void ExtremeAimbot::Work()
{
    while (s_running) {
        const bool active = g_Globals.AimBot.Enabled && g_Globals.AimBot.AimbotType == 2;

        if (!active) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        if (!IsKeyActive()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        if (g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0 || !g_Globals.EspConfig.Matrix) {
            std::this_thread::yield();
            continue;
        }

        uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;
        if (!localPlayer) {
            std::this_thread::yield();
            continue;
        }

        Player target;
        if (FindBestTarget(target)) {
            Vector3 selectedBone = GetHitbox(target);
            Quaternion newRot = Quaternion::GetRotationToLocation(
                selectedBone, 0.1f, g_Globals.EspConfig.MainCamera);
            Mem.Write<Quaternion>(localPlayer + Offsets::AimRotation, newRot);
        }

        std::this_thread::yield();
    }
}

void ExtremeAimbot::Start()
{
    if (s_running) return;
    s_running = true;
    s_worker = std::thread(Work);
    s_worker.detach();
}

void ExtremeAimbot::Stop()
{
    s_running = false;
}

} // namespace Aim
