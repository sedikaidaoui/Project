#pragma once

#include <imgui.h>
#include "Vector/Vector2.hpp"
#include "Vector/Vector3.hpp"
#include "Matrix4v4.hpp"
#include <src/Globals.hpp>
#include <cmath>

class W2S {
public:
    static Vector2 WorldToScreen(const Matrix4x4& viewMatrix, const Vector3& pos, int width, int height) {
        Vector2 result(-1, -1);

        if (width <= 0 || height <= 0) {
            return result;
        }

        float v9 = (pos.X * viewMatrix.m00) + (pos.Y * viewMatrix.m10) + (pos.Z * viewMatrix.m20) + viewMatrix.m30;
        float v10 = (pos.X * viewMatrix.m01) + (pos.Y * viewMatrix.m11) + (pos.Z * viewMatrix.m21) + viewMatrix.m31;
        float v12 = (pos.X * viewMatrix.m03) + (pos.Y * viewMatrix.m13) + (pos.Z * viewMatrix.m23) + viewMatrix.m33;

        if (v12 >= 0.01f) {
            float v13 = static_cast<float>(width) / 2.0f;
            float v14 = static_cast<float>(height) / 2.0f;

            result.X = v13 + (v13 * v9) / v12;
            result.Y = v14 - (v14 * v10) / v12;
        } else {
            result.X = -99999.0f;
            result.Y = -99999.0f;
        }

        return result;
    }

    static ImVec2 WorldToScreenImVec2(const Matrix4x4& viewMatrix, const Vector3& pos, int width, int height) {
        Vector2 result = WorldToScreen(viewMatrix, pos, width, height);
        return ImVec2(result.X, result.Y);
    }

    static Vector3 ScreenToWorld(const Vector2& screenPos, float distance) {
        Vector3 result(0, 0, 0);
        if (distance <= 0) return result;

        int width = g_Globals.EspConfig.Width;
        int height = g_Globals.EspConfig.Height;
        if (width <= 0 || height <= 0) return result;

        float ndcX = (2.0f * screenPos.X) / width - 1.0f;
        float ndcY = 1.0f - (2.0f * screenPos.Y) / height;

        Vector3 cameraPos = g_Globals.EspConfig.MainCamera;
        Matrix4x4 viewMatrix = g_Globals.EspConfig.ViewMatrix;

        Vector3 cameraRight = Vector3::Normalized(Vector3(viewMatrix.m00, viewMatrix.m10, viewMatrix.m20));
        Vector3 cameraUp = Vector3::Normalized(Vector3(viewMatrix.m01, viewMatrix.m11, viewMatrix.m21));
        Vector3 cameraForward = Vector3::Normalized(Vector3(viewMatrix.m02, viewMatrix.m12, viewMatrix.m22));

        const float halfFov = 90.0f * (3.14159265f / 180.0f) * 0.5f;
        float worldX = ndcX * tanf(halfFov);
        float worldY = ndcY * tanf(halfFov);

        Vector3 worldDirection = cameraRight * worldX + cameraUp * worldY + cameraForward;
        worldDirection = Vector3::Normalized(worldDirection);
        return cameraPos + worldDirection * distance;
    }
};

//float v9 = (pos.X * viewMatrix.m00) + (pos.Y * viewMatrix.m10) + (pos.Z * viewMatrix.m20) + viewMatrix.m30;
//float v10 = (pos.X * viewMatrix.m01) + (pos.Y * viewMatrix.m11) + (pos.Z * viewMatrix.m21) + viewMatrix.m31;
//float v12 = (pos.X * viewMatrix.m03) + (pos.Y * viewMatrix.m13) + (pos.Z * viewMatrix.m23) + viewMatrix.m33;
