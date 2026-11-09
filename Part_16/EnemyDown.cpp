#include "EnemyDown.hpp"

#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/TMatrix.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Player.h>
#include <src/Globals.hpp>
#include <Windows.h>
#include <chrono>
#include <unordered_map>

namespace {

std::atomic<bool> isRunning{ false };
std::thread worker;

std::unordered_map<uint32_t, Vector3> s_originalPositions;

void RestoreAll() {
    for (const auto& kvp : s_originalPositions) {
        uint32_t rootBone = 0, t1 = 0, t2 = 0, matrixPtr = 0;
        if (!Mem.ReadFast2<uint32_t>(kvp.first + Offsets::Bones::Root, &rootBone) || !rootBone) continue;
        if (!Mem.ReadFast2<uint32_t>(rootBone + 0x8, &t1) || !t1) continue;
        if (!Mem.ReadFast2<uint32_t>(t1 + 0x8, &t2) || !t2) continue;
        if (!Mem.ReadFast2<uint32_t>(t2 + 0x20, &matrixPtr) || !matrixPtr) continue;
        Mem.Write<Vector3>(matrixPtr + 0x60, kvp.second);
    }
    s_originalPositions.clear();
}

bool GetNodePosition(uint32_t rootBone, Vector3& out) {
    return TransformUtils::GetNodePosition(rootBone, out);
}

} // namespace

void EnemyDown::Work() {
    if (isRunning) return;
    isRunning = true;

    worker = std::thread([] {
        while (isRunning) {
            if (!g_Globals.Exploits.EnemyDown) {
                if (!s_originalPositions.empty()) RestoreAll();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            const uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;
            if (!localPlayer) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            uint32_t localRoot = Mem.ReadS<uint32_t>(localPlayer + Offsets::Bones::Root);
            Vector3 localPos;
            if (!localRoot || !GetNodePosition(localRoot, localPos)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            if (localPos.X == 0.0f && localPos.Y == 0.0f && localPos.Z == 0.0f) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            Player* bestEnemy = nullptr;
            float closestDist = 9999.0f;

            SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
            for (auto& kvp : g_Globals.EspConfig.Entities) {
                Player& ent = kvp.second;
                if (ent.Address == 0 || ent.Address == localPlayer) continue;
                if (!ent.IsKnown || ent.IsDead) continue;
                if (ent.IsKnocked) continue;
                if (ent.IsTeam == Player::Bool3::True) continue;

                uint32_t entRoot = Mem.ReadS<uint32_t>(ent.Address + Offsets::Bones::Root);
                Vector3 rootPos;
                if (!entRoot || !GetNodePosition(entRoot, rootPos)) continue;

                float dist = Vector3::Distance(localPos, rootPos);
                if (dist < closestDist) {
                    closestDist = dist;
                    bestEnemy = &ent;
                }
            }

            if (!bestEnemy) {
                if (!s_originalPositions.empty()) RestoreAll();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            uint32_t bestRoot = Mem.ReadS<uint32_t>(bestEnemy->Address + Offsets::Bones::Root);
            Vector3 enemyPos;
            if (!bestRoot || !GetNodePosition(bestRoot, enemyPos)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            if (enemyPos.X == 0.0f && enemyPos.Y == 0.0f && enemyPos.Z == 0.0f) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            auto it = s_originalPositions.find(bestEnemy->Address);
            if (it == s_originalPositions.end())
                it = s_originalPositions.emplace(bestEnemy->Address, enemyPos).first;

            const Vector3 origPos = it->second;

            float dropAmount = (enemyPos.Y > localPos.Y) ? 2.0f : 1.8f;

            Vector3 newPos;
            newPos.X = enemyPos.X;
            newPos.Y = origPos.Y - dropAmount;
            newPos.Z = enemyPos.Z;

            uint32_t t1 = 0, t2 = 0, matrixPtr = 0;
            if (Mem.ReadFast2<uint32_t>(bestRoot + 0x8, &t1) && t1 &&
                Mem.ReadFast2<uint32_t>(t1 + 0x8, &t2) && t2 &&
                Mem.ReadFast2<uint32_t>(t2 + 0x20, &matrixPtr) && matrixPtr) {
                Mem.Write<Vector3>(matrixPtr + 0x60, newPos);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        if (!s_originalPositions.empty()) RestoreAll();
    });
    worker.detach();
}

void EnemyDown::Stop() {
    isRunning = false;
    if (!s_originalPositions.empty()) RestoreAll();
}