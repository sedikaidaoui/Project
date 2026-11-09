// ============================================================================
//  Aim RAGE - EliteXzp AimbotPullThreadFunc port
//  ("aimbot rage by @EliteXzp!")
//
//  Flow: worker thread ~1ms -> bone pull -> LookRotation -> Slerp ->
//  write AimRotation + Prev/Backup + m_SwapWeaponTime + network ShadowState
//  (PlayerNetwork 0x1A60, StartRot 0x34 / TargetRot 0x44).
//
//  Visible check uses the local-player aim-assist lists (0x420 / 0x43C); it is
//  skipped when "Track Through Walls" (RageWallTrack) is enabled.
//
//  Thread safety: the entity map is only walked on the render thread
//  (Data::Work -> Process -> FrameScan); every lookup takes EntitiesLock.
// ============================================================================
#include <imgui.h>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Math/Quaternion.hpp>
#include <EspLines/Aimbot/RageAimbot.hpp>
#include <src/Globals.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <atomic>
#include <thread>
#include <chrono>
#include <cmath>
#include <vector>
#include <utility>
#include <algorithm>
#include <cfloat>

namespace Aim {

// ---- class statics ----
Player RageAimbot::RageTarget;
bool RageAimbot::HasRageTarget = false;
bool RageAimbot::RageAimActive = false;
bool RageAimbot::RotationApplied = false;
std::chrono::steady_clock::time_point RageAimbot::FireKeyPressTime{};

// ============================================================================
//  SHIMS - the method keeps its original names/semantics
// ============================================================================
template<typename T> static inline T Ler(uintptr_t a) { return Mem::Read<T>(a); }
template<typename T> static inline void Escrever(uintptr_t a, const T& v) { Mem::Write<T>(a, v); }

// state of the method (same names as the reference source)
static std::atomic<bool>      AimbotPullRunning{ false };
static std::atomic<bool>      AimbotPullExited{ true };   // worker reached its shutdown path
static bool                   AimbotPullEnabled = false;    // <- AimBot.Rage
static bool                   isMagnetToggled = false;      // <- Exploits.MagnetLite
static std::atomic<bool>      g_MatchStateLive{ false };    // <- EspConfig.InMatch
static uintptr_t              g_AimbotPullLocalPlayer = 0;  // <- EspConfig.LocalPlayer
static uintptr_t              g_AimbotPullTarget = 0;       // <- RAGE SCAN
static bool                   VisibleCheck1 = false;        // <- !AimBot.RageWallTrack
static bool                   AimbotKillCover = false;      // <- AimBot.RageWallTrack
static bool                   SyncFall = false;             // this project has no fall clamp
static int                    AimbotPullBoneTarget = 0;     // 0 = head, 1 = neck
static float                  AimbotPullSmooth = 5.0f;      // <- AimBot.RageTrackSpeed
static std::atomic<bool>      g_MagnetV2ReadyForAimbot{ true };
static std::atomic<ULONGLONG> g_MagnetV2ReadyTick{ 1 };
static constexpr ULONGLONG    kMagnetV2AimbotDelayMs = 300;

static bool IsSaneGamePtr(uintptr_t p) { return p > 0x10000 && p < 0xFFFFFFFFull; }

// snapshot cache (Entities) -> bone position from the cache
static Vector3 RageEntityBone(uintptr_t addr, bool neck) {
    Vector3 p{};
    if (!addr) return p;
    SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
    auto it = g_Globals.EspConfig.Entities.find((long)addr);
    if (it == g_Globals.EspConfig.Entities.end()) return p;
    p = neck ? it->second.Neck : it->second.Head;
    if (p.X == 0.0f && p.Y == 0.0f) p = it->second.Head;
    return p;
}

static Vector3 GetHeadPosition(uintptr_t addr) {
    if (addr && addr == (uintptr_t)g_Globals.EspConfig.LocalPlayer)
        return g_Globals.EspConfig.MainCamera;   // local = camera position (eye)
    return RageEntityBone(addr, false);
}

static Vector3 GetPlayerPosition(uintptr_t addr, int) {
    if (addr && addr == (uintptr_t)g_Globals.EspConfig.LocalPlayer)
        return g_Globals.EspConfig.MainCamera;
    Vector3 p{};
    if (!addr) return p;
    SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
    auto it = g_Globals.EspConfig.Entities.find((long)addr);
    if (it != g_Globals.EspConfig.Entities.end()) p = it->second.Root;
    return p;
}

// bone ids 0x14/0x18 of the old project -> snapshot cache (Neck -> Head)
static Vector3 ObterOssos(uintptr_t addr, int) { return RageEntityBone(addr, true); }

// camera transform -> cached world position
static Vector3 Transform$$ObterPosicao(uintptr_t) { return g_Globals.EspConfig.MainCamera; }

static bool IsLiveMatchEntity(uintptr_t addr) {
    if (!addr) return false;
    if (addr == (uintptr_t)g_Globals.EspConfig.LocalPlayer) return g_Globals.EspConfig.InMatch;
    SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
    auto it = g_Globals.EspConfig.Entities.find((long)addr);
    return it != g_Globals.EspConfig.Entities.end() && it->second.IsKnown && !it->second.IsDead;
}

static bool AimbotShouldActivate(uintptr_t) { return Aim::RageAimbot::IsKeyActive(); }

static bool IsValidQuaternion_AP(const Quaternion& q) {
    if (!std::isfinite(q.X) || !std::isfinite(q.Y) || !std::isfinite(q.Z) || !std::isfinite(q.W))
        return false;
    const float n = q.X * q.X + q.Y * q.Y + q.Z * q.Z + q.W * q.W;
    return n > 0.5f && n < 1.5f;
}

// ============================================================================
//  RAGE SCAN
// ============================================================================
static float    AimbotPullFov        = 600.0f;  // screen radius in pixels
static bool     isPriorityHacker_    = false;   // priority-hacker mode (default off)
static bool     isPriorityHacker     = false;
static float    ClosestDistPull      = 0.0f;
static uint32_t ClosestEnemyPull     = 0;
static float    HackerPullAliveDist  = 0.0f;
static uint32_t HackerPullAlive      = 0;
static float    HackerPullDyingDist  = 0.0f;
static uint32_t HackerPullDying      = 0;
static float    killCoverBestScreenDist = 0.0f;
static uint32_t killCoverBestTarget    = 0;
static bool     killCoverCurrentSeen   = false;
static float    killCoverCurrentScreenDist = 0.0f;
static uint32_t s_kc_target            = 0;
static uint32_t gAimbotPullTarget      = 0; // sticky check for the current frame

static uintptr_t GuestPtr(uintptr_t p) { return IsSaneGamePtr(p) ? p : 0; }

void RageAimbot::BeginScan() {
    ClosestDistPull      = FLT_MAX;
    ClosestEnemyPull     = 0;
    HackerPullAliveDist  = FLT_MAX;
    HackerPullAlive      = 0;
    HackerPullDyingDist  = FLT_MAX;
    HackerPullDying      = 0;
    killCoverBestScreenDist = FLT_MAX;
    killCoverBestTarget    = 0;
    killCoverCurrentSeen   = false;
    killCoverCurrentScreenDist = FLT_MAX;
}

void RageAimbot::ScanEntity(uint32_t Entity, const Vector3& PosHeadInScr, bool isDying) {
    const float sw = (float)g_Globals.EspConfig.Width;
    const float sh = (float)g_Globals.EspConfig.Height;
    const uintptr_t JogadorLocal = (uintptr_t)g_Globals.EspConfig.LocalPlayer;

    if (AimbotPullEnabled) {
        float cx = sw / 2.0f, cy = sh / 2.0f;
        float dx = PosHeadInScr.X - cx, dy = PosHeadInScr.Y - cy;
        float fovLim2 = isPriorityHacker_ ? FLT_MAX : (AimbotPullFov * AimbotPullFov);
        if ((dx * dx + dy * dy) <= fovLim2) {
            float dp = std::sqrt(dx * dx + dy * dy);
            if (AimbotShouldActivate(JogadorLocal) && g_AimbotPullTarget != 0 && Entity == gAimbotPullTarget)
                dp -= 10000.0f;
            if (isPriorityHacker) {
                if (!isDying) { if (dp < HackerPullAliveDist) { HackerPullAliveDist = dp; HackerPullAlive = Entity; } }
                else { if (dp < HackerPullDyingDist) { HackerPullDyingDist = dp; HackerPullDying = Entity; } }
            }
            if (!isDying && dp < ClosestDistPull) { ClosestDistPull = dp; ClosestEnemyPull = Entity; }
        }
    }

    if (!isDying && AimbotKillCover) {
        float cx = sw * 0.5f, cy = sh * 0.5f;
        float dxC = PosHeadInScr.X - cx, dyC = PosHeadInScr.Y - cy;
        float sd = std::sqrt(dxC * dxC + dyC * dyC);
        if (sd <= AimbotPullFov) {
            if (sd < killCoverBestScreenDist) { killCoverBestScreenDist = sd; killCoverBestTarget = Entity; }
            if (Entity == s_kc_target) { killCoverCurrentSeen = true; killCoverCurrentScreenDist = sd; }
        }
    }
}

void RageAimbot::EndScan() {
    const uintptr_t JogadorLocal = (uintptr_t)g_Globals.EspConfig.LocalPlayer;

    uint32_t aimbotTarget = 0;
    if (isPriorityHacker) aimbotTarget = HackerPullAlive;
    if (aimbotTarget == 0) aimbotTarget = ClosestEnemyPull;

    const bool isMagnetActive = isMagnetToggled;
    const bool magnetV2Wait = isMagnetToggled &&
        !(g_MagnetV2ReadyForAimbot.load(std::memory_order_relaxed) &&
          GetTickCount64() >= g_MagnetV2ReadyTick.load(std::memory_order_relaxed));

    if (AimbotPullEnabled && aimbotTarget != 0 && JogadorLocal != 0 && !isMagnetActive
        && !magnetV2Wait && IsLiveMatchEntity(JogadorLocal) && IsLiveMatchEntity(aimbotTarget)) {
        g_AimbotPullLocalPlayer = GuestPtr(JogadorLocal);
        g_AimbotPullTarget = GuestPtr(aimbotTarget);
    } else {
        g_AimbotPullLocalPlayer = 0;
        g_AimbotPullTarget = 0;
    }

    gAimbotPullTarget = (uint32_t)g_AimbotPullTarget;   // sticky check for the next frame
    s_kc_target       = (uint32_t)g_AimbotPullTarget;   // kill-cover current target
}

// Scan independent from the ESP render path (Data::Work runs it every tick).
void RageAimbot::FrameScan() {
    if (!g_Globals.EspConfig.Matrix) return;

    BeginScan();
    {
        SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
        for (const auto& [id, p] : g_Globals.EspConfig.Entities) {
            const ImVec2 phHead = W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, p.Head,
                g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
            ScanEntity((uint32_t)id, Vector3(phHead.x, phHead.y, 0.0f), p.IsDead || p.IsKnocked);
        }
    }
    EndScan();
}

// ============================================================================
//  THE METHOD (by @EliteXzp, only the I/O adapted)
// ============================================================================
void AimbotPullThreadFunc() {
    uintptr_t cachedTarget = 0;
    Quaternion currentAimRot = Quaternion::Identity();
    bool hasCurrentAim = false;
    Vector3 lastBonePos = { 0, 0, 0 };
    Vector3 boneVelocity = { 0, 0, 0 };
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (AimbotPullRunning.load(std::memory_order_relaxed)) {
        if (!AimbotPullEnabled || isMagnetToggled ||
            !g_MatchStateLive.load(std::memory_order_relaxed)) {
            cachedTarget = 0; hasCurrentAim = false;
            lastBonePos = { 0, 0, 0 }; boneVelocity = { 0, 0, 0 };
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }

        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        if (dt > 1.0f) dt = 0.016f;
        if (dt <= 0.0f) dt = 0.001f;

        uintptr_t localPlayer = g_AimbotPullLocalPlayer;
        if (!IsSaneGamePtr(localPlayer) || !IsLiveMatchEntity(localPlayer)) {
            cachedTarget = 0; hasCurrentAim = false;
            lastBonePos = { 0, 0, 0 }; boneVelocity = { 0, 0, 0 };
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        static ULONGLONG lastActivateTime = 0;
        bool shouldActivate = false;

        if (AimbotShouldActivate(localPlayer)) {
            lastActivateTime = GetTickCount64();
        }
        if ((GetTickCount64() - lastActivateTime <= 240)) {
            shouldActivate = true;
        }

        if (!shouldActivate) {
            hasCurrentAim = false;
            lastBonePos = { 0, 0, 0 }; boneVelocity = { 0, 0, 0 };
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        // pull enemy: wait for the grab delay of Pull Enemy
        if (AimbotKillCover) {
            if (!g_MagnetV2ReadyForAimbot.load(std::memory_order_relaxed)) {
                hasCurrentAim = false;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            const ULONGLONG readyTick = g_MagnetV2ReadyTick.load(std::memory_order_relaxed);
            if (readyTick == 0 || (GetTickCount64() - readyTick) < kMagnetV2AimbotDelayMs) {
                hasCurrentAim = false;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
        }

        uintptr_t target = g_AimbotPullTarget;
        if (!IsSaneGamePtr(target) || !IsLiveMatchEntity(target)) {
            cachedTarget = 0; hasCurrentAim = false;
            lastBonePos = { 0, 0, 0 }; boneVelocity = { 0, 0, 0 };
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        if (VisibleCheck1) {
            bool isVisible = false;

            uint32_t aimAssist = Ler<uint32_t>(localPlayer + 0x420);
            if (aimAssist && aimAssist != 0xFFFFFFFF) {
                uint32_t targetInfo = Ler<uint32_t>(aimAssist + 0xC);
                if (targetInfo && targetInfo != 0xFFFFFFFF)
                    isVisible = true;
            }

            if (!isVisible) {
                uint32_t aimAssistSighting = Ler<uint32_t>(localPlayer + 0x43C);
                if (aimAssistSighting && aimAssistSighting != 0xFFFFFFFF) {
                    uint32_t targetInfo = Ler<uint32_t>(aimAssistSighting + 0xC);
                    if (targetInfo && targetInfo != 0xFFFFFFFF)
                        isVisible = true;
                }
            }

            if (!isVisible) {
                hasCurrentAim = false;
                lastBonePos = { 0, 0, 0 };
                boneVelocity = { 0, 0, 0 };
                continue;
            }
        }

        if (target != cachedTarget) {
            cachedTarget = target;
            lastBonePos = { 0, 0, 0 }; boneVelocity = { 0, 0, 0 };
            hasCurrentAim = false;
        }

        bool isFemale = Ler<bool>(target + Offsets::isfemale);
        Vector3 bonePos = { 0, 0, 0 };

        int boneTarget = AimbotPullBoneTarget;
        if (boneTarget == 0) {
            bonePos = GetHeadPosition(target);
        } else {
            bonePos = ObterOssos(target, isFemale ? 0x18 : 0x14);
        }

        if (bonePos.X == 0.0f && bonePos.Y == 0.0f) {
            bonePos = GetHeadPosition(target);
        }

        if (bonePos.X == 0.0f && bonePos.Y == 0.0f) {
            bonePos = GetPlayerPosition(target, 0);
            if (bonePos.X != 0.0f || bonePos.Z != 0.0f) {
                bonePos.Y += (boneTarget == 0) ? 1.6f : 1.2f;
            }
        }

        if (bonePos.X == 0.0f && bonePos.Y == 0.0f) {
            uintptr_t shadowState = Ler<uintptr_t>(target + Offsets::PlayerNetwork_HHCBNAPCKHF);
            if (shadowState > 0xFFFF) {
                bonePos = Ler<Vector3>(shadowState + Offsets::ShadowState_TargetPos);
                if (bonePos.X != 0.0f || bonePos.Z != 0.0f) {
                    bonePos.Y += (boneTarget == 0) ? 1.6f : 1.2f;
                }
            }
        }

        if (bonePos.X == 0.0f && bonePos.Y == 0.0f) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        if (!AimbotKillCover && boneTarget == 0) {
            bonePos.Y -= 0.03f;
        }

        if (lastBonePos.X != 0.0f || lastBonePos.Y != 0.0f) {
            boneVelocity.X = bonePos.X - lastBonePos.X;
            boneVelocity.Y = bonePos.Y - lastBonePos.Y;
            boneVelocity.Z = bonePos.Z - lastBonePos.Z;

            float velDist = std::sqrt(boneVelocity.X * boneVelocity.X + boneVelocity.Y * boneVelocity.Y + boneVelocity.Z * boneVelocity.Z);
            if (velDist > 2.0f) {
                boneVelocity = { 0, 0, 0 };
            }
        }
        lastBonePos = bonePos;

        Vector3 predictedPos = bonePos;
        if (!AimbotKillCover) {
            predictedPos = {
                bonePos.X + boneVelocity.X * 0.5f,
                bonePos.Y + boneVelocity.Y * 0.5f,
                bonePos.Z + boneVelocity.Z * 0.5f
            };
        }

        Vector3 camPos = { 0, 0, 0 };
        uintptr_t cameraComp = Ler<uintptr_t>(localPlayer + Offsets::MainCamTransform);
        if (cameraComp > 0x10000) {
            camPos = Transform$$ObterPosicao(cameraComp);
        }
        if (camPos.X == 0.0f && camPos.Y == 0.0f && camPos.Z == 0.0f) {
            camPos = GetHeadPosition(localPlayer);
        }
        if (camPos.X == 0.0f && camPos.Y == 0.0f && camPos.Z == 0.0f) {
            camPos = GetPlayerPosition(localPlayer, 0);
            camPos.Y += 1.6f;
        }
        if (camPos.X == 0.0f && camPos.Y == 0.0f && camPos.Z == 0.0f) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        Vector3 dir = Vector3::Normalized(predictedPos - camPos);
        if (Vector3::SqrMagnitude(dir) < 0.001f) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        if (SyncFall) {
            dir.Y = std::clamp(dir.Y, -0.65f, 0.35f);
            dir = Vector3::Normalized(dir);
        }

        Vector3 safeUp = (std::abs(dir.Y) > 0.97f) ? Vector3(0, 0, 1) : Vector3(0, 1, 0);
        Quaternion targetRot = Quaternion::LookRotation(dir, safeUp);
        Quaternion aimRot;

        if (!AimbotKillCover && AimbotPullSmooth > 1.0f && hasCurrentAim) {
            const float rate = 60.0f / AimbotPullSmooth;
            float t = 1.0f - std::exp(-rate * dt);
            if (t > 1.0f) t = 1.0f;
            if (t < 0.0001f) t = 0.0001f;
            aimRot = Quaternion::Slerp(currentAimRot, targetRot, t);
        } else {
            aimRot = targetRot;
        }

        aimRot = Quaternion::Normalized(aimRot);
        currentAimRot = aimRot;
        hasCurrentAim = true;

        if (!IsValidQuaternion_AP(aimRot)) {
            hasCurrentAim = false;
            currentAimRot = Quaternion::Identity();
            lastBonePos = { 0, 0, 0 }; boneVelocity = { 0, 0, 0 };
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        if (!g_MatchStateLive.load(std::memory_order_relaxed) ||
            !IsSaneGamePtr(localPlayer) || !IsLiveMatchEntity(localPlayer)) {
            hasCurrentAim = false;
            lastBonePos = { 0, 0, 0 }; boneVelocity = { 0, 0, 0 };
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        __try {
            uintptr_t net = Ler<uintptr_t>(localPlayer + Offsets::PlayerNetwork_HHCBNAPCKHF);
            for (int k = 0; k < 4; k++) {
                if (!g_MatchStateLive.load(std::memory_order_relaxed)) break;

                uint32_t aimCount = Ler<uint32_t>(localPlayer + Offsets::m_SwapWeaponTime);
                Escrever<uint32_t>(localPlayer + Offsets::m_SwapWeaponTime, aimCount + 1);

                Escrever<Quaternion>(localPlayer + Offsets::AimRotation, aimRot);
                Escrever<Quaternion>(localPlayer + Offsets::AimRotationPrev, aimRot);
                Escrever<Quaternion>(localPlayer + Offsets::AimRotationBackup, aimRot);

                if (net > 0x10000) {
                    Escrever<Quaternion>(net + Offsets::ShadowState_StartRot, aimRot);
                    Escrever<Quaternion>(net + Offsets::ShadowState_TargetRot, aimRot);
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            hasCurrentAim = false;
            lastBonePos = { 0, 0, 0 }; boneVelocity = { 0, 0, 0 };
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// ============================================================================
//  Target selection
// ============================================================================
bool RageAimbot::IsKeyActive() {
    // The reference pulls while the fire button is held.
    return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
}

Vector3 RageAimbot::GetHitbox(const Player& entity) {
    switch (g_Globals.AimBot.RealRagePriority) {
    case 1: return entity.Neck;
    case 2: return entity.LeftShoulder;
    case 3: return entity.RightShoulder;
    case 0:
    default: return entity.Head;
    }
}

Vector3 RageAimbot::GetBestMultipointTargetPoint(const Player& entity) {
    Vector3 baseBone = GetHitbox(entity);

    const float offset = 0.06f;
    Vector3 multipointNodes[5] = {
        baseBone,
        Vector3(baseBone.X, baseBone.Y + offset, baseBone.Z),
        Vector3(baseBone.X - offset, baseBone.Y, baseBone.Z),
        Vector3(baseBone.X + offset, baseBone.Y, baseBone.Z),
        Vector3(baseBone.X, baseBone.Y - offset, baseBone.Z)
    };

    float centerX = g_Globals.EspConfig.Width * 0.5f;
    float centerY = g_Globals.EspConfig.Height * 0.5f;
    float fovLimit = 8.0f * 10.0f;

    Vector3 bestPoint = baseBone;
    float bestDist = FLT_MAX;

    for (int i = 0; i < 5; i++) {
        ImVec2 sp = W2S::WorldToScreenImVec2(
            g_Globals.EspConfig.ViewMatrix, multipointNodes[i],
            g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);

        if (sp.x < 1.0f || sp.y < 1.0f) continue;
        if (!std::isfinite(sp.x) || !std::isfinite(sp.y)) continue;

        float dx = sp.x - centerX;
        float dy = sp.y - centerY;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist <= fovLimit && dist < bestDist) {
            bestDist = dist;
            bestPoint = multipointNodes[i];
        }
    }

    return bestPoint;
}

bool RageAimbot::FindBestTarget(Player& outTarget) {
    if (g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0 || !g_Globals.EspConfig.Matrix)
        return false;

    std::vector<std::pair<long, Player>> snapshot;
    {
        SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
        snapshot.reserve(g_Globals.EspConfig.Entities.size());
        for (const auto& pair : g_Globals.EspConfig.Entities)
            snapshot.emplace_back(pair.first, pair.second);
    }

    bool found = false;
    float bestMetric = FLT_MAX;
    float centerX = g_Globals.EspConfig.Width * 0.5f;
    float centerY = g_Globals.EspConfig.Height * 0.5f;

    // aim drift (human hand sway)
    {
        uint64_t dt2 = GetTickCount64();
        float ph = (float)(dt2 % 3000) / 3000.0f * 6.2831853f;
        float sx = ph;
        if (sx > 3.14159265f) sx -= 6.2831853f;
        float swayX = sx - (sx * sx * sx) / 6.0f;
        float ph2 = ph + 1.5707963f;
        float sy = ph2;
        if (sy > 3.14159265f) sy -= 6.2831853f;
        if (sy < -3.14159265f) sy += 6.2831853f;
        float swayY = sy - (sy * sy * sy) / 6.0f;
        centerX += swayX * 6.0f;
        centerY += swayY * 6.0f;
    }

    float fovRange = 100000.0f;
    float maxDistance = 100000.0f;

    Vector3 camPos = g_Globals.EspConfig.MainCamera;

    size_t scanStart = 0;
    if (!snapshot.empty()) {
        uint64_t st = GetTickCount64();
        scanStart = (size_t)(((st ^ (st >> 16)) * 0x9E3779B1ULL) % snapshot.size());
    }
    for (size_t si = 0; si < snapshot.size(); si++) {
        const Player& entity = snapshot[(scanStart + si) % snapshot.size()].second;

        if (entity.Address == 0 || entity.IsDead) continue;
        if (!entity.IsKnown) continue;
        if (entity.Address == g_Globals.EspConfig.LocalPlayer) continue;
        if (!g_Globals.AimBot.RageWallTrack && !entity.IsVisible) continue;

        Vector3 targetBone = GetBestMultipointTargetPoint(entity);
        if (targetBone == Vector3::Zero()) continue;
        if (!std::isfinite(targetBone.X) || !std::isfinite(targetBone.Y) || !std::isfinite(targetBone.Z)) continue;

        // bone plausibility: head-root 0.5..2.6m
        {
            Vector3 er = entity.Root;
            if (er == Vector3::Zero() ||
                !std::isfinite(er.X + er.Y + er.Z)) continue;
            float dyh = targetBone.Y - er.Y;
            if (dyh < 0.5f || dyh > 2.6f) continue;
            const Vector3& n = entity.Neck;
            if (n != Vector3::Zero()) {
                if (n.Y < er.Y || n.Y > targetBone.Y) continue;
                if (Vector3::Distance(targetBone, n) > 1.2f) continue;
                if (Vector3::Distance(n, er) > 1.8f) continue;
            }
            if (entity.LeftShoulder != Vector3::Zero() && entity.RightShoulder != Vector3::Zero()) {
                if (Vector3::Distance(entity.LeftShoulder, entity.RightShoulder) > 1.4f) continue;
            }
        }

        float playerDistance = Vector3::Distance(camPos, targetBone);
        if (!std::isfinite(playerDistance) || playerDistance > maxDistance) continue;

        ImVec2 screenPos = W2S::WorldToScreenImVec2(
            g_Globals.EspConfig.ViewMatrix, targetBone,
            g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);

        if (screenPos.x < 1.0f || screenPos.y < 1.0f) continue;
        if (!std::isfinite(screenPos.x) || !std::isfinite(screenPos.y)) continue;

        float dx = screenPos.x - centerX;
        float dy = screenPos.y - centerY;
        float crosshairDist = std::sqrt(dx * dx + dy * dy);
        if (!std::isfinite(crosshairDist) || crosshairDist > fovRange) continue;

        int priority = g_Globals.AimBot.RealRagePriority;
        float score;
        if (priority == 1) {
            score = (float)entity.Health;
        }
        else if (priority == 2) {
            score = playerDistance;
        }
        else {
            score = crosshairDist;
        }
        if (HasRageTarget && entity.Address == RageTarget.Address)
            score *= 0.75f;

        if (score < bestMetric) {
            bestMetric = score;
            outTarget = entity;
            found = true;
        }
    }

    return found;
}

// ============================================================================
//  Lifecycle: Process (Data::Work per frame) feeds the state, the worker thread
//  is started exactly once.
// ============================================================================
void RageAimbot::Process() {
    g_MatchStateLive.store(g_Globals.EspConfig.InMatch, std::memory_order_relaxed);
    AimbotPullEnabled = g_Globals.AimBot.Rage;
    VisibleCheck1     = !g_Globals.AimBot.RageWallTrack;
    AimbotKillCover   = g_Globals.AimBot.RageWallTrack;
    AimbotPullSmooth  = std::fmaxf(1.0f, g_Globals.AimBot.RageTrackSpeed);
    AimbotPullFov     = std::fmaxf(10.0f, g_Globals.AimBot.RageFov);
    AimbotPullBoneTarget = (g_Globals.AimBot.RageBone == 1) ? 1 : 0;
    isMagnetToggled   = g_Globals.Exploits.MagnetLite;
    g_AimbotPullLocalPlayer = (uintptr_t)g_Globals.EspConfig.LocalPlayer;

    // Pull Enemy ready bridge (300ms grab before the pull starts)
    static ULONGLONG magnetSince = 0;
    if (isMagnetToggled) {
        if (!magnetSince) magnetSince = GetTickCount64();
        if (GetTickCount64() >= magnetSince + kMagnetV2AimbotDelayMs) {
            g_MagnetV2ReadyTick.store(magnetSince + kMagnetV2AimbotDelayMs, std::memory_order_relaxed);
            g_MagnetV2ReadyForAimbot.store(true, std::memory_order_relaxed);
        }
    } else {
        magnetSince = 0;
    }

    if (AimbotPullEnabled && g_MatchStateLive.load(std::memory_order_relaxed) &&
        g_Globals.EspConfig.Matrix) {
        FrameScan();
    } else {
        g_AimbotPullTarget = 0;
        gAimbotPullTarget = 0;
        s_kc_target = 0;
    }

    RageAimActive = (g_AimbotPullTarget != 0);
    HasRageTarget = (g_AimbotPullTarget != 0);

    StartAimbot(); // idempotent: one thread only
}

void RageAimbot::StartAimbot() {
    static bool started = false;
    if (started) return;
    started = true;
    AimbotPullRunning.store(true, std::memory_order_relaxed);
    AimbotPullExited.store(false, std::memory_order_relaxed);
    std::thread([] {
        try {
            AimbotPullThreadFunc();
        } catch (...) {
            // never let an exception escape the worker: std::terminate would
            // kill the whole emulator process.
        }
        AimbotPullExited.store(true, std::memory_order_relaxed);
    }).detach();
}

void RageAimbot::StopAimbot() {
    AimbotPullRunning.store(false, std::memory_order_relaxed);
    RageAimActive = false;
    HasRageTarget = false;
    RotationApplied = false;
    g_AimbotPullTarget = 0;

    // The worker is detached: give it a bounded window to leave its loop so it
    // cannot touch this DLL after FreeLibraryAndExitThread().
    for (int i = 0; i < 100 && !AimbotPullExited.load(std::memory_order_relaxed); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

} // namespace Aim
