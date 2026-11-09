#include "MagnetLite.hpp"

#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <EspLines/Math/TMatrix.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <src/Globals.hpp>

#include <chrono>
#include <cmath>
#include <cfloat>
#include <map>
#include <cstdint>

namespace {

// --- etat persistant (identique au source "Magnet") -------------------------
std::chrono::steady_clock::time_point s_lastFire{};
std::map<uint32_t, Vector3>           s_originalPositions;

bool     s_pullSmoothInited  = false;
uint32_t s_stablePullEntity  = 0;
Vector3  s_smoothWritePos    = Vector3::Zero();
float    s_smoothedProjLen   = 0.0f;
Vector3  s_smoothedFireDir   = Vector3::Zero();
Vector3  s_smoothedCamPos    = Vector3::Zero();

// Lecture / ecriture memoire (equivalent ReadZ / WriteZ)
template <typename T> inline bool RZ(uintptr_t address, T& out) { return Mem.Read<T>(address, out); }
template <typename T> inline void WZ(uintptr_t address, const T& value) { Mem.Write<T>(address, value); }

inline bool ReadPtr(uintptr_t address, uint32_t& out) {
    out = 0;
    return Mem.ReadFast2<uint32_t>(address, &out) && out != 0;
}

// Ramene tous les ennemis tirees a leur position d'origine
void RestoreAll() {
    for (const auto& entry : s_originalPositions) {
        uint32_t rootBone = 0, trans = 0, obj = 0, matrix = 0;
        if (ReadPtr(entry.first + Offsets::Bones::Root, rootBone) &&
            ReadPtr(rootBone + 0x8, trans) &&
            ReadPtr(trans + 0x8, obj) &&
            ReadPtr(obj + 0x20, matrix)) {
            WZ<Vector3>(matrix + 0x60, entry.second);
        }
    }
    s_originalPositions.clear();
    s_pullSmoothInited = false;
    s_stablePullEntity = 0;
}

} // namespace

void Lite::MagnetLiteRun(uint32_t localPlayer,
                         uint32_t entities,
                         uint32_t entitiesCount,
                         const Vector3& mainPos,
                         const Matrix4x4& viewMatrix,
                         int screenWidth,
                         int screenHeight)
{
    if (!g_Globals.Exploits.MagnetLite) {
        if (!s_originalPositions.empty()) RestoreAll();
        return;
    }

    if (!localPlayer || !entities || entitiesCount < 1 || screenWidth < 1 || screenHeight < 1)
        return;

    const float PullMagnetSilentFOV = g_Globals.Exploits.MagnetLiteFov;
    const float fireCooldownMs360    = g_Globals.Exploits.MagnetLiteCooldownMs;

    // ---- detection du tir (saim1) ----
    bool isCurrentlyFiring = false;
    if (RZ(localPlayer + Offsets::saim1, isCurrentlyFiring) && isCurrentlyFiring)
        s_lastFire = std::chrono::steady_clock::now();

    const bool isFiring =
        (std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - s_lastFire).count() < fireCooldownMs360);

    if (isFiring)
    {
        // ---- meilleure cible (plus proche du viseur) ----
        uint32_t bestEntity  = 0;
        float    closestDist = FLT_MAX;
        const ImVec2 screenCenter(screenWidth / 2.0f, screenHeight / 2.0f);

        for (uint32_t i = 0; i < entitiesCount; ++i)
        {
            uint32_t entityAddr = 0;
            if (!Mem.ReadFast2<uint32_t>(entities + i * 0x10, &entityAddr)
                || entityAddr == 0
                || entityAddr == localPlayer)
                continue;

            bool isDead = false;
            if (!RZ(entityAddr + Offsets::Player_IsDead, isDead) || isDead)
                continue;

            bool isTeam = false;
            uint32_t avatarManager = 0, avatar = 0, avatarData = 0;
            if (!ReadPtr(entityAddr + Offsets::AvatarManager, avatarManager) ||
                !ReadPtr(avatarManager + Offsets::Avatar, avatar) ||
                !ReadPtr(avatar + Offsets::Avatar_Data, avatarData))
                continue;
            if (!RZ(avatarData + Offsets::Avatar_Data_IsTeam, isTeam) || isTeam)
                continue;

            uint32_t headBone = 0;
            if (!ReadPtr(entityAddr + Offsets::Bones::Head, headBone))
                continue;

            Vector3 headPos = Vector3::Zero();
            if (!TransformUtils::GetNodePosition(headBone, headPos))
                continue;

            const ImVec2 head2D = W2S::WorldToScreenImVec2(viewMatrix, headPos, screenWidth, screenHeight);
            const float dx = head2D.x - screenCenter.x;
            const float dy = head2D.y - screenCenter.y;
            const float crosshairDist = sqrtf(dx * dx + dy * dy);

            if (crosshairDist < PullMagnetSilentFOV && crosshairDist < closestDist)
            {
                closestDist = crosshairDist;
                bestEntity  = entityAddr;
            }
        }

        // ---- application du pull sur la meilleure cible ----
        if (bestEntity)
        {
            uint32_t rootBone = 0;
            if (ReadPtr(bestEntity + Offsets::Bones::Root, rootBone))
            {
                Vector3 currentPos = Vector3::Zero();
                if (TransformUtils::GetNodePosition(rootBone, currentPos))
                {
                    // Sauvegarde de la position d'origine (premiere rencontre)
                    if (s_originalPositions.find(bestEntity) == s_originalPositions.end())
                        s_originalPositions[bestEntity] = currentPos;

                    const Vector3 originalPos = s_originalPositions[bestEntity];
                    const Vector3 camPos = mainPos;

                    // Direction de tir depuis la matrice de vue (forward)
                    const Vector3 rawForward = Vector3::Normalized(
                        Vector3(viewMatrix.m02, viewMatrix.m12, viewMatrix.m22));

                    // Init / reset du lissage quand la cible change
                    if (!s_pullSmoothInited || s_stablePullEntity != bestEntity)
                    {
                        s_smoothedCamPos = camPos;
                        s_smoothedFireDir = rawForward;
                        const Vector3 toE = currentPos - camPos;
                        const float rp = Vector3::Dot(toE, rawForward);
                        s_smoothedProjLen = rp < 1.5f ? 1.5f : rp;
                        s_smoothWritePos = currentPos;
                        s_stablePullEntity = bestEntity;
                        s_pullSmoothInited = true;
                    }
                    else
                    {
                        // Lissage de la position camera
                        s_smoothedCamPos.X += (camPos.X - s_smoothedCamPos.X) * 0.1f;
                        s_smoothedCamPos.Y += (camPos.Y - s_smoothedCamPos.Y) * 0.1f;
                        s_smoothedCamPos.Z += (camPos.Z - s_smoothedCamPos.Z) * 0.1f;

                        // Lissage de la direction de tir (re-normalise)
                        Vector3 blended;
                        blended.X = s_smoothedFireDir.X + (rawForward.X - s_smoothedFireDir.X) * 0.1f;
                        blended.Y = s_smoothedFireDir.Y + (rawForward.Y - s_smoothedFireDir.Y) * 0.1f;
                        blended.Z = s_smoothedFireDir.Z + (rawForward.Z - s_smoothedFireDir.Z) * 0.1f;
                        const float lenSq = blended.X * blended.X + blended.Y * blended.Y + blended.Z * blended.Z;
                        s_smoothedFireDir = (lenSq > 1e-10f) ? Vector3::Normalized(blended) : rawForward;

                        // Lissage de la longueur projettee
                        const Vector3 toEnemy = currentPos - s_smoothedCamPos;
                        float rawProj = Vector3::Dot(toEnemy, rawForward);
                        if (rawProj < 1.5f) rawProj = 1.5f;
                        s_smoothedProjLen += (rawProj - s_smoothedProjLen) * 0.1f;
                    }

                    // Point cible sur la ligne de tir
                    Vector3 targetPos;
                    targetPos.X = s_smoothedCamPos.X + s_smoothedFireDir.X * s_smoothedProjLen;
                    targetPos.Y = s_smoothedCamPos.Y + s_smoothedFireDir.Y * s_smoothedProjLen;
                    targetPos.Z = s_smoothedCamPos.Z + s_smoothedFireDir.Z * s_smoothedProjLen;
                    targetPos.Y += 0.08f;

                    // Clamp horizontal (max 3 unites depuis l'original)
                    const float deltaX = targetPos.X - originalPos.X;
                    const float deltaZ = targetPos.Z - originalPos.Z;
                    if (fabsf(deltaX) > 3.0f)
                        targetPos.X = originalPos.X + (deltaX > 0.f ? 3.0f : -3.0f);
                    if (fabsf(deltaZ) > 3.0f)
                        targetPos.Z = originalPos.Z + (deltaZ > 0.f ? 3.0f : -3.0f);

                    // Clamp vertical (max 0.50 vers le haut, jamais vers le bas)
                    if (targetPos.Y < originalPos.Y)
                        targetPos.Y = originalPos.Y;
                    else if (targetPos.Y - originalPos.Y > 0.50f)
                        targetPos.Y = originalPos.Y + 0.50f;

                    // Lissage de la position ecrite
                    s_smoothWritePos.X += (targetPos.X - s_smoothWritePos.X) * 0.1f;
                    s_smoothWritePos.Y += (targetPos.Y - s_smoothWritePos.Y) * 0.1f;
                    s_smoothWritePos.Z += (targetPos.Z - s_smoothWritePos.Z) * 0.1f;

                    // Ecriture de la position lisee dans la matrice racine
                    uint32_t trans = 0, obj = 0, matrix = 0;
                    if (ReadPtr(rootBone + 0x8, trans) &&
                        ReadPtr(trans + 0x8, obj) &&
                        ReadPtr(obj + 0x20, matrix))
                    {
                        WZ<Vector3>(matrix + 0x60, s_smoothWritePos);
                    }
                }
            }
        }
    }
    else if (!s_originalPositions.empty())
    {
        // ---- restauration de toutes les positions d'origine ----
        RestoreAll();
    }
}
