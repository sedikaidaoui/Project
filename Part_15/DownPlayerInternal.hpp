#pragma once
#include <cstdint>
#include <atomic>
#include <thread>
#include <EspLines/Math/Vector/Vector3.hpp>

class DownPlayerInternal {
public:
    static void Start();
    static void Stop();
private:
    static void Work();
    static void TeleportTaskLoop();

    static std::atomic<bool> _isRunning;
    static std::atomic<bool> _taskRunning;
    static std::thread _workThread;
    static std::thread _taskThread;

    static bool isFrozen;
    static uint32_t lastMatrixPtr;
    static Vector3 originalPos;
    static Vector3 frozenPos;

    static constexpr uint32_t kMatrixPosOffset = 0x60;
    static constexpr float teleportDownDistance = 0.9f;
    static constexpr int teleportDelay = 1;
};