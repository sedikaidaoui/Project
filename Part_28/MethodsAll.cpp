#include "MethodsAll.hpp"
#include <src/Globals.hpp>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <EspLines/Player.h>
#include <EspLines/Exploits/Exploit/EnemyPull360.hpp>
#include <EspLines/Exploits/Exploit/MagnetEnemy.hpp>
#include <EspLines/Exploits/Exploit/FakeLagFreeze.hpp>
#include <Windows.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>

namespace FWork {
namespace MethodsAll {

    static std::atomic<bool> s_workersRunning{ false };
    static std::thread s_speedThread;
    static std::thread s_shakeThread;

    static uint32_t s_lastSpeedTimer = 0;

    static void SpeedTimerWorker()
    {
        while (s_workersRunning) {
            if (!g_Globals.Exploits.SpeedTimerHack) {
                if (s_lastSpeedTimer != 0) {
                    Mem.Write<float>(s_lastSpeedTimer + Offsets::FixedDeltaTime, 0.033000f);
                    s_lastSpeedTimer = 0;
                    g_Globals.Exploits.SpeedTimerHackActive = false;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            if (Offsets::Il2Cpp == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            uint32_t baseGameFacade = 0, gameFacade = 0, staticGameFacade = 0, currentGame = 0, speedTimer = 0;

            if (Mem.Read<uint32_t>(Offsets::Il2Cpp + Offsets::InitBase, baseGameFacade) && baseGameFacade != 0 &&
                Mem.Read<uint32_t>(baseGameFacade, gameFacade) && gameFacade != 0 &&
                Mem.Read<uint32_t>(gameFacade + Offsets::StaticClass, staticGameFacade) && staticGameFacade != 0 &&
                Mem.Read<uint32_t>(staticGameFacade, currentGame) && currentGame != 0 &&
                Mem.Read<uint32_t>(currentGame + Offsets::GameTimer, speedTimer) && speedTimer != 0)
            {
                s_lastSpeedTimer = speedTimer;
                Mem.Write<float>(speedTimer + Offsets::FixedDeltaTime, 0.065000f);
                g_Globals.Exploits.SpeedTimerHackActive = true;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (s_lastSpeedTimer != 0) {
            Mem.Write<float>(s_lastSpeedTimer + Offsets::FixedDeltaTime, 0.033000f);
            s_lastSpeedTimer = 0;
        }
    }

    // UpPlayer122 shake kill — X-axis shake on crosshair-closest enemy while firing
    static void ShakeKillWorker()
    {
        while (s_workersRunning) {
            if (!g_Globals.Exploits.ShakeKill) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;
            bool isFiring = false;
            if (!localPlayer ||
                !Mem.ReadFast2<bool>(localPlayer + Offsets::IsFiring, &isFiring) ||
                !isFiring)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            if (g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0 || !g_Globals.EspConfig.Matrix) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            float screenCenterX = g_Globals.EspConfig.Width * 0.5f;
            float screenCenterY = g_Globals.EspConfig.Height * 0.5f;
            Player* bestTarget = nullptr;
            float closestCrosshairDist = FLT_MAX;

            {
                SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
                for (auto& pair : g_Globals.EspConfig.Entities) {
                    Player& entity = pair.second;
                    if (!entity.IsKnown || entity.IsDead || entity.IsKnocked) continue;
                    if (entity.IsTeam == Player::Bool3::True) continue;
                    if (entity.Address == localPlayer) continue;

                    ImVec2 head2D = W2S::WorldToScreenImVec2(
                        g_Globals.EspConfig.ViewMatrix, entity.Head,
                        g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
                    if (head2D.x < 1.0f || head2D.y < 1.0f) continue;

                    float dx = head2D.x - screenCenterX;
                    float dy = head2D.y - screenCenterY;
                    float crosshairDist = sqrtf(dx * dx + dy * dy);
                    if (crosshairDist >= closestCrosshairDist) continue;

                    closestCrosshairDist = crosshairDist;
                    bestTarget = &entity;
                }
            }

            if (!bestTarget) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            uint32_t rootBone = Mem.ReadS<uint32_t>(bestTarget->Address + static_cast<uint32_t>(Offsets::Bones::Root));
            if (!rootBone) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            uint32_t transformValue = Mem.ReadS<uint32_t>(rootBone + 0x8);
            if (!transformValue) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            uint32_t transformObj = Mem.ReadS<uint32_t>(transformValue + 0x8);
            if (!transformObj) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            uint32_t matrix = Mem.ReadS<uint32_t>(transformObj + 0x20);
            if (!matrix) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
            if (!Mem.Read<float>(matrix + Offsets::ShakeKillPosX, posX) ||
                !Mem.Read<float>(matrix + Offsets::ShakeKillPosY, posY) ||
                !Mem.Read<float>(matrix + Offsets::ShakeKillPosZ, posZ))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            (void)posY;
            (void)posZ;

            const int sign = (rand() % 2 == 0) ? -1 : 1;
            const float shakeX = static_cast<float>(sign) * ((static_cast<float>(rand() % 300) / 100.0f) + 2.5f);
            const float newX = posX + shakeX;

            Mem.Write<float>(matrix + Offsets::ShakeKillPosX, newX);

            std::this_thread::sleep_for(std::chrono::milliseconds(90));
        }
    }

    void Initialize()
    {
        if (s_workersRunning) return;
        s_workersRunning = true;

        s_speedThread = std::thread(SpeedTimerWorker);
        s_shakeThread = std::thread(ShakeKillWorker);

        s_speedThread.detach();
        s_shakeThread.detach();

        EnemyPull::EnemyPull360::Start();
        Magnet::MagnetEnemy::Start();

        FakeLagFreeze::Initialize();
    }

    void Shutdown()
    {
        s_workersRunning = false;
        FakeLagFreeze::Shutdown();
        EnemyPull::EnemyPull360::Stop();
        Magnet::MagnetEnemy::Stop();

        if (s_lastSpeedTimer != 0) {
            Mem.Write<float>(s_lastSpeedTimer + Offsets::FixedDeltaTime, 0.0166667f);
            s_lastSpeedTimer = 0;
        }
        g_Globals.Exploits.SpeedTimerHackActive = false;

        // Walk Speed : remet le multiplicateur de deplacement a 1.0 AVANT le
        // detachement, sinon le joueur reste x60 jusqu'a la fin du match.
        if (g_Globals.Exploits.WalkSpeedAttrPtr != 0) {
            Mem.Write<float>((uintptr_t)g_Globals.Exploits.WalkSpeedAttrPtr + 0x1BC, 1.0f);
            Mem.Write<float>((uintptr_t)g_Globals.Exploits.WalkSpeedAttrPtr + 0x1B8, 1.0f);
            g_Globals.Exploits.WalkSpeedAttrPtr = 0;
        }
        g_Globals.Exploits.WalkSpeedActive = false;
        g_Globals.Exploits.WalkSpeedApplied = 0.0f;
    }

} // namespace MethodsAll
} // namespace FWork
