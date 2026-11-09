#pragma once
#include <cstdint>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <random>
#include <chrono>
#include "Vector3.hpp"

// Forward for NINJA X globals
extern bool AimbotRage;
extern int fov;
extern float maxDistance;

struct PersistentEntityCache {
    uintptr_t hipAddr = 0;
    uint32_t originalHip = 0;
    Vector3 lastPosition = Vector3::Zero();
    std::chrono::high_resolution_clock::time_point lastSeen;
};

namespace AimExternal {
    void StartAimbot();
    void StopAimbot();
    void ExternalAimbotTick(); // one tick, called from thread
    void AimbotExternalRunToggle(bool enable);
    extern std::atomic<bool> g_aimbotExternalBusy;
    extern std::atomic<bool> g_aimbotExternalPatched;
    extern bool prev_aimbot_memory_state;
    extern std::thread aimbotThread2;
    extern std::atomic<bool> aimbotRunning2;
}
