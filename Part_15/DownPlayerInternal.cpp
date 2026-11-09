#include "DownPlayerInternal.hpp"
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <src/Globals.hpp>
#include <cmath>
#include <chrono>

std::atomic<bool> DownPlayerInternal::_isRunning{ false };
std::atomic<bool> DownPlayerInternal::_taskRunning{ false };
std::thread DownPlayerInternal::_workThread;
std::thread DownPlayerInternal::_taskThread;

bool DownPlayerInternal::isFrozen = false;
uint32_t DownPlayerInternal::lastMatrixPtr = 0;
Vector3 DownPlayerInternal::originalPos = Vector3::Zero();
Vector3 DownPlayerInternal::frozenPos = Vector3::Zero();

void DownPlayerInternal::Start() {
    if (_isRunning.load()) return;

    _isRunning = true;

    _workThread = std::thread(Work);
    _workThread.detach();

    _taskThread = std::thread(TeleportTaskLoop);
    _taskThread.detach();
}

void DownPlayerInternal::Stop() {
    _isRunning = false;
    g_Globals.Exploits.DownPlayer = false;
}

void DownPlayerInternal::Work() {
    while (_isRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Hilo principal: mientras DownPlayer activo
// baja al player y lo "congela" en esa posicion
void DownPlayerInternal::TeleportTaskLoop() {
    _taskRunning = true;

    while (_taskRunning.load()) {
        if (!g_Globals.Exploits.DownPlayer) {
            if (isFrozen) {
                // restore posicion original al desactivar
                uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;
                uint32_t currentMatrixPtr = lastMatrixPtr;

                if (localPlayer != 0) {
                    uint32_t r = 0, t1 = 0, t2 = 0, m = 0;
                    if (Mem.Read(localPlayer + (uint32_t)Offsets::Bones::Root, r) && r != 0 &&
                        Mem.Read(r + 0x8, t1) && t1 != 0 &&
                        Mem.Read(t1 + 0x8, t2) && t2 != 0 &&
                        Mem.Read(t2 + 0x20, m) && m != 0)
                    {
                        currentMatrixPtr = m;
                    }
                }

                float opLen = sqrtf(originalPos.X * originalPos.X +
                                    originalPos.Y * originalPos.Y +
                                    originalPos.Z * originalPos.Z);
                if (currentMatrixPtr != 0 &&
                    originalPos != Vector3::Zero() &&
                    opLen > 0.1f)
                {
                    for (int k = 0; k < 20; k++) {
                        Mem.Write<Vector3>(currentMatrixPtr + kMatrixPosOffset, originalPos);
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                }

                isFrozen = false;
                lastMatrixPtr = 0;
                originalPos = Vector3::Zero();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;
        if (localPlayer == 0) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); continue; }

        uint32_t rootPtr = 0;
        if (!Mem.Read(localPlayer + (uint32_t)Offsets::Bones::Root, rootPtr) || rootPtr == 0)
            goto dp_next_tick;

        uint32_t dp_t1 = 0;
        if (!Mem.Read(rootPtr + 0x8, dp_t1) || dp_t1 == 0) goto dp_next_tick;

        uint32_t dp_t2 = 0;
        if (!Mem.Read(dp_t1 + 0x8, dp_t2) || dp_t2 == 0) goto dp_next_tick;

        uint32_t matrixPtr = 0;
        if (!Mem.Read(dp_t2 + 0x20, matrixPtr) || matrixPtr == 0) goto dp_next_tick;

        lastMatrixPtr = matrixPtr;

        Vector3 currentPos;
        if (!Mem.Read<Vector3>(matrixPtr + kMatrixPosOffset, currentPos))
            goto dp_next_tick;

        if (!isFrozen) {
            originalPos = currentPos;

            float len = sqrtf(originalPos.X * originalPos.X +
                              originalPos.Y * originalPos.Y +
                              originalPos.Z * originalPos.Z);
            if (len > 0.1f) {
                Vector3 newPos = currentPos;
                newPos.Y -= teleportDownDistance;      // baja 0.9 unidades
                Mem.Write<Vector3>(matrixPtr + kMatrixPosOffset, newPos);
                frozenPos = newPos;
                isFrozen = true;
            }
        }
        else {
            Mem.Write<Vector3>(matrixPtr + kMatrixPosOffset, frozenPos);  // lo mantiene abajo
        }

    dp_next_tick:
        std::this_thread::sleep_for(std::chrono::milliseconds(teleportDelay));
    }

    _taskRunning = false;
}