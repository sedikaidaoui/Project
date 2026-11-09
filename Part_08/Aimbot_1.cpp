#include <imgui.h>
#include <imgui_internal.h>
#include <EspLines\Memory\Memory.hpp>
#include <EspLines\Math\Vector\Vector3.hpp>
#include <EspLines\Math\Vector\Vector2.hpp>
#include <EspLines/Math/Quaternion.hpp>
#include <EspLines/Aimbot/Aimbot.hpp>
#include <EspLines/Aimbot/ExtremeAimbot.hpp>
#include <EspLines/Aimbot/AimbotLogic.h> 
#include <src/Globals.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <atomic>
#include <algorithm>
#include <random>

namespace Aim {

std::atomic<bool> Aimbot::aimbotRunning{ false };
std::thread Aimbot::aimbotThread;
static std::atomic<bool> rubixRunning{ false };
static std::thread rubixThread;
static uint32_t s_rubixJitterSeed = 0;

static inline uint32_t NextRubixJitter(uint32_t range) {
    if (range == 0) return 0;
    s_rubixJitterSeed = s_rubixJitterSeed * 1664525u + 1013904223u;
    return s_rubixJitterSeed % range;
}
bool Aimbot::lastKeyState = false;
Player* Aimbot::lastTarget = nullptr;
uintptr_t Aimbot::g_currentTargetAddr = 0;
static bool isPatched = false;
static uintptr_t lastLockedAddr = 0;
static uint32_t lastOriginalValue = 0;
static uintptr_t aimLockTargetAddr = 0;
static bool aimLockActive = false;
static int aimLockBone = 0;
static float aimLockTimer = 0.0f;

static std::chrono::steady_clock::time_point acquisitionStartTime;
static float currentReactionDelay = 0.0f;
static int shotsToBody = 0;

// Random engine for humanization
static std::mt19937 rng(std::random_device{}());

Vector3 Aimbot::GetHitBoxPosition(const Player& entity) {
    // 1. Nếu bật AimbotChest -> Luôn luôn nhắm vào Hip (Ngực/Hông)
    if (g_Globals.AimBot.AimbotChest) {
        return entity.Hip;
    }

    // 2. Nếu là Legit Aimbot -> Chạy logic Body to Head (3-5 viên người rồi lên đầu)
    if (entity.ShotsFiredAtTarget < shotsToBody) {
        return entity.Hip; 
    }
    return entity.Head;
}

bool Aimbot::FindClosestEnemy(Player& outTarget) {
    float closestDistance = FLT_MAX;
    bool found = false;

    Vector2 screenCenter(g_Globals.EspConfig.Width / 2.0f, g_Globals.EspConfig.Height / 2.0f);

    SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
    for (auto& pair : g_Globals.EspConfig.Entities) {
        Player* entity = &pair.second;

        if (entity->IsDead || (g_Globals.AimBot.IgnoreKnocked && entity->IsKnocked)) continue;
        
        // --- LEGIT VISIBILITY CHECK ---
        if (!entity->IsVisible) continue;

        Vector3 targetHitBox = GetHitBoxPosition(*entity);
        
        // Sticky target check
        if (g_currentTargetAddr != 0 && entity->Address == g_currentTargetAddr) {
            float distance = Vector3::Distance(g_Globals.EspConfig.MainCamera, targetHitBox);
            if (g_Globals.AimBot.AimbotChest || distance <= g_Globals.AimBot.DistanceAim) {
                outTarget = *entity;
                return true; 
            }
        }

        ImVec2 hitBox2D = W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, targetHitBox, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
        if (hitBox2D.x < 1 || hitBox2D.y < 1) continue;

        float distance = Vector3::Distance(g_Globals.EspConfig.MainCamera, targetHitBox);
        if (!g_Globals.AimBot.AimbotChest && distance > g_Globals.AimBot.DistanceAim) continue;

        float crosshairDist = std::sqrt(std::pow(hitBox2D.x - screenCenter.X, 2) + std::pow(hitBox2D.y - screenCenter.Y, 2));
        
        // FOV Check
        if (crosshairDist > g_Globals.AimBot.Fov) continue;

        if (crosshairDist >= closestDistance) continue;

        closestDistance = crosshairDist;
        outTarget = *entity;
        found = true;
    }

    return found;
}

bool Aimbot::FindRubixTarget(Player& outTarget) {
    float closestDistance = FLT_MAX;
    bool found = false;

    Vector2 screenCenter(g_Globals.EspConfig.Width / 2.0f, g_Globals.EspConfig.Height / 2.0f);

    SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
    for (auto& pair : g_Globals.EspConfig.Entities) {
        Player* entity = &pair.second;

        if (entity->IsDead || (g_Globals.AimBot.IgnoreKnocked && entity->IsKnocked)) continue;

        Vector3 targetHitBox = entity->Head;
        ImVec2 hitBox2D = W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, targetHitBox, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
        if (hitBox2D.x < 1 || hitBox2D.y < 1) continue;

        float distance = Vector3::Distance(g_Globals.EspConfig.MainCamera, targetHitBox);
        if (distance > g_Globals.AimBot.DistanceAim) continue;

        float crosshairDist = std::sqrt(std::pow(hitBox2D.x - screenCenter.X, 2) + std::pow(hitBox2D.y - screenCenter.Y, 2));
        if (crosshairDist > g_Globals.AimBot.Fov) continue;

        if (crosshairDist >= closestDistance) continue;

        closestDistance = crosshairDist;
        outTarget = *entity;
        std::this_thread::sleep_for(std::chrono::milliseconds((std::max)(0, static_cast<int>(g_Globals.AimBot.Smoothing))));
        found = true;
    }

    return found;
}

void Aimbot::LegitAimbot()
{
    static uintptr_t lastLockedAddr = 0;
    static bool isPatched = false;

    auto clearLock = [&]() {
        if (isPatched && lastLockedAddr != 0) {
            Mem.WriteStealth<uint32_t>(lastLockedAddr, 0); // Safe reset
            isPatched = false;
            lastLockedAddr = 0;
        }
    };

    if ((!g_Globals.AimBot.Enabled || g_Globals.AimBot.AimbotType != 0) && !g_Globals.AimBot.AimbotChest) {
        g_currentTargetAddr = 0;
        clearLock();
        return;
    }

    bool keyHeld = (GetAsyncKeyState(g_Globals.AimBot.AimbotBind) & 0x8000) != 0;
    if (!keyHeld) {
        g_currentTargetAddr = 0;
        clearLock();
        return;
    }

    uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;
    int IsFiring = 0;
    if (!localPlayer || !Mem.ReadFast2<int>(localPlayer + ::Offsets::IsFiring, &IsFiring) || IsFiring == 0) {
        g_currentTargetAddr = 0;
        clearLock();
        return;
    }

    Player target;
    if (!FindClosestEnemy(target)) {
        g_currentTargetAddr = 0;
        clearLock();
        return;
    }

    // --- REACTION DELAY & TARGET ACQUISITION ---
    if (target.Address != g_currentTargetAddr) {
        clearLock();
        g_currentTargetAddr = target.Address;
        acquisitionStartTime = std::chrono::steady_clock::now();
        
        if (g_Globals.AimBot.AimbotChest) {
            currentReactionDelay = 0.0f; // No delay for Chest
        } else {
            // Legit Aimbot Delay (100ms - 150ms)
            std::uniform_real_distribution<float> delayDist(150.0f, 180.0f);
            currentReactionDelay = delayDist(rng);
        }
    }

    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(now - acquisitionStartTime).count();
    
    // Don't aim until reaction delay passed
    if (elapsed < currentReactionDelay) {
        return;
    }

    // --- DATA DECOUPLING: No memory reads here! ---
    if (target.CachedColliderAddress == 0) {
        clearLock();
        return;
    }

    // CRITICAL FIX: NEVER write ChestCollider to the memory lock.
    // The game's engine expects a Head Collider ID for this specific address.
    // Using ChestCollider here causes out-of-bounds array access and crashes the game.
    uint32_t colliderValue = target.HeadColliderValue;

    if (colliderValue == 0) {
        clearLock();
        return;
    }

    uintptr_t lockedAimAddr = target.CachedColliderAddress;

    if (isPatched && lastLockedAddr != 0 && lastLockedAddr != lockedAimAddr) {
        Mem.WriteStealth<uint32_t>(lastLockedAddr, 0); // Safely reset previous target
        isPatched = false;
    }
    
    isPatched = true;
    lastLockedAddr = lockedAimAddr;

    // Sử dụng WriteStealth: Ghi trung gian qua Engine giả lập
    // và chỉ ghi khi giá trị thực sự thay đổi để né tránh Anti-cheat
    Mem.WriteStealth<uint32_t>(lockedAimAddr, colliderValue);
}

void Aimbot::RubixAimbot()
{
    if (!g_Globals.AimBot.Enabled || g_Globals.AimBot.AimbotType != 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        return;
    }

    if (g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0 || !g_Globals.EspConfig.Matrix) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return;
    }

    Player target;
    if (!FindRubixTarget(target) || target.Address == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return;
    }

    uintptr_t headColliderAddr = target.Address + Offsets::RubixHeadCollider;
    uintptr_t lockedAimAddr = target.Address + Offsets::kanishkcheat;

    uint32_t headCollider = 0;
    if (!Mem.Read<uint32_t>(headColliderAddr, headCollider) || headCollider == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return;
    }

    constexpr int repeatCount = 10;
    for (int i = 0; i < repeatCount; ++i) {
        Mem.Write<uint32_t>(lockedAimAddr, headCollider);
    }
    Mem.Write<uint32_t>(lockedAimAddr, headCollider);

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void Aimbot::MouseAimbot()
{
    if (!g_Globals.AimBot.MouseAim) return;
    bool keyHeld = (GetAsyncKeyState(g_Globals.AimBot.MouseAimBind) & 0x8000) != 0;
    if (!keyHeld) return;
    if (g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0 || !g_Globals.EspConfig.Matrix) return;

    static float remainderX = 0.0f;
    static float remainderY = 0.0f;
    static Vector2 currentCurveOffset = { 0, 0 };

    // --- PID Controller State ---
    static float errorSumX = 0.0f;
    static float errorSumY = 0.0f;
    static float lastErrorX = 0.0f;
    static float lastErrorY = 0.0f;
    static auto lastTime = std::chrono::steady_clock::now();

    Player target;
    if (!FindClosestEnemy(target)) {
        remainderX = 0; remainderY = 0;
        currentCurveOffset = { 0, 0 };
        errorSumX = 0; errorSumY = 0;
        lastErrorX = 0; lastErrorY = 0;
        return;
    }

    uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;
    int IsFiring = 0;
    if (!localPlayer || !Mem.ReadFast2<int>(localPlayer + ::Offsets::IsFiring, &IsFiring) || IsFiring == 0) {
        remainderX = 0; remainderY = 0;
        currentCurveOffset = { 0, 0 };
        errorSumX = 0; errorSumY = 0;
        lastErrorX = 0; lastErrorY = 0;
        return;
    }

    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(now - acquisitionStartTime).count();
    if (elapsed < currentReactionDelay) return;

    // Time delta for PID
    float dt = std::chrono::duration<float>(now - lastTime).count();
    lastTime = now;
    if (dt > 0.1f) dt = 0.01f; // Cap dt to prevent spikes after pause

    Vector3 targetHitBox = GetHitBoxPosition(target);

    // --- MICRO-JITTER (Humanization) ---
    std::uniform_real_distribution<float> jitterDist(-0.03f, 0.03f);
    targetHitBox.X += jitterDist(rng);
    targetHitBox.Y += jitterDist(rng);
    targetHitBox.Z += jitterDist(rng);

    ImVec2 hitBox2D = W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, targetHitBox, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
    if (hitBox2D.x < 1 || hitBox2D.y < 1) return;

    float screenCenterX = g_Globals.EspConfig.Width / 2.0f;
    float screenCenterY = g_Globals.EspConfig.Height / 2.0f;

    float deltaX = hitBox2D.x - screenCenterX;
    float deltaY = hitBox2D.y - screenCenterY;

    float dist = std::sqrt(deltaX * deltaX + deltaY * deltaY);
    if (dist > g_Globals.AimBot.Fov) return;

    // --- BEZIER-LIKE CURVE LOGIC ---
    if (currentCurveOffset.X == 0 && currentCurveOffset.Y == 0 && dist > 10.0f) {
        std::uniform_real_distribution<float> offsetDist(-10.0f, 10.0f);
        currentCurveOffset.X = offsetDist(rng);
        currentCurveOffset.Y = offsetDist(rng);
    }

    float decayFactor = (std::min)(1.0f, dist / 40.0f);
    float finalDeltaX = deltaX + (currentCurveOffset.X * decayFactor);
    float finalDeltaY = deltaY + (currentCurveOffset.Y * decayFactor);

    // --- PID CONTROLLER CALCULATION ---
    // Kp: Proportional (Speed), Ki: Integral (Correction), Kd: Derivative (Damping/Braking)
    float Kp = 0.35f / ((std::max)(1.0f, g_Globals.AimBot.Smoothing) * 0.5f);
    float Ki = 0.02f;
    float Kd = 0.015f;

    // P Term
    float pX = finalDeltaX * Kp;
    float pY = finalDeltaY * Kp;

    // I Term (Accumulated error)
    errorSumX += finalDeltaX * dt;
    errorSumY += finalDeltaY * dt;
    // Clamp I to prevent windup
    errorSumX = (std::max)(-50.0f, (std::min)(50.0f, errorSumX));
    errorSumY = (std::max)(-50.0f, (std::min)(50.0f, errorSumY));
    float iX = errorSumX * Ki;
    float iY = errorSumY * Ki;

    // D Term (Rate of change)
    float dX = (finalDeltaX - lastErrorX) / dt;
    float dY = (finalDeltaY - lastErrorY) / dt;
    lastErrorX = finalDeltaX;
    lastErrorY = finalDeltaY;
    float dTermX = dX * Kd;
    float dTermY = dY * Kd;

    // Final Move
    float moveX = pX + iX + dTermX;
    float moveY = pY + iY + dTermY;

    // --- LIMITING & OUTPUT ---
    float maxSpeed = 25.0f;
    float currentSpeed = std::sqrt(moveX * moveX + moveY * moveY);
    if (currentSpeed > maxSpeed) {
        moveX = (moveX / currentSpeed) * maxSpeed;
        moveY = (moveY / currentSpeed) * maxSpeed;
    }

    moveX += remainderX;
    moveY += remainderY;

    int finalMoveX = static_cast<int>(moveX);
    int finalMoveY = static_cast<int>(moveY);

    remainderX = moveX - finalMoveX;
    remainderY = moveY - finalMoveY;

    if (finalMoveX != 0 || finalMoveY != 0) {
        mouse_event(MOUSEEVENTF_MOVE, static_cast<DWORD>(finalMoveX), static_cast<DWORD>(finalMoveY), 0, 0);
    }
}

void Aimbot::StartAimbot() {
    if (aimbotRunning) return;
    aimbotRunning = true;
    s_rubixJitterSeed = (uint32_t)GetTickCount() ^ 0xC0FFEE42u;

    aimbotThread = std::thread([] {
        auto lastLegitUpdate = std::chrono::steady_clock::now();
        auto lastAimLockUpdate = std::chrono::steady_clock::now();

        while (aimbotRunning) {
            auto now = std::chrono::steady_clock::now();

            // Legit Aimbot (Memory)
            int legitInterval = g_Globals.AimBot.AimbotChest ? 1 : g_Globals.AimBot.UpdateInterval;
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastLegitUpdate).count() >= legitInterval) {
                Aim::Aimbot::LegitAimbot();
                lastLegitUpdate = now;
            }

            // AIMLOCK check (AimbotType == 3 or AimLockToggle)
            if (g_Globals.AimBot.Enabled && (g_Globals.AimBot.AimbotType == 3 || g_Globals.AimBot.AimLockToggle)) {
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAimLockUpdate).count() >= 16) {
                    Player target;
                    if (Aim::Aimbot::FindAimLockTarget(target)) {
                        Aim::Aimbot::AimLockTarget(target);
                    } else {
                        Aim::Aimbot::ResetAimLock();
                    }
                    lastAimLockUpdate = now;
                }
            } else if (!g_Globals.AimBot.AimLockToggle && g_Globals.AimBot.AimbotType != 3) {
                Aim::Aimbot::ResetAimLock();
            }

            // Mouse Aimbot (Humanized)
            Aim::Aimbot::MouseAimbot();

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    if (!rubixRunning) {
        rubixRunning = true;
        rubixThread = std::thread([] {
            while (rubixRunning && aimbotRunning) {
                Aim::Aimbot::RubixAimbot();
            }
        });
        rubixThread.detach();
    }

    ExtremeAimbot::Start();
}

void Aimbot::StopAimbot() {
    aimbotRunning = false;
    rubixRunning = false;
    ExtremeAimbot::Stop();
    Aim::Aimbot::ResetAimLock();
    if (aimbotThread.joinable()) {
        aimbotThread.join();
    }
}

bool Aimbot::FindAimLockTarget(Player& outTarget) {
    float closestDistance = FLT_MAX;
    bool found = false;
    Vector2 screenCenter(g_Globals.EspConfig.Width / 2.0f, g_Globals.EspConfig.Height / 2.0f);

    SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
    for (auto& pair : g_Globals.EspConfig.Entities) {
        Player* entity = &pair.second;
        if (entity->IsDead || entity->IsKnocked) continue;
        if (g_Globals.AimBot.IgnoreBots && entity->IsBot) continue;
        if (entity->Address == g_Globals.EspConfig.LocalPlayer) continue;

        Vector3 bone = (g_Globals.AimBot.AimLockBone == 1) ? entity->Hip :
                       (g_Globals.AimBot.AimLockBone == 2) ? entity->Neck : entity->Head;
        ImVec2 screenPos = W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, bone, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
        if (screenPos.x < 1 || screenPos.y < 1) continue;

        float dist = Vector3::Distance(g_Globals.EspConfig.MainCamera, bone);
        if (dist > g_Globals.AimBot.AimLockMaxDistance) continue;

        float crosshairDist = std::sqrt(std::pow(screenPos.x - screenCenter.X, 2) + std::pow(screenPos.y - screenCenter.Y, 2));
        if (g_Globals.AimBot.AimLockFovEnable && crosshairDist > (float)g_Globals.AimBot.AimLockFov) continue;

        if (crosshairDist < closestDistance) {
            closestDistance = crosshairDist;
            outTarget = *entity;
            found = true;
        }
    }
    return found;
}

void Aimbot::AimLockTarget(const Player& target) {
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

void Aimbot::ResetAimLock() {
    // Reset aimlock state - nothing to persist currently
}

} // namespace Aim
