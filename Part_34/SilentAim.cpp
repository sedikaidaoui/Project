#include <EspLines/Aimbot/SilentAim.hpp>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <random>
#include <thread>

namespace Aim {

std::atomic<bool> SilentAim::running{ false };
std::thread SilentAim::workerThread;
std::atomic<bool> SilentAim::silentModeRunning{ false };
std::thread SilentAim::silentModeThread;
std::atomic<bool> SilentAim::aimLockRunning{ false };
std::thread SilentAim::aimLockThread;

static std::mt19937 s_silentRng(std::random_device{}());
static std::uniform_real_distribution<float> s_jitterDist(-0.5f, 0.5f);

// NEW: Silent Mode Work Thread
void SilentAim::SilentModeWork()
{
    while (silentModeRunning) {
        try {
            if (!g_Globals.AimBot.SilentMode || !g_Globals.AimBot.SilentAimEnabled) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            if (g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0 ||
                !g_Globals.EspConfig.Matrix || g_Globals.EspConfig.LocalPlayer == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            // Check silent bind
            if (g_Globals.AimBot.SilentAimBind != 0 &&
                (GetAsyncKeyState(g_Globals.AimBot.SilentAimBind) & 0x8000) == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            // Find target
            Player target{};
            if (FindTarget(target)) {
                SilentModePushFrame(target);
            }

            std::this_thread::yield();
        }
        catch (...) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

// NEW: Silent Mode Push Frame Update
void SilentAim::SilentModePushFrame(const Player& target)
{
    if (Offsets::SilentLegitIsFiring == 0 || Offsets::SilentLegitWeaponInfo == 0)
        return;

    uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;
    if (!localPlayer) return;

    bool isFiring = Mem.ReadS<bool>(localPlayer + Offsets::SilentLegitIsFiring);
    if (!isFiring) return;

    uint32_t weaponData = Mem.ReadS<uint32_t>(localPlayer + Offsets::SilentLegitWeaponInfo);
    if (weaponData == 0) return;

    Vector3 startPos = Mem.ReadS<Vector3>(weaponData + Offsets::SilentLegitGunTip);

    // Silent Method / Mode selection
    Vector3 selectedBonePos;
    if (g_Globals.AimBot.SilentAimMode == 2) {
        // Body mode selected in UI:
        if (g_Globals.AimBot.SilentBodyBone == 1)
            selectedBonePos = (target.Neck != Vector3::Zero()) ? target.Neck : target.Hip;
        else
            selectedBonePos = (target.Hip != Vector3::Zero()) ? target.Hip : target.Root;
    } else {
        switch (g_Globals.AimBot.SilentMethod) {
        case 1: // Head
            selectedBonePos = target.Head;
            break;
        case 2: // Both
            selectedBonePos = target.Head;
            break;
        case 0: // Body
        default:
            selectedBonePos = (g_Globals.AimBot.AimbotChest) ? target.Hip : target.Head;
            break;
        }
    }

    if (selectedBonePos == Vector3(0, 0, 0)) return;

    float jitterStrength = g_Globals.AimBot.SilentJitter * g_Globals.AimBot.SilentSmoothing;
    Vector3 finalPosition = selectedBonePos + Vector3(
        s_jitterDist(s_silentRng) * jitterStrength,
        s_jitterDist(s_silentRng) * jitterStrength,
        s_jitterDist(s_silentRng) * jitterStrength
    );

    Vector3 delta = finalPosition - startPos;
    Mem.Write<Vector3>(weaponData + Offsets::SilentLegitBulletHit, delta);
}

// NEW: AIMLOCK Work Thread
void SilentAim::AimLockWork()
{
    while (aimLockRunning) {
        try {
            if (!g_Globals.AimBot.AimLockToggle) {
                Aim::SilentAim::ResetAimLockState();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            if (g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0 ||
                !g_Globals.EspConfig.Matrix || g_Globals.EspConfig.LocalPlayer == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            // Find best target for AIMLOCK
            Player* bestTarget = nullptr;
            float closestDist = FLT_MAX;
            Vector2 screenCenter(g_Globals.EspConfig.Width / 2.0f, g_Globals.EspConfig.Height / 2.0f);

            {
                SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
                for (auto& pair : g_Globals.EspConfig.Entities) {
                    Player& entity = pair.second;
                    if (entity.IsDead || entity.IsKnocked) continue;
                    if (g_Globals.AimBot.IgnoreBots && entity.IsBot) continue;
                    if (entity.Address == g_Globals.EspConfig.LocalPlayer) continue;
                    if (g_Globals.AimBot.AimLockOnlyVisible && !entity.IsVisible) continue;

                    Vector3 bone = (g_Globals.AimBot.AimLockBone == 1) ? entity.Hip :
                                   (g_Globals.AimBot.AimLockBone == 2) ? entity.Neck : entity.Head;
                    ImVec2 screenPos = W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, bone, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
                    if (screenPos.x < 1 || screenPos.y < 1) continue;

                    float dist = Vector3::Distance(g_Globals.EspConfig.MainCamera, bone);
                    if (dist > g_Globals.AimBot.AimLockMaxDistance) continue;

                    float crosshairDist = std::sqrt(std::pow(screenPos.x - screenCenter.X, 2) + std::pow(screenPos.y - screenCenter.Y, 2));
                    float fovRange = (float)(g_Globals.AimBot.AimLockFovEnable ? g_Globals.AimBot.AimLockFov : 9999);
                    if (crosshairDist > fovRange) continue;

                    if (crosshairDist < closestDist) {
                        closestDist = crosshairDist;
                        bestTarget = &entity;
                    }
                }
            }

            if (bestTarget) {
                AimLockPushFrame(*bestTarget);
            }

            std::this_thread::yield();
        }
        catch (...) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

// NEW: AIMLOCK Push Frame
void SilentAim::AimLockPushFrame(const Player& target)
{
    if (Offsets::SilentLegitIsFiring == 0 || Offsets::SilentLegitWeaponInfo == 0)
        return;

    uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;
    if (!localPlayer) return;

    bool isFiring = Mem.ReadS<bool>(localPlayer + Offsets::SilentLegitIsFiring);
    if (!isFiring) return;

    uint32_t weaponData = Mem.ReadS<uint32_t>(localPlayer + Offsets::SilentLegitWeaponInfo);
    if (weaponData == 0) return;

    Vector3 startPos = Mem.ReadS<Vector3>(weaponData + Offsets::SilentLegitGunTip);

    Vector3 bone = (g_Globals.AimBot.AimLockBone == 1) ? target.Hip :
                   (g_Globals.AimBot.AimLockBone == 2) ? target.Neck : target.Head;
    if (bone == Vector3(0, 0, 0)) return;

    Vector3 delta = bone - startPos;
    Mem.Write<Vector3>(weaponData + Offsets::SilentLegitBulletHit, delta);
}

// NEW: Reset AIMLOCK state
void SilentAim::ResetAimLockState()
{
    // Reset aim lock values
    if (Offsets::SilentLegitIsFiring != 0 && Offsets::SilentLegitWeaponInfo != 0) {
        uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;
        if (localPlayer) {
            uint32_t weaponData = Mem.ReadS<uint32_t>(localPlayer + Offsets::SilentLegitWeaponInfo);
            if (weaponData != 0) {
                Mem.Write<Vector3>(weaponData + Offsets::SilentLegitBulletHit, Vector3(0, 0, 0));
            }
        }
    }
}

bool SilentAim::FindTarget(Player& outTarget)
{
    SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
    Player* best = nullptr;
    float closestDist = FLT_MAX;
    ImVec2 screenCenter(g_Globals.EspConfig.Width * 0.5f, g_Globals.EspConfig.Height * 0.5f);

    for (auto& pair : g_Globals.EspConfig.Entities) {
        Player& entity = pair.second;
        if (entity.IsDead || entity.IsKnocked || entity.IsTeam == Player::Bool3::True) continue;
        if (g_Globals.AimBot.IgnoreBots && entity.IsBot) continue;
        if (entity.Address == g_Globals.EspConfig.LocalPlayer) continue;
        if (!entity.IsVisible) continue;

        Vector3 bone;
        if (g_Globals.AimBot.SilentAimMode == 2) {
            bone = (g_Globals.AimBot.SilentBodyBone == 1 && entity.Neck != Vector3::Zero()) ? entity.Neck : 
                   (entity.Hip != Vector3::Zero()) ? entity.Hip : entity.Root;
        } else {
            bone = (g_Globals.AimBot.AimbotChest) ? entity.Hip : entity.Head;
        }
        if (bone == Vector3::Zero()) continue;

        ImVec2 screenPos = W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, bone, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
        if (screenPos.x < 1.0f || screenPos.y < 1.0f) continue;

        float dist = Vector3::Distance(g_Globals.EspConfig.MainCamera, bone);
        if (dist > (float)g_Globals.AimBot.DistanceAim) continue;

        float dx = screenPos.x - screenCenter.x;
        float dy = screenPos.y - screenCenter.y;
        float crosshairDist = std::sqrt(dx * dx + dy * dy);
        if (crosshairDist > (float)g_Globals.AimBot.Fov) continue;

        if (crosshairDist < closestDist) {
            closestDist = crosshairDist;
            best = &entity;
        }
    }

    if (best) {
        outTarget = *best;
        return true;
    }
    return false;
}

void SilentAim::PushFrameUpdate(const Player& target)
{
    SilentModePushFrame(target);
}

void SilentAim::Work()
{
    while (running) {
        try {
            // Work handles Standard (0) and Body (2) modes
            if (!g_Globals.AimBot.SilentAimEnabled ||
                (g_Globals.AimBot.SilentAimMode != 0 && g_Globals.AimBot.SilentAimMode != 2) ||
                (g_Globals.AimBot.SilentAimBind != 0 &&
                 (GetAsyncKeyState(g_Globals.AimBot.SilentAimBind) & 0x8000) == 0)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            if (g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0 ||
                !g_Globals.EspConfig.Matrix || g_Globals.EspConfig.LocalPlayer == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            Player target{};
            if (FindTarget(target)) {
                PushFrameUpdate(target);
            }

            std::this_thread::yield();
        }
        catch (...) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void SilentAim::Start()
{
    if (running) return;
    running = true;
    workerThread = std::thread([] {
        Work();
    });
    workerThread.detach();

    // NEW: Start Silent Mode thread
    silentModeRunning = true;
    silentModeThread = std::thread([] {
        SilentModeWork();
    });
    silentModeThread.detach();

    // NEW: Start AIMLOCK thread
    aimLockRunning = true;
    aimLockThread = std::thread([] {
        AimLockWork();
    });
    aimLockThread.detach();
}

void SilentAim::Stop()
{
    running = false;
    silentModeRunning = false;
    aimLockRunning = false;
    if (silentModeThread.joinable()) silentModeThread.join();
    if (aimLockThread.joinable()) aimLockThread.join();
}

} // namespace Aim
