#define NOMINMAX
#undef min
#undef max

#include "ForceAimFeatures.hpp"
#include "MovementExploits.hpp"
#include <EspLines/Player.h>
#include <src/Globals.hpp>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/TMatrix.hpp>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <EspLines/Exploits/Exploit/BackJump.hpp>
#include <EspLines/Exploits/Exploit/TelaParada.hpp>
#include <EspLines/Exploits/Exploit/PullEnemy.hpp>
#include <EspLines/Exploits/Exploit/EnemyDown.hpp>
#include <EspLines/Exploits/Exploit/RapidFly.hpp>
#include <EspLines/Exploits/Exploit/SpeedHack.hpp>
#include <EspLines/Exploits/Exploit/Underground.hpp>
#include <cmath>
#include <chrono>
#include <vector>
#include <cfloat>
#include <unordered_map>
#include <Windows.h>

namespace FWork {
namespace ForceAim {
    static const float ClimbDepth = 1.0f;
    static const int MoveDurationMs = 400;
    static const int SmoothSteps = 20;
    static Vector3 savedOriginalPos;
    static bool hasSavedPos = false;
    static bool isMoving = false;
    static bool isUnderground = false;
    static int currentSmoothStep = 0;
    static Vector3 smoothStart;
    static Vector3 smoothEnd;
    static std::chrono::steady_clock::time_point smoothStartTime;
    static bool previousDivekillState = false;

    static std::unordered_map<uint32_t, Vector3> s_originalRoots;
    static uint32_t s_lockedAddr = 0;
    static int s_lastForceAimMode = -1;

    Player* FindClosestEnemy() {
        float closestDistance = FLT_MAX;
        Player* closestEntity = nullptr;

        Vector2 screenCenter(g_Globals.EspConfig.Width / 2.0f, g_Globals.EspConfig.Height / 2.0f);

        for (auto& pair : g_Globals.EspConfig.Entities) {
            Player* entity = &pair.second;

            if (entity->IsDead || (g_Globals.AimBot.IgnoreKnocked && entity->IsKnocked)) continue;

            ImVec2 head2D = W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, entity->Head, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);

            if (head2D.x < 1 || head2D.y < 1) continue;

            if (entity->Name.find("FARHAN.cpp") != std::string::npos) {
                continue;
            }

            float distance = Vector3::Distance(g_Globals.EspConfig.MainCamera, entity->Head);
            if (distance > g_Globals.AimBot.DistanceAim) continue;

            float crosshairDist = std::sqrt(std::pow(head2D.x - screenCenter.X, 2) + std::pow(head2D.y - screenCenter.Y, 2));
            if (crosshairDist >= closestDistance) continue;

            closestDistance = crosshairDist;
            closestEntity = entity;
        }

        return closestEntity;

    }

    // ---- Dive Kill ----
    void divekill()
    {
        if (g_Globals.EspConfig.LocalPlayer == 0 || !g_Globals.EspConfig.Matrix) return;

        if (!g_Globals.General.divekill && isUnderground && !isMoving)
        {
            if (hasSavedPos)
            {
                uint32_t rootBonePtr = Mem.Read<uint32_t>(g_Globals.EspConfig.LocalPlayer + static_cast<uint32_t>(Offsets::Bones::Root));
                if (rootBonePtr != 0)
                {
                    uint32_t transformValue = Mem.Read<uint32_t>(rootBonePtr + 0x8);
                    if (transformValue != 0)
                    {
                        uint32_t transformObjPtr = Mem.Read<uint32_t>(transformValue + 0x8);
                        if (transformObjPtr != 0)
                        {
                            uint32_t matrixPtr = Mem.Read<uint32_t>(transformObjPtr + 0x20);
                            if (matrixPtr != 0)
                            {
                                Vector3 currentPos;
                                if (TransformUtils::GetNodePosition(rootBonePtr, currentPos))
                                {
                                    isMoving = true;
                                    currentSmoothStep = 0;
                                    smoothStart = currentPos;
                                    smoothEnd = savedOriginalPos;
                                    smoothStartTime = std::chrono::steady_clock::now();
                                }
                            }
                        }
                    }
                }
            }

            previousDivekillState = false;

        }

        if (!g_Globals.General.divekill && !isMoving)
        {

            isUnderground = false;
            hasSavedPos = false;
            previousDivekillState = false;
            return;
        }

        try
        {
            uint32_t rootBonePtr = Mem.Read<uint32_t>(g_Globals.EspConfig.LocalPlayer + static_cast<uint32_t>(Offsets::Bones::Root));
            if (rootBonePtr == 0) return;

            uint32_t transformValue = Mem.Read<uint32_t>(rootBonePtr + 0x8);
            if (transformValue == 0) return;

            uint32_t transformObjPtr = Mem.Read<uint32_t>(transformValue + 0x8);
            if (transformObjPtr == 0) return;

            uint32_t matrixPtr = Mem.Read<uint32_t>(transformObjPtr + 0x20);
            if (matrixPtr == 0) return;

            Vector3 currentPos;
            if (!TransformUtils::GetNodePosition(rootBonePtr, currentPos)) return;

            bool justActivated = !previousDivekillState && g_Globals.General.divekill;

            if (g_Globals.General.divekill && !isUnderground && !isMoving)
            {
                savedOriginalPos = currentPos;
                hasSavedPos = true;

                Vector3 target = Vector3(savedOriginalPos.X, savedOriginalPos.Y - ClimbDepth, savedOriginalPos.Z);
                isMoving = true;
                currentSmoothStep = 0;
                smoothStart = currentPos;
                smoothEnd = target;
                smoothStartTime = std::chrono::steady_clock::now();
            }

            previousDivekillState = g_Globals.General.divekill;

            if (isMoving)
            {
                auto currentTime = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - smoothStartTime).count();

                if (elapsed >= MoveDurationMs)
                {
                    Mem.Write<Vector3>(matrixPtr + 0x60, smoothEnd);
                    isMoving = false;
                    if (smoothEnd.Y < smoothStart.Y)
                    {

                        isUnderground = true;
                    }
                    else
                    {

                        isUnderground = false;
                        hasSavedPos = false;
                    }
                }
                else
                {

                    float t = elapsed / (float)MoveDurationMs;
                    Vector3 pos = Vector3::Lerp(smoothStart, smoothEnd, t);
                    Mem.Write<Vector3>(matrixPtr + 0x60, pos);
                }
            }
            else if (isUnderground && hasSavedPos)
            {

                Vector3 locked = Vector3(
                    savedOriginalPos.X,
                    savedOriginalPos.Y - ClimbDepth,
                    savedOriginalPos.Z
                );
                Mem.Write<Vector3>(matrixPtr + 0x60, locked);
            }
        }
        catch (const std::exception& e)
        {

        }
    }

    // ---- Force Aim Core (from ForceAimCpp - improved) ----

    static bool ResolveRootMatrix(uint32_t entityAddr, uint32_t& matrixPtr) {
        uint32_t rootBone = 0, t1 = 0, t2 = 0;
        matrixPtr = 0;
        if (!Mem.Read(entityAddr + static_cast<uint32_t>(Offsets::Bones::Root), rootBone) || rootBone == 0)
            return false;
        if (!Mem.Read(rootBone + 0x8, t1) || t1 == 0)
            return false;
        if (!Mem.Read(t1 + 0x8, t2) || t2 == 0)
            return false;
        if (!Mem.Read(t2 + 0x20, matrixPtr) || matrixPtr == 0)
            return false;
        return true;
    }

    static void WriteRootPosition(uint32_t entityAddr, const Vector3& pos) {
        uint32_t matrixPtr = 0;
        if (!ResolveRootMatrix(entityAddr, matrixPtr))
            return;
        Mem.Write<Vector3>(matrixPtr + 0x60, pos);
    }

    static void ClearLock() {
        s_lockedAddr = 0;
    }

    static Vector3 ReadLiveBone(uint32_t entityAddr, uintptr_t boneOffset) {
        uint32_t bonePtr = 0;
        if (!Mem.Read(entityAddr + static_cast<uint32_t>(boneOffset), bonePtr) || bonePtr == 0)
            return Vector3::Zero();
        Vector3 pos;
        if (!TransformUtils::GetNodePosition(bonePtr, pos) || pos == Vector3::Zero())
            return Vector3::Zero();
        return pos;
    }

    static void RestoreAll() {
        for (const auto& entry : s_originalRoots)
            WriteRootPosition(entry.first, entry.second);
        s_originalRoots.clear();
        ClearLock();
    }

    static Vector3 GetAimPoint(const Player& p) {
        if (p.Address == 0)
            return Vector3::Zero();

        if (g_Globals.General.FORCEPART == 1) {
            Vector3 hip = ReadLiveBone(p.Address, Offsets::Bones::Hip);
            if (hip != Vector3::Zero())
                return hip;
            return (p.Hip != Vector3::Zero()) ? p.Hip : Vector3::Zero();
        }

        Vector3 head = ReadLiveBone(p.Address, Offsets::Bones::Head);
        if (head != Vector3::Zero())
            return head;
        return (p.Head != Vector3::Zero()) ? p.Head : Vector3::Zero();
    }

    static bool IsValidTarget(const Player& p, uint32_t localAddr) {
        if (p.Address == 0 || p.Address == localAddr)
            return false;
        if (!p.IsKnown || p.IsDead || p.IsTeam == Player::Bool3::True)
            return false;
        if (g_Globals.AimBot.IgnoreKnocked && p.IsKnocked)
            return false;
        return GetAimPoint(p) != Vector3::Zero();
    }

    static Player* FindTarget() {
        Player* best = nullptr;
        float bestFov = FLT_MAX;
        const float cx = static_cast<float>(g_Globals.EspConfig.Width) / 2.0f;
        const float cy = static_cast<float>(g_Globals.EspConfig.Height) / 2.0f;
        const uint32_t localAddr = g_Globals.EspConfig.LocalPlayer;
        const float maxDist = g_Globals.General.ForceAimMaxDistance;
        const float aimFov = static_cast<float>(g_Globals.AimBot.Fov);

        for (auto& kv : g_Globals.EspConfig.Entities) {
            Player* p = &kv.second;
            if (!IsValidTarget(*p, localAddr))
                continue;

            Vector3 aimPt = GetAimPoint(*p);
            Vector2 scr = W2S::WorldToScreen(g_Globals.EspConfig.ViewMatrix, aimPt,
                g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
            if (scr.X < 1.0f || scr.Y < 1.0f)
                continue;

            float dist = Vector3::Distance(g_Globals.EspConfig.MainCamera, aimPt);
            if (dist > maxDist)
                continue;

            float fov = sqrtf((scr.X - cx) * (scr.X - cx) + (scr.Y - cy) * (scr.Y - cy));
            if (fov > aimFov || fov >= bestFov)
                continue;

            bestFov = fov;
            best = p;
        }
        return best;
    }

    static void Tick() {
        uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;

        if (!g_Globals.General.ForceonAim) {
            if (!s_originalRoots.empty())
                RestoreAll();
            s_lastForceAimMode = -1;
            return;
        }
        if (g_Globals.EspConfig.Width < 1 || g_Globals.EspConfig.Height < 1 ||
            !g_Globals.EspConfig.Matrix || localPlayer == 0)
            return;

        const int aimMode = (g_Globals.General.FORCEPART == 1) ? 1 : 0;
        if (s_lastForceAimMode != aimMode) {
            s_lastForceAimMode = aimMode;
            if (!s_originalRoots.empty())
                RestoreAll();
            ClearLock();
        }

        bool isFiring = Mem.Read<bool>(localPlayer + Offsets::IsFiring);
        const bool lmbHeld = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        if (!isFiring && !lmbHeld) {
            if (!s_originalRoots.empty())
                RestoreAll();
            ClearLock();
            return;
        }

        Player* target = FindTarget();
        if (!target) {
            if (!s_originalRoots.empty())
                RestoreAll();
            return;
        }

        if (!s_originalRoots.empty() && s_originalRoots.find(target->Address) == s_originalRoots.end()) {
            RestoreAll();
        }

        uint32_t rootBone = 0;
        if (!Mem.Read(target->Address + static_cast<uint32_t>(Offsets::Bones::Root), rootBone) || rootBone == 0)
            return;

        Vector3 currentRoot;
        if (!TransformUtils::GetNodePosition(rootBone, currentRoot))
            return;

        if (s_originalRoots.find(target->Address) == s_originalRoots.end())
            s_originalRoots[target->Address] = currentRoot;

        const Vector3 originalRoot = s_originalRoots[target->Address];

        Vector3 aimTarget = GetAimPoint(*target);
        if (aimTarget == Vector3::Zero())
            return;

        const Vector3 rootOffsetFromAim = currentRoot - aimTarget;

        Vector3 fwd(g_Globals.EspConfig.ViewMatrix.m02,
            g_Globals.EspConfig.ViewMatrix.m12,
            g_Globals.EspConfig.ViewMatrix.m22);
        float len = sqrtf(fwd.X * fwd.X + fwd.Y * fwd.Y + fwd.Z * fwd.Z);
        if (len < 1e-4f)
            return;
        fwd.X /= len;
        fwd.Y /= len;
        fwd.Z /= len;

        const Vector3 cam = g_Globals.EspConfig.MainCamera;
        Vector3 toAim = aimTarget - cam;
        float proj = fwd.X * toAim.X + fwd.Y * toAim.Y + fwd.Z * toAim.Z;

        if (proj < 2.5f) proj = 2.5f;

        Vector3 aimOnLine = cam + fwd * proj;
        aimOnLine.Y -= 0.010f;

        Vector3 desiredRoot = aimOnLine + rootOffsetFromAim;

        uint32_t t1 = 0;
        if (Mem.Read(rootBone + 0x8, t1) && t1 != 0) {
            uint32_t t2 = 0;
            if (Mem.Read(t1 + 0x8, t2) && t2 != 0) {
                uint32_t visualState = 0;
                if (Mem.Read(t2 + 0x20, visualState) && visualState != 0) {
                    for (int i = 0; i < 4; i++) {
                        Mem.Write(visualState + 0x60, desiredRoot);
                    }
                } else {
                    for (int i = 0; i < 4; i++) {
                        WriteRootPosition(target->Address, desiredRoot);
                    }
                }
            } else {
                for (int i = 0; i < 4; i++) {
                    WriteRootPosition(target->Address, desiredRoot);
                }
            }
        } else {
            for (int i = 0; i < 4; i++) {
                WriteRootPosition(target->Address, desiredRoot);
            }
        }

        Vector3 right(g_Globals.EspConfig.ViewMatrix.m00,
            g_Globals.EspConfig.ViewMatrix.m10,
            g_Globals.EspConfig.ViewMatrix.m20);
        float rlen = sqrtf(right.X * right.X + right.Y * right.Y + right.Z * right.Z);
        if (rlen > 1e-4f) {
            right.X /= rlen; right.Y /= rlen; right.Z /= rlen;
        }

        Vector3 pullVec = desiredRoot - originalRoot;
        float pFwd = pullVec.X * fwd.X + pullVec.Y * fwd.Y + pullVec.Z * fwd.Z;
        float pRight = pullVec.X * right.X + pullVec.Y * right.Y + pullVec.Z * right.Z;

        pFwd = std::max(-20.0f, std::min(20.0f, pFwd));
        pRight = std::max(-4.7f, std::min(4.7f, pRight));

        pullVec.X = right.X * pRight + fwd.X * pFwd;
        pullVec.Z = right.Z * pRight + fwd.Z * pFwd;

        pullVec.Y = std::max(-0.55f, std::min(0.55f, pullVec.Y));

        desiredRoot = originalRoot + pullVec;

        for (int i = 0; i < 4; i++) {
            WriteRootPosition(target->Address, desiredRoot);
        }
    }

    // ---- Auto Fire ----
    static void UpdateAutoFire()
    {
        if (!g_Globals.AimBot.AutoFire) return;
        if (!g_Globals.EspConfig.Matrix || g_Globals.EspConfig.LocalPlayer == 0) return;
        if (g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0) return;

        const float radius = g_Globals.AimBot.AutoFireRadius;
        const ImVec2 screenCenter(
            static_cast<float>(g_Globals.EspConfig.Width) / 2.0f,
            static_cast<float>(g_Globals.EspConfig.Height) / 2.0f);

        bool shouldFire = false;

        {
            SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
            for (auto& pair : g_Globals.EspConfig.Entities) {
                const Player& entity = pair.second;

                if (entity.Address == g_Globals.EspConfig.LocalPlayer) continue;
                if (entity.IsTeam == Player::Bool3::True) continue;
                if (entity.IsDead) continue;
                if (g_Globals.AimBot.IgnoreKnocked && entity.IsKnocked) continue;
                if (!entity.IsKnown) continue;
                if (!entity.IsVisible) continue;

                ImVec2 headScreen = W2S::WorldToScreenImVec2(
                    g_Globals.EspConfig.ViewMatrix,
                    entity.Head,
                    g_Globals.EspConfig.Width,
                    g_Globals.EspConfig.Height);

                if (headScreen.x <= 0.0f || headScreen.y <= 0.0f) continue;

                const float dx = headScreen.x - screenCenter.x;
                const float dy = headScreen.y - screenCenter.y;

                if (fabsf(dx) <= radius && fabsf(dy) <= radius) {
                    shouldFire = true;
                    break;
                }
            }
        }

        if (shouldFire) {
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        }
    }

    void Update() {
        if (g_Globals.EspConfig.LocalPlayer) {
            divekill();
            Tick();
            UpdateAutoFire();
        }
        MovementExploits::Update();
        BackJump::Run();
        if (g_Globals.Exploits.UndergroundEnabled)
            Underground::Work();
        else
            Underground::Stop();
        TelaParada::Run();
        PullEnemy::Run();
        if (g_Globals.Exploits.EnemyDown)
            EnemyDown::Work();
        else
            EnemyDown::Stop();
        if (g_Globals.Exploits.RapidFly)
            RapidFly::Start();
        else
            RapidFly::Stop();
        SpeedHack::Run();
    }

} // namespace ForceAim
} // namespace FWork
