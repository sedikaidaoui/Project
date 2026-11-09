#include "NoClip.hpp"
#include <cmath>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Math/Matrix4v4.hpp>
#include <EspLines/Offsets.hpp>
#include <src/Globals.hpp>

static Vector3 noclipLastPos = { 0, 0, 0 };
static bool noclipInitialized = false;
static bool noclipWasActive = false;

static bool ReadTransformPosition(uintptr_t localPlayer, Vector3& outPos) {
    uint32_t rootBonePtr = 0;
    if (!Mem.ReadFast2<uint32_t>(localPlayer + Offsets::Bones::Root, &rootBonePtr) || rootBonePtr == 0)
        return false;

    uint32_t transformPtr = 0;
    if (!Mem.ReadFast2<uint32_t>(rootBonePtr + 0x8, &transformPtr) || transformPtr == 0)
        return false;

    uint32_t transformObj = 0;
    if (!Mem.ReadFast2<uint32_t>(transformPtr + 0x8, &transformObj) || transformObj == 0)
        return false;

    uint32_t matrixValue = 0;
    if (!Mem.ReadFast2<uint32_t>(transformObj + 0x20, &matrixValue) || matrixValue == 0)
        return false;

    if (!Mem.ReadFast2<Vector3>(matrixValue + 0x60, &outPos))
        return false;

    return true;
}

static bool WriteTransformPosition(uintptr_t localPlayer, const Vector3& newPos) {
    uint32_t rootBonePtr = 0;
    if (!Mem.ReadFast2<uint32_t>(localPlayer + Offsets::Bones::Root, &rootBonePtr) || rootBonePtr == 0)
        return false;

    uint32_t transformPtr = 0;
    if (!Mem.ReadFast2<uint32_t>(rootBonePtr + 0x8, &transformPtr) || transformPtr == 0)
        return false;

    uint32_t transformObj = 0;
    if (!Mem.ReadFast2<uint32_t>(transformPtr + 0x8, &transformObj) || transformObj == 0)
        return false;

    uint32_t matrixValue = 0;
    if (!Mem.ReadFast2<uint32_t>(transformObj + 0x20, &matrixValue) || matrixValue == 0)
        return false;

    Mem.Write<Vector3>(matrixValue + 0x60, newPos);
    return true;
}

void NoClip::Run() {
    if (!g_Globals.Exploits.NoClip) {
        noclipInitialized = false;
        noclipWasActive = false;
        return;
    }

    uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;
    if (!localPlayer) return;

    bool keyPressed = (g_Globals.Exploits.NoClipBind != 0)
        ? (GetAsyncKeyState(g_Globals.Exploits.NoClipBind) & 0x8000) != 0
        : true;

    if (!keyPressed) {
        if (noclipWasActive) {
            noclipInitialized = false;
            noclipWasActive = false;
        }
        return;
    }

    Vector3 currentPos{};
    if (!ReadTransformPosition(localPlayer, currentPos))
        return;

    if (!noclipWasActive) {
        noclipLastPos = currentPos;
        noclipInitialized = true;
        noclipWasActive = true;
    }

    if (!g_Globals.EspConfig.Matrix) return;

    uint32_t moveComp = 0;
    if (Mem.ReadFast2<uint32_t>(localPlayer + Offsets::MovementComponent, &moveComp) && moveComp != 0) {
        uint8_t grounded = 0;
        Mem.Write<uint8_t>(localPlayer + Offsets::IsGrounded, grounded);
    }

    Matrix4x4& matrix = g_Globals.EspConfig.ViewMatrix;
    Vector3 forward(matrix.m02, matrix.m12, matrix.m22);
    Vector3 right(matrix.m00, matrix.m10, matrix.m20);

    Vector3 fwd = Vector3::Normalized(Vector3(forward.X, 0.0f, forward.Z));
    Vector3 rgt = Vector3::Normalized(Vector3(right.X, 0.0f, right.Z));

    Vector3 moveDir = Vector3::Zero();
    float speed = g_Globals.Exploits.NoClipSpeed;

    if (GetAsyncKeyState(0x57) & 0x8000) moveDir = moveDir + fwd;
    if (GetAsyncKeyState(0x53) & 0x8000) moveDir = moveDir - fwd;
    if (GetAsyncKeyState(0x44) & 0x8000) moveDir = moveDir + rgt;
    if (GetAsyncKeyState(0x41) & 0x8000) moveDir = moveDir - rgt;

    float moveLen = std::sqrt(moveDir.X * moveDir.X + moveDir.Z * moveDir.Z);
    if (moveLen > 0.0f) {
        moveDir.X = (moveDir.X / moveLen) * speed;
        moveDir.Z = (moveDir.Z / moveLen) * speed;
    }

    currentPos.X += moveDir.X * 0.016f;
    currentPos.Z += moveDir.Z * 0.016f;

    float vSpeed = 0.0f;
    if (GetAsyncKeyState(0x20) & 0x8000)
        vSpeed = speed;
    else if (GetAsyncKeyState(0xA2) & 0x8000)
        vSpeed = -speed;

    currentPos.Y += vSpeed * 0.016f;

    WriteTransformPosition(localPlayer, currentPos);

    if (moveComp != 0) {
        Mem.Write<Vector3>(moveComp + Offsets::Position, currentPos);
        Mem.Write<float>(moveComp + Offsets::VSpeed, vSpeed);
    }
}
