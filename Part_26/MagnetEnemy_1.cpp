#include "MagnetEnemy.hpp"
#include <imgui.h>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <EspLines/Math/TMatrix.hpp>
#include <src/Globals.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Player.h>
#include <Windows.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <cmath>

namespace Magnet {

namespace {

    struct EnemyData {
        uint32_t address = 0;
        Vector3 originalPos;
        Vector3 enemyPos;
        Vector3 chestPosition;
        float distance = 0.0f;
        std::chrono::steady_clock::time_point startTime;
        bool isMovingToHead = false;
    };

    std::atomic<bool> s_running{ false };
    std::thread s_worker;
    std::vector<EnemyData> s_enemyList;

    bool IsValidMagnetTarget(const Player& entity)
    {
        if (entity.IsDead) return false;
        if (g_Globals.AimBot.IgnoreKnocked && entity.IsKnocked) return false;
        if (g_Globals.AimBot.IgnoreBots && entity.IsBot) return false;
        return true;
    }

} // namespace

bool GetEnemyMatrix(uint32_t address, uint32_t& matrixOut)
{
    uint32_t rootBone = 0, transformValue = 0, transformObj = 0;
    if (!Mem.ReadFast2<uint32_t>(address + Offsets::Bones::Root, &rootBone) || !rootBone) return false;
    if (!Mem.ReadFast2<uint32_t>(rootBone + 0x8, &transformValue) || !transformValue) return false;
    if (!Mem.ReadFast2<uint32_t>(transformValue + 0x8, &transformObj) || !transformObj) return false;
    if (!Mem.ReadFast2<uint32_t>(transformObj + 0x20, &matrixOut) || !matrixOut) return false;
    return true;
}

void RestoreEnemy(EnemyData& data)
{
    uint32_t matrix = 0;
    if (!GetEnemyMatrix(data.address, matrix)) return;
    Mem.Write<Vector3>(matrix + Offsets::MatrixPosition, data.originalPos);
}

void ScanEnemiesOnce()
{
    s_enemyList.clear();

    if (g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0 || !g_Globals.EspConfig.Matrix)
        return;

    const float centerX = g_Globals.EspConfig.Width * 0.5f;
    const float centerY = g_Globals.EspConfig.Height * 0.5f;
    const float maxFov = g_Globals.Exploits.MagnetEnemyFov;
    const float maxDist = g_Globals.Exploits.MagnetEnemyMaxDistance;

    Player* bestTarget = nullptr;
    float bestMetric = FLT_MAX;

    {
        SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
        for (auto& pair : g_Globals.EspConfig.Entities) {
            Player& entity = pair.second;
            if (entity.Address == g_Globals.EspConfig.LocalPlayer) continue;
            if (!entity.IsKnown || entity.IsDead) continue;
            if (entity.IsTeam == Player::Bool3::True) continue;
            if (entity.Distance > maxDist) continue;
            if (g_Globals.AimBot.IgnoreKnocked && entity.IsKnocked) continue;
            if (g_Globals.AimBot.IgnoreBots && entity.IsBot) continue;

            ImVec2 screenPos = W2S::WorldToScreenImVec2(
                g_Globals.EspConfig.ViewMatrix, entity.Head,
                g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
            if (screenPos.x < 0.0f || screenPos.y < 0.0f) continue;

            const float dx = screenPos.x - centerX;
            const float dy = screenPos.y - centerY;
            const float fovDist = sqrtf(dx * dx + dy * dy);
            if (fovDist > maxFov) continue;

            if (fovDist < bestMetric) {
                bestMetric = fovDist;
                bestTarget = &entity;
            }
        }
    }

    if (!bestTarget) return;

    uint32_t matrix = 0;
    if (!GetEnemyMatrix(bestTarget->Address, matrix)) return;

    Vector3 originalPos;
    if (!Mem.Read<Vector3>(matrix + Offsets::MatrixPosition, originalPos)) return;

    s_enemyList.push_back({
        bestTarget->Address,
        originalPos,
        originalPos,
        originalPos,
        bestTarget->Distance,
        std::chrono::steady_clock::now(),
        false
    });
}

Vector3 GetCurrentTargetPosition(EnemyData& data)
{
    const auto currentTime = std::chrono::steady_clock::now();
    const auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - data.startTime).count();

    if (elapsedTime < 500 && !data.isMovingToHead) {
        uint32_t chestBone = 0;
        if (Mem.ReadFast2<uint32_t>(data.address + Offsets::Bones::Hip, &chestBone) && chestBone) {
            Vector3 chestPos;
            if (TransformUtils::GetNodePosition(chestBone, chestPos) && chestPos != Vector3::Zero())
                data.chestPosition = chestPos;
        }
        return data.chestPosition;
    }

    if (!data.isMovingToHead) {
        data.isMovingToHead = true;
        data.startTime = currentTime;
    }

    uint32_t headBone = 0;
    if (Mem.ReadFast2<uint32_t>(data.address + Offsets::Bones::Head, &headBone) && headBone) {
        Vector3 headPos;
        if (TransformUtils::GetNodePosition(headBone, headPos) && headPos != Vector3::Zero())
            return headPos;
    }

    return data.enemyPos;
}

void Work()
{
    while (s_running) {
        if (!g_Globals.Exploits.MagnetEnemyEnabled) {
            for (auto& data : s_enemyList)
                RestoreEnemy(data);
            s_enemyList.clear();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        if (!g_Globals.EspConfig.Matrix || !g_Globals.EspConfig.LocalPlayer ||
            g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        const bool isKeyPressed =
            (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0 ||
            (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

        if (isKeyPressed && s_enemyList.empty())
            ScanEnemiesOnce();
        else if (!isKeyPressed) {
            for (auto& data : s_enemyList)
                RestoreEnemy(data);
            s_enemyList.clear();
        }

        if (isKeyPressed) {
            for (auto it = s_enemyList.begin(); it != s_enemyList.end();) {
                auto& data = *it;

                bool valid = false;
                {
                    SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
                    auto found = g_Globals.EspConfig.Entities.find(data.address);
                    if (found != g_Globals.EspConfig.Entities.end() && IsValidMagnetTarget(found->second))
                        valid = true;
                }

                if (!valid) {
                    RestoreEnemy(data);
                    it = s_enemyList.erase(it);
                    continue;
                }

                Vector3 targetHeadPos = GetCurrentTargetPosition(data);
                if (targetHeadPos == Vector3::Zero()) {
                    ++it;
                    continue;
                }

                Vector3 camForward = {
                    g_Globals.EspConfig.ViewMatrix.m02,
                    g_Globals.EspConfig.ViewMatrix.m12,
                    g_Globals.EspConfig.ViewMatrix.m22
                };
                camForward = Vector3::Normalized(camForward);

                const Vector3 camPos = g_Globals.EspConfig.MainCamera;
                Vector3 bulletPoint = {
                    camPos.X + camForward.X * data.distance,
                    camPos.Y + camForward.Y * data.distance,
                    camPos.Z + camForward.Z * data.distance
                };

                uint32_t matrix = 0;
                if (!GetEnemyMatrix(data.address, matrix)) {
                    ++it;
                    continue;
                }

                Vector3 moveDir = bulletPoint - targetHeadPos;
                moveDir.Y = 0.0f;
                moveDir = Vector3::Normalized(moveDir);

                const float speed = g_Globals.Exploits.MagnetEnemySpeed;
                Vector3 targetPos = data.enemyPos + moveDir;

                data.enemyPos.X += (targetPos.X - data.enemyPos.X) * speed;
                data.enemyPos.Y = data.enemyPos.Y;
                data.enemyPos.Z += (targetPos.Z - data.enemyPos.Z) * speed;

                Mem.Write<Vector3>(matrix + Offsets::MatrixPosition, data.enemyPos);
                ++it;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    for (auto& data : s_enemyList)
        RestoreEnemy(data);
    s_enemyList.clear();
}

void MagnetEnemy::Start()
{
    if (s_running) return;
    s_running = true;
    s_worker = std::thread(Work);
    s_worker.detach();
}
void MagnetEnemy::Stop()
{
    s_running = false;
    for (auto& data : s_enemyList)
        RestoreEnemy(data);
    s_enemyList.clear();
}

} // namespace Magnet
