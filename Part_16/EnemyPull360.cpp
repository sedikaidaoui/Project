#include <imgui.h>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <EspLines/Math/TMatrix.hpp>
#include <src/Globals.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Player.h>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <chrono>
#include <algorithm>
#include "EnemyPull360.hpp"

std::atomic<bool> enemyPullRunning{ false };
std::thread enemyPullThread;

namespace {

    static std::unordered_map<uint32_t, Vector3> originalPositions;
    static Player* currentTarget = nullptr;

    static uint32_t s_stablePullEntity = 0;
    static Vector3 s_smoothedCamPos;
    static Vector3 s_smoothedFireDir;
    static float s_smoothedProjLen = 0.0f;
    static Vector3 s_smoothWritePos;
    static bool s_pullSmoothInited = false;

    static constexpr float kCamSmoothAlpha = 0.32f;
    static constexpr float kDirSmoothAlpha = 0.20f;
    static constexpr float kProjSmoothAlpha = 0.28f;
    static constexpr float kWriteSmoothAlpha = 0.54f;

    static Vector3 LerpVec(const Vector3& a, const Vector3& b, float t)
    {
        return {
            a.X + (b.X - a.X) * t,
            a.Y + (b.Y - a.Y) * t,
            a.Z + (b.Z - a.Z) * t
        };
    }

    static void RestoreAllPositions()
    {
        for (auto& entry : originalPositions) {
            uint32_t rootBone = 0, transformValue = 0, transformObj = 0, matrixPtr = 0;
            if (!Mem.ReadFast2<uint32_t>(entry.first + Offsets::Bones::Root, &rootBone) || !rootBone) continue;
            if (!Mem.ReadFast2<uint32_t>(rootBone + 0x8, &transformValue) || !transformValue) continue;
            if (!Mem.ReadFast2<uint32_t>(transformValue + 0x8, &transformObj) || !transformObj) continue;
            if (!Mem.ReadFast2<uint32_t>(transformObj + 0x20, &matrixPtr) || !matrixPtr) continue;
            Mem.Write<Vector3>(matrixPtr + 0x60, entry.second);
        }
        originalPositions.clear();
        s_pullSmoothInited = false;
        currentTarget = nullptr;
        s_stablePullEntity = 0;
    }

    static Player* FindBestTarget()
    {
        if (g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0 || !g_Globals.EspConfig.Matrix)
            return nullptr;

        float closestDist = FLT_MAX;
        Player* bestTarget = nullptr;
        float centerX = g_Globals.EspConfig.Width * 0.5f;
        float centerY = g_Globals.EspConfig.Height * 0.5f;
        float fov = (float)g_Globals.Exploits.EnemyPullFov;
        float maxDist = g_Globals.Exploits.EnemyPullMaxDistance;
        Vector3 camPos = g_Globals.EspConfig.MainCamera;

        SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
        for (auto& pair : g_Globals.EspConfig.Entities) {
            Player& entity = pair.second;
            if (!entity.IsKnown || entity.IsDead) continue;
            if (g_Globals.AimBot.IgnoreKnocked && entity.IsKnocked) continue;
            if (entity.IsTeam == Player::Bool3::True) continue;

            ImVec2 head2D = W2S::WorldToScreenImVec2(
                g_Globals.EspConfig.ViewMatrix, entity.Head,
                g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
            if (head2D.x < 1.0f || head2D.y < 1.0f) continue;

            float dist3D = Vector3::Distance(camPos, entity.Head);
            if (dist3D > maxDist) continue;

            float dx = head2D.x - centerX;
            float dy = head2D.y - centerY;
            float crosshairDist = sqrtf(dx * dx + dy * dy);
            if (crosshairDist > fov) continue;

            if (crosshairDist < closestDist) {
                closestDist = crosshairDist;
                bestTarget = &entity;
            }
        }
        return bestTarget;
    }

    static void ApplyInstantPull(Player& entity)
    {
        uint32_t rootBone = 0;
        if (!Mem.ReadFast2<uint32_t>(entity.Address + Offsets::Bones::Root, &rootBone) || !rootBone) return;

        Vector3 currentPos;
        if (!TransformUtils::GetNodePosition(rootBone, currentPos)) return;

        uint32_t transformValue = 0, transformObj = 0, matrixPtr = 0;
        if (!Mem.ReadFast2<uint32_t>(rootBone + 0x8, &transformValue) || !transformValue) return;
        if (!Mem.ReadFast2<uint32_t>(transformValue + 0x8, &transformObj) || !transformObj) return;
        if (!Mem.ReadFast2<uint32_t>(transformObj + 0x20, &matrixPtr) || !matrixPtr) return;

        if (originalPositions.find(entity.Address) == originalPositions.end())
            originalPositions[entity.Address] = currentPos;

        Vector3 originalPos = originalPositions[entity.Address];
        Vector3 camPos = g_Globals.EspConfig.MainCamera;

        Vector3 rawForward = {
            g_Globals.EspConfig.ViewMatrix.m02,
            g_Globals.EspConfig.ViewMatrix.m12,
            g_Globals.EspConfig.ViewMatrix.m22
        };
        rawForward = Vector3::Normalized(rawForward);

        uint32_t eid = entity.Address;
        if (!s_pullSmoothInited || s_stablePullEntity != eid) {
            s_smoothedCamPos = camPos;
            s_smoothedFireDir = rawForward;
            Vector3 toE = currentPos - camPos;
            float rp = Vector3::Dot(toE, rawForward);
            s_smoothedProjLen = rp < 1.5f ? 1.5f : rp;
            s_smoothWritePos = currentPos;
            s_stablePullEntity = eid;
            s_pullSmoothInited = true;
        } else {
            s_smoothedCamPos = LerpVec(s_smoothedCamPos, camPos, kCamSmoothAlpha);
            Vector3 blended = LerpVec(s_smoothedFireDir, rawForward, kDirSmoothAlpha);
            s_smoothedFireDir = Vector3::Normalized(blended);
            Vector3 toEnemy = currentPos - s_smoothedCamPos;
            float rawProj = Vector3::Dot(toEnemy, rawForward);
            if (rawProj < 1.5f) rawProj = 1.5f;
            s_smoothedProjLen += (rawProj - s_smoothedProjLen) * kProjSmoothAlpha;
        }

        Vector3 targetPos = s_smoothedCamPos + (s_smoothedFireDir * s_smoothedProjLen);
        targetPos.Y += g_Globals.Exploits.PullHeightOffset;

        float deltaX = targetPos.X - originalPos.X;
        float deltaZ = targetPos.Z - originalPos.Z;
        if (fabsf(deltaX) > 3.0f) targetPos.X = originalPos.X + (deltaX > 0.0f ? 3.0f : -3.0f);
        if (fabsf(deltaZ) > 3.0f) targetPos.Z = originalPos.Z + (deltaZ > 0.0f ? 3.0f : -3.0f);

        constexpr float maxUp = 0.50f;
        if (targetPos.Y < originalPos.Y)
            targetPos.Y = originalPos.Y;
        else if (targetPos.Y - originalPos.Y > maxUp)
            targetPos.Y = originalPos.Y + maxUp;

        s_smoothWritePos = LerpVec(s_smoothWritePos, targetPos, kWriteSmoothAlpha);
        Mem.Write<Vector3>(matrixPtr + 0x60, s_smoothWritePos);
    }

} // namespace

bool EnemyPull::EnemyPull360::IsLocalFiring()
{
    bool firing = false;
    if (!g_Globals.EspConfig.LocalPlayer) return false;
    if (!Mem.ReadFast2<bool>(g_Globals.EspConfig.LocalPlayer + Offsets::IsFiring, &firing))
        return false;
    return firing;
}

void EnemyPull::EnemyPull360::RestoreOriginalPosition()
{
    RestoreAllPositions();
}

void EnemyPull::EnemyPull360::Start()
{
    if (enemyPullRunning) return;
    enemyPullRunning = true;

    enemyPullThread = std::thread([] {
        while (enemyPullRunning) {
            if (!g_Globals.Exploits.EnemyPullEnabled) {
                RestoreAllPositions();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            if (g_Globals.EspConfig.Width <= 0 || !g_Globals.EspConfig.Matrix) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }

            bool pulling = false;
            if (IsLocalFiring()) {
                pulling = true;
                if (!currentTarget)
                    currentTarget = FindBestTarget();

                if (currentTarget && !currentTarget->IsDead)
                    ApplyInstantPull(*currentTarget);
                else
                    currentTarget = nullptr;
            } else {
                RestoreAllPositions();
            }

            int delay = g_Globals.Exploits.EnemyPullTickMs > 0 ? g_Globals.Exploits.EnemyPullTickMs : 2;
            if (pulling && delay > 1) delay = 1;
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
        RestoreAllPositions();
    });
    enemyPullThread.detach();
}

void EnemyPull::EnemyPull360::Stop()
{
    enemyPullRunning = false;
    RestoreAllPositions();
}
