#include "NoGravityFly.hpp"
#include "imgui.h"
#include "Offset.hpp"
#include "Unity.hh"
#include "Complementos.h"
#include <Windows.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>

namespace NoGravityFly {
    static constexpr float HORIZONTAL_SPEED = 6.0f;
    static constexpr float UP_SPEED = 3.0f;
    static constexpr float DOWN_SPEED = -3.0f;
    static constexpr float HOVER_SPEED = 0.02f;
    static constexpr float MAX_FLY_HEIGHT = 9000.0f;

    static float baseHeight = 0.0f;
    static bool heightInitialized = false;
    static bool boostApplied = false;
    static std::atomic<bool> isRunning{false};
    static std::thread workThread;

    static void ApplyFly() {
        // Read localPlayer via Il2Cpp chain
        uint32_t baseGameFacade = 0, gameFacade = 0, staticGameFacade = 0, currentGame = 0, currentMatch = 0, localPlayer = 0;
        if (!ReadZ(Offsets::Il2Cpp + Offsets::InitBase, baseGameFacade) || !baseGameFacade) return;
        if (!ReadZ(baseGameFacade, gameFacade) || !gameFacade) return;
        if (!ReadZ(gameFacade + Offsets::StaticClass, staticGameFacade) || !staticGameFacade) return;
        if (!ReadZ(staticGameFacade, currentGame) || !currentGame) return;
        if (!ReadZ(currentGame + Offsets::CurrentMatch, currentMatch) || !currentMatch) return;
        if (!ReadZ(currentMatch + Offsets::LocalPlayer, localPlayer) || !localPlayer) return;

        uint32_t moveComp = 0;
        if (!ReadZ(localPlayer + Offsets::MovementComponent, moveComp) || moveComp == 0) return;

        uint8_t grounded = 0;
        WriteZ<uint8_t>(localPlayer + Offsets::IsGrounded, grounded);

        Vector3 pos{};
        if (!ReadZ(moveComp + Offsets::Position, pos)) return;

        if (!heightInitialized) { baseHeight = pos.Y; heightInitialized = true; }
        if (!boostApplied) {
            pos.Y += 20.0f;
            boostApplied = true;
            WriteZ<Vector3>(moveComp + Offsets::Position, pos);
            WriteZ<float>(moveComp + Offsets::VSpeed, UP_SPEED);
            return;
        }

        float maxAllowedHeight = baseHeight + MAX_FLY_HEIGHT;

        // Read viewMatrix for forward/right
        uint32_t mainTransform = 0, followCamera = 0, camera = 0, cameraBase = 0;
        Matrix4x4 viewMatrix{};
        bool hasMatrix = false;
        if (ReadZ(localPlayer + Offsets::MainCameraTransform, mainTransform) && mainTransform) {
            Vector3 camPos; GetPosition(mainTransform, camPos);
            if (ReadZ(localPlayer + Offsets::FollowCamera, followCamera) && followCamera) {
                if (ReadZ(followCamera + Offsets::Camera, camera) && camera) {
                    if (ReadZ(camera + 0x8, cameraBase) && cameraBase) {
                        if (ReadZ(cameraBase + Offsets::ViewMatrix, viewMatrix)) hasMatrix = true;
                    }
                }
            }
        }
        if (!hasMatrix) return;

        Vector3 forward(viewMatrix.m02, viewMatrix.m12, viewMatrix.m22);
        Vector3 right(viewMatrix.m00, viewMatrix.m10, viewMatrix.m20);
        Vector3 fwd = Vector3::Normalized(Vector3(forward.X, 0.0f, forward.Z));
        Vector3 rgt = Vector3::Normalized(Vector3(right.X, 0.0f, right.Z));
        Vector3 moveDir = Vector3::Zero();
        if (GetAsyncKeyState(0x57) & 0x8000) moveDir = moveDir + fwd;
        if (GetAsyncKeyState(0x53) & 0x8000) moveDir = moveDir - fwd;
        if (GetAsyncKeyState(0x44) & 0x8000) moveDir = moveDir + rgt;
        if (GetAsyncKeyState(0x41) & 0x8000) moveDir = moveDir - rgt;
        float moveLen = std::sqrt(moveDir.X * moveDir.X + moveDir.Z * moveDir.Z);
        if (moveLen > 0.0f) { moveDir.X = (moveDir.X / moveLen) * HORIZONTAL_SPEED; moveDir.Z = (moveDir.Z / moveLen) * HORIZONTAL_SPEED; }
        pos.X += moveDir.X * 0.016f;
        pos.Z += moveDir.Z * 0.016f;
        if (pos.Y > maxAllowedHeight) pos.Y = maxAllowedHeight;
        WriteZ<Vector3>(moveComp + Offsets::Position, pos);
        float vSpeed = HOVER_SPEED;
        if ((GetAsyncKeyState(0x20) & 0x8000) && pos.Y < maxAllowedHeight - 0.1f) vSpeed = UP_SPEED;
        else if (GetAsyncKeyState(0xA2) & 0x8000) vSpeed = DOWN_SPEED;
        WriteZ<float>(moveComp + Offsets::VSpeed, vSpeed);
    }

    static void Loop() {
        while (isRunning.load()) {
            try {
                if (!NoGravityFlyEnabled) { std::this_thread::sleep_for(std::chrono::milliseconds(40)); continue; }
                ApplyFly();
            } catch (...) {}
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void Start() {
        if (isRunning.load()) return;
        isRunning = true;
        heightInitialized = false;
        boostApplied = false;
        workThread = std::thread(Loop);
        workThread.detach();
    }
    void Stop() {
        isRunning = false;
        heightInitialized = false;
        boostApplied = false;
    }
    bool IsRunning() { return isRunning.load(); }
}
