#include <imgui.h>
#include <imgui_internal.h>
#include <EspLines\Memory\Memory.hpp>
#include <EspLines\Math\Vector\Vector3.hpp>
#include <EspLines\Math\Vector\Vector2.hpp>
#include <EspLines\Aimbot\RageAimbot.hpp> 
#include <src\Globals.hpp>
#include <EspLines\Offsets.hpp>
#include <EspLines\Math\WordToScreen.hpp>

namespace Aim {

Vector3 RageAimbot::GetHitBoxPosition(const Player& entity) {
    using HitBox = Config::HitBox;

    switch (g_Globals.AimBot.HitBox) {
    case HitBox::Neck: return entity.Neck;
    case HitBox::Chest: return entity.Hip;
    case HitBox::Head: return entity.Head;
    default:            return entity.Head;
    }
}


bool RageAimbot::FindClosestEnemy(Player& outTarget) {
    float closestDistance = FLT_MAX;
    bool found = false;

    Vector2 screenCenter(g_Globals.EspConfig.Width / 2.0f, g_Globals.EspConfig.Height / 2.0f);

    SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
    for (auto& pair : g_Globals.EspConfig.Entities) {
        Player* entity = &pair.second;

        if (entity->IsDead || (g_Globals.AimBot.IgnoreKnocked && entity->IsKnocked)) continue;

        Vector3 targetHitBox = GetHitBoxPosition(*entity);
        ImVec2 hitBox2D = W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, targetHitBox, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
        if (hitBox2D.x < 1 || hitBox2D.y < 1) continue;

        float distance = Vector3::Distance(g_Globals.EspConfig.MainCamera, targetHitBox);
        if (distance > g_Globals.AimBot.DistanceAim) continue;

        float crosshairDist = std::sqrt(std::pow(hitBox2D.x - screenCenter.X, 2) + std::pow(hitBox2D.y - screenCenter.Y, 2));
        if (crosshairDist >= closestDistance) continue;

        closestDistance = crosshairDist;
        outTarget = *entity;
        found = true;
    }

    return found;
}


static bool isPatched = false;
static uintptr_t lastPatchedAddr = 0;
static uint32_t lastOriginalValue = 0;

void RageAimbot::Aimbot()
{
    if (!g_Globals.AimBot.AimShoulder)
    {
        if (isPatched && lastPatchedAddr) {
            Mem.Write<uint32_t>(lastPatchedAddr, lastOriginalValue);
            isPatched = false;
            lastPatchedAddr = 0;
            lastOriginalValue = 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        return;
    }

    Player target;
        if (!FindClosestEnemy(target)) {
            if (isPatched && lastPatchedAddr) {
                Mem.Write<uint32_t>(lastPatchedAddr, lastOriginalValue);
                isPatched = false;
                lastPatchedAddr = 0;
                lastOriginalValue = 0;
            }
            std::this_thread::sleep_for(std::chrono::nanoseconds(5));
            return;
        }

        uintptr_t headColliderAddr = target.Address + Offsets::ColliderHECFNHJKOMN;
        uintptr_t lockedAimAddr = target.Address + Offsets::ColliderINICDNFOFJB;

        uint32_t collider = 0;
        if (!Mem.ReadFast2<uint32_t>(headColliderAddr, &collider) || collider == 0) {
            if (isPatched && lastPatchedAddr) {
                Mem.Write<uint32_t>(lastPatchedAddr, lastOriginalValue);
                isPatched = false;
                lastPatchedAddr = 0;
                lastOriginalValue = 0;
            }
            return;
        }

        if (isPatched && lastPatchedAddr != lockedAimAddr) {
            Mem.Write<uint32_t>(lastPatchedAddr, lastOriginalValue);
            isPatched = false;
        }

        if (!isPatched) {
            if (!Mem.ReadFast2<uint32_t>(lockedAimAddr, &lastOriginalValue)) return;
            lastPatchedAddr = lockedAimAddr;
            isPatched = true;
        }
        
        // Sử dụng WriteStealth để ghi qua Engine giả lập
        Mem.WriteStealth<uint32_t>(lockedAimAddr, collider);
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
}
} // namespace Aim