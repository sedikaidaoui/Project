#include <atomic>
#include <chrono>
#include "TpWall.hpp"
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Offsets.hpp>
#include <src/Globals.hpp>
#include <Windows.h>
#include <cmath>

namespace TpWall {

    static Vector3 currentVelocity = { 0, 0, 0 };
    static auto lastUpdateTime = std::chrono::steady_clock::now();
    static bool initialized = false;

    float SmoothDamp(float current, float target, float& velocity, float smoothTime, float maxSpeed, float deltaTime) {
        smoothTime = (smoothTime < 0.0001f) ? 0.0001f : smoothTime;
        float omega = 2.0f / smoothTime;
        float x = omega * deltaTime;
        float exp_val = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
        float change = current - target;
        float originalTarget = target;
        
        float maxChange = maxSpeed * smoothTime;
        change = (change > maxChange) ? maxChange : (change < -maxChange) ? -maxChange : change;
        target = current - change;
        
        float temp = (velocity + omega * change) * deltaTime;
        velocity = (velocity - omega * temp) * exp_val;
        float output = target + (change + temp) * exp_val;
        
        if ((originalTarget - current > 0.0f) == (output > originalTarget)) {
            output = originalTarget;
            velocity = (output - originalTarget) / deltaTime;
        }
        return output;
    }

    bool GetCameraForward(Vector3& forward) {
        if (!g_Globals.EspConfig.Matrix) return false;
        Matrix4x4& vm = g_Globals.EspConfig.ViewMatrix;
        forward = { vm.m02, vm.m12, vm.m22 };
        return true;
    }

    uint32_t GetMatrixPtr(uint32_t localPlayer) {
        if (!localPlayer) return 0;
        uint32_t rootBonePtr = Mem.ReadS<uint32_t>(localPlayer + static_cast<uint32_t>(Offsets::Bones::Root));
        if (!rootBonePtr) return 0;
        uint32_t transform1 = Mem.ReadS<uint32_t>(rootBonePtr + 0x8);
        if (!transform1) return 0;
        uint32_t transform2 = Mem.ReadS<uint32_t>(transform1 + 0x8);
        if (!transform2) return 0;
        return Mem.ReadS<uint32_t>(transform2 + 0x20);
    }

    void TeleportForward() {
        uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;
        if (!localPlayer) return;

        Vector3 cameraForward;
        if (!GetCameraForward(cameraForward)) return;

        uint32_t matrixPtr = GetMatrixPtr(localPlayer);
        if (!matrixPtr) return;

        Vector3 currentPos = Mem.ReadS<Vector3>(matrixPtr + 0x60);

        if (!initialized) {
            currentVelocity = { 0, 0, 0 };
            initialized = true;
            lastUpdateTime = std::chrono::steady_clock::now();
            return;
        }

        auto now = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastUpdateTime).count();
        lastUpdateTime = now;
        deltaTime = (deltaTime > 0.1f) ? 0.1f : (deltaTime < 0.001f) ? 0.001f : deltaTime;

        float distance = g_Globals.Exploits.TpWallDistance;
        float smoothness = g_Globals.Exploits.TpWallSmooth;

        float adjustedY = cameraForward.Y;
        float actualDistance = distance;
        
       /* if (cameraForward.Y > 0.3f) {
            actualDistance = distance * 2.5f;   
            
            uint32_t movementComponent = Mem.ReadS<uint32_t>(localPlayer + Offsets::FlyHack::MOVEMENT_COMPONENT);
            if (movementComponent) {
                Mem.Write<float>(movementComponent + Offsets::FlyHack::VSPEED, 50.0f);
            }
        }*/
         
        /*else if (std::fabs(cameraForward.Y) < 0.3f) {
            adjustedY = 0.05f;   
        }*/

        Vector3 targetPosition = {
            currentPos.X + (cameraForward.X * actualDistance),
            currentPos.Y + (adjustedY * actualDistance),
            currentPos.Z + (cameraForward.Z * actualDistance)
        };

        float smoothTime = 1.0f / smoothness;   
        float maxSpeed = actualDistance * smoothness * 2.0f;

        Vector3 smoothedPos = {
            SmoothDamp(currentPos.X, targetPosition.X, currentVelocity.X, smoothTime, maxSpeed, deltaTime),
            SmoothDamp(currentPos.Y, targetPosition.Y, currentVelocity.Y, smoothTime, maxSpeed, deltaTime),
            SmoothDamp(currentPos.Z, targetPosition.Z, currentVelocity.Z, smoothTime, maxSpeed, deltaTime)
        };

        Mem.Write<Vector3>(matrixPtr + 0x60, smoothedPos);
    }

    void Update() {
        if (!g_Globals.Exploits.TpWall) {
             
            initialized = false;
            currentVelocity = { 0, 0, 0 };
            return;
        }
        if (g_Globals.EspConfig.LocalPlayer == 0) return;

        TeleportForward();
    }

    void Start() { 
        initialized = false;
        currentVelocity = { 0, 0, 0 };
    }
    
    void Stop() { 
        initialized = false;
        currentVelocity = { 0, 0, 0 };
    }
    void SetTeleportDistance(float distance) { g_Globals.Exploits.TpWallDistance = distance; }
    void SetTeleportDelay(int delayMs) {}

    Vector3 CalculateNewPosition(const Vector3& currentPos, const Vector3& cameraForward) {
        float d = g_Globals.Exploits.TpWallDistance;
        return { currentPos.X + cameraForward.X * d, currentPos.Y + cameraForward.Y * d, currentPos.Z + cameraForward.Z * d };
    }

    bool GetPlayerPosition(Vector3& position) {
        uint32_t matrixPtr = GetMatrixPtr(g_Globals.EspConfig.LocalPlayer);
        if (!matrixPtr) return false;
        position = Mem.ReadS<Vector3>(matrixPtr + 0x60);
        return true;
    }

    bool SetPlayerPosition(const Vector3& position) {
        uint32_t matrixPtr = GetMatrixPtr(g_Globals.EspConfig.LocalPlayer);
        if (!matrixPtr) return false;
        Mem.Write<Vector3>(matrixPtr + 0x60, position);
        return true;
    }
}