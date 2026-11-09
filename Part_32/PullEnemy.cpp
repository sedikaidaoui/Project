#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <EspLines/Math/TMatrix.hpp>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Player.h>
#include <src/Globals.hpp>
#include "PullEnemy.hpp"

#include <chrono>
#include <cmath>
#include <cfloat>
#include <unordered_map>
#include <cstdint>

namespace {
    // Storage for original positions so we can restore them when not pulling
    std::unordered_map<uint32_t, Vector3> s_originalPositions;
    std::chrono::steady_clock::time_point s_lastTick{};

    // Restore all enemies to their original positions
    void RestoreAllPositions() {
        for (const auto& entry : s_originalPositions) {
            uint32_t rootBonePtr = 0, t1 = 0, t2 = 0, matrixPtr = 0;
            if (!Mem.ReadFast2<uint32_t>(entry.first + Offsets::Bones::Root, &rootBonePtr) || rootBonePtr == 0)
                continue;
            if (!Mem.ReadFast2<uint32_t>(rootBonePtr + 0x8, &t1) || t1 == 0)
                continue;
            if (!Mem.ReadFast2<uint32_t>(t1 + 0x8, &t2) || t2 == 0)
                continue;
            if (!Mem.ReadFast2<uint32_t>(t2 + 0x20, &matrixPtr) || matrixPtr == 0)
                continue;
            Mem.Write<Vector3>(matrixPtr + 0x60, entry.second);
        }
        s_originalPositions.clear();
    }

    // Find the best target: closest enemy head inside crosshair FOV
    Player* FindBestTarget() {
        Player* bestTarget = nullptr;
        float closestDist = FLT_MAX;

        const int w = g_Globals.EspConfig.Width;
        const int h = g_Globals.EspConfig.Height;
        if (w < 1 || h < 1 || !g_Globals.EspConfig.Matrix)
            return nullptr;

        const Vector2 screenCenter(w / 2.0f, h / 2.0f);
        const float aimFov = (float)g_Globals.Exploits.PullEnemyFov;
        const float maxDist3D = g_Globals.Exploits.PullEnemyDistance;
        const bool ignoreKnocked = g_Globals.AimBot.IgnoreKnocked;
        const uint32_t localAddr = g_Globals.EspConfig.LocalPlayer;

        for (auto& kv : g_Globals.EspConfig.Entities) {
            Player& ent = kv.second;
            if (ent.Address == 0 || ent.Address == localAddr)
                continue;
            if (!ent.IsKnown || ent.IsDead)
                continue;
            if (ent.IsTeam == Player::Bool3::True)
                continue;
            if (ignoreKnocked && ent.IsKnocked)
                continue;

            Vector2 head2D = W2S::WorldToScreen(g_Globals.EspConfig.ViewMatrix, ent.Head, w, h);
            if (head2D.X < 1.0f || head2D.Y < 1.0f)
                continue;

            float dist3D = Vector3::Distance(g_Globals.EspConfig.MainCamera, ent.Head);
            if (dist3D > maxDist3D)
                continue;

            float crosshairDist = Vector2::Distance(screenCenter, head2D);
            if (crosshairDist > aimFov)
                continue;

            if (crosshairDist < closestDist) {
                closestDist = crosshairDist;
                bestTarget = &ent;
            }
        }
        return bestTarget;
    }

    // Apply the instant pull: move enemy root position to fire line
    void ApplyInstantPull(Player& entity) {
        uint32_t rootBonePtr = 0, transformValue = 0, transformObjPtr = 0, matrixPtr = 0;
        if (!Mem.ReadFast2<uint32_t>(entity.Address + Offsets::Bones::Root, &rootBonePtr) || rootBonePtr == 0)
            return;
        if (!Mem.ReadFast2<uint32_t>(rootBonePtr + 0x8, &transformValue) || transformValue == 0)
            return;
        if (!Mem.ReadFast2<uint32_t>(transformValue + 0x8, &transformObjPtr) || transformObjPtr == 0)
            return;
        if (!Mem.ReadFast2<uint32_t>(transformObjPtr + 0x20, &matrixPtr) || matrixPtr == 0)
            return;

        Vector3 currentPos;
        if (!TransformUtils::GetNodePosition(rootBonePtr, currentPos))
            return;

        // Save original position if not already saved
        if (s_originalPositions.find(entity.Address) == s_originalPositions.end())
            s_originalPositions[entity.Address] = currentPos;

        const Vector3 originalPos = s_originalPositions[entity.Address];

        // Fire direction from view matrix (forward vector)
        Vector3 fireDir(
            g_Globals.EspConfig.ViewMatrix.m02,
            g_Globals.EspConfig.ViewMatrix.m12,
            g_Globals.EspConfig.ViewMatrix.m22);
        float len = sqrtf(fireDir.X * fireDir.X + fireDir.Y * fireDir.Y + fireDir.Z * fireDir.Z);
        if (len < 1e-4f) return;
        fireDir.X /= len;
        fireDir.Y /= len;
        fireDir.Z /= len;

        const Vector3 camPos = g_Globals.EspConfig.MainCamera;

        // Real distance from camera to root
        const Vector3 toRoot = currentPos - camPos;
        float dist = sqrtf(toRoot.X * toRoot.X + toRoot.Y * toRoot.Y + toRoot.Z * toRoot.Z);
        if (dist < 1.5f) dist = 1.5f;

        // New position = camera + (forward * dist), Y stays the same
        Vector3 finalPos;
        finalPos.X = camPos.X + fireDir.X * dist;
        finalPos.Y = originalPos.Y; // Don't change height
        finalPos.Z = camPos.Z + fireDir.Z * dist;

        // Pull percentage: blend between original and fire line
        const float blend = (float)g_Globals.Exploits.PullEnemyPercent * 0.01f;
        if (blend > 0.0f && blend != 1.0f) {
            finalPos.X = originalPos.X + (finalPos.X - originalPos.X) * blend;
            finalPos.Z = originalPos.Z + (finalPos.Z - originalPos.Z) * blend;
        }

        // Clamp X/Z +/-5f from original to prevent extreme pulls
        constexpr float kClampXZ = 5.0f;
        float deltaX = finalPos.X - originalPos.X;
        float deltaZ = finalPos.Z - originalPos.Z;
        if (fabsf(deltaX) > kClampXZ) finalPos.X = originalPos.X + copysignf(kClampXZ, deltaX);
        if (fabsf(deltaZ) > kClampXZ) finalPos.Z = originalPos.Z + copysignf(kClampXZ, deltaZ);

        Mem.Write<Vector3>(matrixPtr + 0x60, finalPos);
    }

    bool IsLocalFiring() {
        bool firing = false;
        if (!g_Globals.EspConfig.LocalPlayer) return false;
        return Mem.ReadFast2<bool>(g_Globals.EspConfig.LocalPlayer + Offsets::IsFiring, &firing) && firing;
    }
}

// ==================== Pull Enemy Main Run ====================
void PullEnemy::Run() {
    const int bind = g_Globals.Exploits.PullEnemyBind;
    if (bind != 0 && (GetAsyncKeyState(bind) & 0x1)) {
        g_Globals.Exploits.PullEnemy = !g_Globals.Exploits.PullEnemy;
    }

    const bool pullEnemyActive = g_Globals.Exploits.PullEnemy;

    if (!pullEnemyActive) {
        if (!s_originalPositions.empty()) RestoreAllPositions();
        s_lastTick = std::chrono::steady_clock::now();
        return;
    }

    const int tickMs = (g_Globals.Exploits.PullEnemyTickMs > 0) ? g_Globals.Exploits.PullEnemyTickMs : 6;
    const auto now = std::chrono::steady_clock::now();
    if (now - s_lastTick < std::chrono::milliseconds(tickMs))
        return;
    s_lastTick = now;

    if (g_Globals.EspConfig.Width < 1 || g_Globals.EspConfig.Height < 1 ||
        !g_Globals.EspConfig.Matrix || g_Globals.EspConfig.LocalPlayer == 0)
        return;

    // Pull Silent: only works when firing
    if (g_Globals.Exploits.PullSilent) {
        if (!IsLocalFiring()) {
            if (!s_originalPositions.empty()) RestoreAllPositions();
            return;
        }
        Player* target = FindBestTarget();
        if (target != nullptr) ApplyInstantPull(*target);
        else RestoreAllPositions();
        return;
    }

    // Magnet: normal firing check
    if (!IsLocalFiring()) {
        RestoreAllPositions();
        return;
    }

    Player* target = FindBestTarget();
    if (target != nullptr) ApplyInstantPull(*target);
    else RestoreAllPositions();
}