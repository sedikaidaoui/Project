#include "SilentLite.hpp"

#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <EspLines/Math/TMatrix.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <src/Globals.hpp>

#include <Windows.h>
#include <cfloat>
#include <cmath>
#include <map>
#include <vector>
#include <cstdint>

// ===========================================================================
//  SILENT - LITE
//  Code source recopie TEL QUEL (ajout ni retrait d'aucune ligne de logique).
//  Seuls des shims sont definis ci-dessous pour que ce code compile avec
//  l'API de CE projet (Mem.Read / Mem.Write / TransformUtils / W2S).
// ===========================================================================

// ---- SHIMS (aucune modification du code source ci-dessous) -----------------
template <typename T> inline bool ReadZ(uintptr_t address, T& out) { return Mem.Read<T>(address, out); }
template <typename T> inline void WriteZ(uintptr_t address, const T& value) { Mem.Write<T>(address, value); }

static bool GetNodePosition(uint32_t nodeTransform, Vector3& result) {
    return TransformUtils::GetNodePosition(nodeTransform, result);
}

static ImVec2 WorldToScreenImVec2(const Matrix4x4& viewMatrix, const Vector3& pos, int width, int height) {
    return W2S::WorldToScreenImVec2(viewMatrix, pos, width, height);
}

static float GetDistance2D(const ImVec2& a, const ImVec2& b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

// ---- variables globales du code source ------------------------------------
// Blacklist Wukong / Ghost (vide : aucun membre de l'equipe n'est filtre)
std::vector<uint32_t> g_wukongBlacklist;

// false = test de visibilite oblige (ne pas traverser les murs)
static bool  SHowVis = false;

// distance max cible
static float maxDistance = 400.0f;

// master toggle "SILENT LITE" du menu
static bool& aimsilent = g_Globals.AimBot.SilentLiteEnabled;

// mode de ciblage : 0 = Crosshair, 1 = 360 (combo "Silent Mode" du menu)
static int SilentAimMode = 0;

// ===========================================================================
//  CODE SOURCE - NE RIEN MODIFIER CI-DESSOUS
// ===========================================================================

static float SilentAimFOV = 1000.0f;

struct SilentTarget {
    uint32_t entity = 0;
    Vector3 headPosition;
    float screenDistance = FLT_MAX;
    float distance3D = FLT_MAX;
};


SilentTarget FindBestSilentTarget(uint32_t localPlayer, Vector3 cameraPos, Matrix4x4 viewMatrix, uint32_t entities, uint32_t count, int screenWidth, int screenHeight) {
    SilentTarget bestTarget;
    bestTarget.entity = 0;
    bestTarget.screenDistance = FLT_MAX;
    bestTarget.distance3D = FLT_MAX;
    ImVec2 screenCenter = ImVec2(screenWidth / 2.0f, screenHeight / 2.0f);
    const float margin = 80.0f;

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t entityAddr = 0;
        if (!ReadZ(entities + i * 0x10, entityAddr) || entityAddr == 0 || entityAddr == localPlayer)
            continue;

        bool isGhostBanned = false;
        for (auto locked_entity : g_wukongBlacklist) {
            if (locked_entity == entityAddr) {
                isGhostBanned = true;
                break;
            }
        }
        if (isGhostBanned) continue;

        bool entityIsDead = false;
        ReadZ(entityAddr + Offsets::Player_IsDead, entityIsDead);
        if (entityIsDead) continue;

        bool isKnocked = false;
        uint32_t shadowBase = 0;
        if (ReadZ(entityAddr + Offsets::Player_ShadowBase, shadowBase) && shadowBase != 0) {
            int pose;
            if (ReadZ(shadowBase + Offsets::XPose, pose) && pose == 8)
                isKnocked = true;
        }
        if (isKnocked) continue;

        uint32_t avatarManager = 0;
        if (!ReadZ(entityAddr + Offsets::AvatarManager, avatarManager) || avatarManager == 0) continue;

        uint32_t avatar = 0;
        if (!ReadZ(avatarManager + Offsets::Avatar, avatar) || avatar == 0) continue;

        if (!SHowVis) {
            // test de visibilite : ne pas traverser les murs
            bool isVisible = false;
            if (!ReadZ(avatar + Offsets::Avatar_IsVisible, isVisible) || !isVisible)
                continue;
        }
        uint32_t avatarData = 0;
        if (!ReadZ(avatar + Offsets::Avatar_Data, avatarData) || avatarData == 0) continue;

        bool isTeam = false;
        if (ReadZ(avatarData + Offsets::Avatar_Data_IsTeam, isTeam) && isTeam) continue;

        uint32_t headBone = 0;
        if (!ReadZ(entityAddr + Offsets::Bones::Head, headBone) || headBone == 0)
            continue;

        Vector3 headWorld = {};
        if (!GetNodePosition(headBone, headWorld))
            continue;

        float distance3D = Vector3::Distance(cameraPos, headWorld);
        if (distance3D > maxDistance || distance3D < 1.0f) continue;

        ImVec2 head2D = WorldToScreenImVec2(viewMatrix, headWorld, screenWidth, screenHeight);
        bool onScreen = (head2D.x >= -margin && head2D.x <= (float)screenWidth + margin &&
            head2D.y >= -margin && head2D.y <= (float)screenHeight + margin);
        float distToCrosshair = GetDistance2D(head2D, screenCenter);

        if (SilentAimMode == 0) {
            // mode Crosshair : plus proche du centre, dans le FOV uniquement
            if (!onScreen) continue;
            if (distToCrosshair > SilentAimFOV) continue;

            if (distToCrosshair < bestTarget.screenDistance ||
                (distToCrosshair == bestTarget.screenDistance && distance3D < bestTarget.distance3D)) {
                bestTarget.entity = entityAddr;
                bestTarget.headPosition = headWorld;
                bestTarget.screenDistance = distToCrosshair;
                bestTarget.distance3D = distance3D;
            }
        }
        else if (SilentAimMode == 1) {
            float score = distance3D * (onScreen ? 1.0f : 3.0f);
            if (score < bestTarget.distance3D) {
                bestTarget.entity = entityAddr;
                bestTarget.headPosition = headWorld;
                bestTarget.distance3D = score;
                bestTarget.screenDistance = distToCrosshair;
            }
        }
    }

    return bestTarget;
}


void SILENTKILL(uint32_t localPlayer, const Vector3& targetHead, uintptr_t targetEntity) {
    if (targetHead.X == 0 && targetHead.Y == 0 && targetHead.Z == 0)
        return;

    uint32_t weaponInstance = 0;
    for (int attempt = 0; attempt < 5; attempt++) {
        if (ReadZ(localPlayer + Offsets::sAim2, weaponInstance) && weaponInstance != 0)
            break;
    }
    if (weaponInstance == 0)
        return;

    Vector3 fireStart;
    if (!ReadZ(weaponInstance + Offsets::sAim3, fireStart))
        return;

    Vector3 adjustedHead = targetHead;
    adjustedHead.Y += 0.1f;
    Vector3 aimDir = adjustedHead - fireStart;

    float len = Vector3::Magnitude(aimDir);
    if (len > 0.0001f) {
        aimDir.X /= len;
        aimDir.Y /= len;
        aimDir.Z /= len;
    }

    for (int i = 0; i < 120; i++)
        WriteZ<Vector3>(weaponInstance + Offsets::sAim4, aimDir);


}

// --- declencheur (code source, meme lignes) --------------------------------
void Lite::SilentLiteRun(uint32_t localPlayer,
                         uint32_t entities,
                         uint32_t entitiesCount,
                         Vector3 mainPos,
                         Matrix4x4 viewMatrix,
                         int g_windowWidth,
                         int g_windowHeight)
{
    // synchronise avec la combo "Silent Mode" du menu (Crosshair / 360)
    SilentAimMode = g_Globals.AimBot.SilentAimMode;

    //      RageAimbot360Method(localPlayer, entities, entitiesCount, mainPos, viewMatrix, g_windowWidth, g_windowHeight);
    if (aimsilent && (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0)
    {
        static SilentTarget lastTarget;
        lastTarget = FindBestSilentTarget(localPlayer, mainPos, viewMatrix, entities, entitiesCount, g_windowWidth, g_windowHeight);
        if (lastTarget.entity != 0)
        {
            for (int i = 0; i < 130; i++)
                SILENTKILL(localPlayer, lastTarget.headPosition, lastTarget.entity);

        }
    }
}
