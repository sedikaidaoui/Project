#pragma once
#include <cstdint>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Math/Matrix4v4.hpp>
#include <src/Globals.hpp>
#include <EspLines/Memory/Memory.hpp>

namespace TpWall {
     
    void TeleportForward();
    
    void Update();
    
    void Start();
    void Stop();
    
    void SetTeleportDistance(float distance);
    void SetTeleportDelay(int delayMs);
    
    bool GetCameraForward(Vector3& forward);
    Vector3 CalculateNewPosition(const Vector3& currentPos, const Vector3& cameraForward);
    bool GetPlayerPosition(Vector3& position);
    bool SetPlayerPosition(const Vector3& position);
}