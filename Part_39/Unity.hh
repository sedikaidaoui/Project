#pragma once
#include "Vector2.hpp"
#include "Vector3.hpp"
#include "Quaternion.hpp"
#include "bool3.h"

struct MathZ {
    static float Abs(float value) {
        return value < 0.0f ? -value : value;
    }

    static Vector2 Abs(const Vector2& v) {
        return Vector2(Abs(v.X), Abs(v.Y));
    }
};

struct Vector4
{
    float x, y, z, w;

    Vector4() : x(0), y(0), z(0), w(0) {}

    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    void print() const {
        std::cout << "Vector4(" << x << ", " << y << ", " << z << ", " << w << ")\n";
    }
};

struct Matrix4x4 {
    float m00, m01, m02, m03;
    float m10, m11, m12, m13;
    float m20, m21, m22, m23;
    float m30, m31, m32, m33;

    Matrix4x4()
        : m00(0), m01(0), m02(0), m03(0),
        m10(0), m11(0), m12(0), m13(0),
        m20(0), m21(0), m22(0), m23(0),
        m30(0), m31(0), m32(0), m33(0)
    {
    }
};

// Esta función convierte las coordenadas del mundo a las coordenadas de pantalla
Vector2 WorldToScreen(const Matrix4x4& viewMatrix, const Vector3& pos, int width, int height) {
    Vector2 result(-1, -1);

    float v9 = (pos.X * viewMatrix.m00) + (pos.Y * viewMatrix.m10) + (pos.Z * viewMatrix.m20) + viewMatrix.m30;
    float v10 = (pos.X * viewMatrix.m01) + (pos.Y * viewMatrix.m11) + (pos.Z * viewMatrix.m21) + viewMatrix.m31;
    float v12 = (pos.X * viewMatrix.m03) + (pos.Y * viewMatrix.m13) + (pos.Z * viewMatrix.m23) + viewMatrix.m33;

    if (v12 >= 0.001f) {
        float v13 = static_cast<float>(width) / 2.0f;
        float v14 = static_cast<float>(height) / 2.0f;

        result.X = v13 + (v13 * v9) / v12;
        result.Y = v14 - (v14 * v10) / v12;
    }

    return result;
}

// Esta función convierte las coordenadas del mundo a las coordenadas de pantalla
ImVec2 WorldToScreenImVec2(const Matrix4x4& viewMatrix, const Vector3& pos, int width, int height) {
    Vector2 result = WorldToScreen(viewMatrix, pos, width, height);
    return ImVec2(result.X, result.Y);  // Convertir Vector2 a ImVec2
}

class AimBZ {
private:
    static constexpr float SMALL_FLOAT = 0.0000000001f;

public:
    static Quaternion GetRotationToLocation(const Vector3& targetLocation,
        float y_bias, const Vector3& myLoc) {
        Vector3 forwards = targetLocation + Vector3(0, y_bias, 0) - myLoc;
        Vector3 upwards(0, 1, 0);

        return LookRotation(forwards, upwards);
    }

private:
    static Quaternion LookRotation(Vector3 forwards, Vector3 upwards) {
        forwards = Normalized(forwards);
        upwards = Normalized(upwards);

        if (SqrMagnitude(forwards) < SMALL_FLOAT || SqrMagnitude(upwards) < SMALL_FLOAT)
            return Quaternion::Identity();

        if (1 - std::abs(Vector3::Dot(forwards, upwards)) < SMALL_FLOAT)
            return FromToRotation(forwards, upwards);

        Vector3 right = Normalized(Vector3::Cross(upwards, forwards));
        upwards = Vector3::Cross(forwards, right);

        Quaternion quaternion;
        float radicand = right.X + upwards.Y + forwards.Z;

        if (radicand > 0) {
            quaternion.W = std::sqrt(1.0f + radicand) * 0.5f;
            float recip = 1.0f / (4.0f * quaternion.W);
            quaternion.X = (upwards.Z - forwards.Y) * recip;
            quaternion.Y = (forwards.X - right.Z) * recip;
            quaternion.Z = (right.Y - upwards.X) * recip;
        }
        else if (right.X >= upwards.Y && right.X >= forwards.Z) {
            quaternion.X = std::sqrt(1.0f + right.X - upwards.Y - forwards.Z) * 0.5f;
            float recip = 1.0f / (4.0f * quaternion.X);
            quaternion.W = (upwards.Z - forwards.Y) * recip;
            quaternion.Z = (forwards.X + right.Z) * recip;
            quaternion.Y = (right.Y + upwards.X) * recip;
        }
        else if (upwards.Y > forwards.Z) {
            quaternion.Y = std::sqrt(1.0f - right.X + upwards.Y - forwards.Z) * 0.5f;
            float recip = 1.0f / (4.0f * quaternion.Y);
            quaternion.Z = (upwards.Z + forwards.Y) * recip;
            quaternion.W = (forwards.X - right.Z) * recip;
            quaternion.X = (right.Y + upwards.X) * recip;
        }
        else {
            quaternion.Z = std::sqrt(1.0f - right.X - upwards.Y + forwards.Z) * 0.5f;
            float recip = 1.0f / (4.0f * quaternion.Z);
            quaternion.Y = (upwards.Z + forwards.Y) * recip;
            quaternion.X = (forwards.X + right.Z) * recip;
            quaternion.W = (right.Y - upwards.X) * recip;
        }
        return quaternion;
    }

    static Quaternion FromToRotation(const Vector3& forwards, const Vector3& upwards) {
        float dot = Vector3::Dot(forwards, upwards);
        float k = std::sqrt(SqrMagnitude(forwards) * SqrMagnitude(upwards));

        if (std::abs(dot / k + 1) < 0.00001) {
            Vector3 ortho = Orthogonal(forwards);
            return Quaternion(Normalized(ortho).X, Normalized(ortho).Y, Normalized(ortho).Z, 0);
        }
        Vector3 cross = Vector3::Cross(forwards, upwards);
        return Normalized(Quaternion(cross.X, cross.Y, cross.Z, dot + k));
    }

    static Quaternion Normalized(const Quaternion& rotation) {
        float norm = Norm(rotation);
        return Quaternion(rotation.X / norm, rotation.Y / norm, rotation.Z / norm, rotation.W / norm);
    }

    static float Norm(const Quaternion& rotation) {
        return std::sqrt(rotation.X * rotation.X +
            rotation.Y * rotation.Y +
            rotation.Z * rotation.Z +
            rotation.W * rotation.W);
    }

    static Vector3 Orthogonal(const Vector3& v) {
        return v.Z < v.X ? Vector3(v.Y, -v.X, 0) : Vector3(0, -v.Z, v.Y);
    }

    static Vector3 Normalized(const Vector3& vector) {
        float mag = Magnitude(vector);

        if (mag == 0)
            return Vector3::Zero();
        return vector / mag;
    }

    static float Magnitude(const Vector3& vector) {
        return std::sqrt(SqrMagnitude(vector));
    }

    static float SqrMagnitude(const Vector3& v) {
        return v.X * v.X + v.Y * v.Y + v.Z * v.Z;
    }
};


#include <windows.h> // Para VirtualProtect y IsDebuggerPresent
#include <thread>    // Para sleep_for
#include <chrono>    // Para temporizadores

class AimBZv2 {
private:
    static constexpr float SMALL_FLOAT = 0.0000000001f;

public:
    static Quaternion GetRotationToLocation(const Vector3& targetLocation,
        float y_bias, const Vector3& myLoc) {
        Vector3 direction = targetLocation - myLoc;
        direction.Y += y_bias;  // Aplica el sesgo en Y si es necesario
        direction = Normalized(direction);

        Vector3 upwards(0, 1, 0);

        return LookRotation(direction, upwards);
    }

    // Mueve la función Normalized a la sección public para que sea accesible
    static Vector3 Normalized(const Vector3& vector) {
        float mag = Magnitude(vector);
        return (mag == 0) ? Vector3::Zero() : vector / mag;
    }

private:
    static Quaternion LookRotation(Vector3 direction, Vector3 upwards) {
        direction = Normalized(direction);
        upwards = Normalized(upwards);

        if (SqrMagnitude(direction) < SMALL_FLOAT || SqrMagnitude(upwards) < SMALL_FLOAT)
            return Quaternion::Identity();

        if (1 - std::abs(Vector3::Dot(direction, upwards)) < SMALL_FLOAT)
            return FromToRotation(direction, upwards);

        Vector3 right = Normalized(Vector3::Cross(upwards, direction));
        upwards = Vector3::Cross(direction, right);

        Quaternion quaternion;
        float radicand = right.X + upwards.Y + direction.Z;

        if (radicand > 0) {
            quaternion.W = std::sqrt(1.0f + radicand) * 0.5f;
            float recip = 1.0f / (4.0f * quaternion.W);
            quaternion.X = (upwards.Z - direction.Y) * recip;
            quaternion.Y = (direction.X - right.Z) * recip;
            quaternion.Z = (right.Y - upwards.X) * recip;
        }
        else if (right.X >= upwards.Y && right.X >= direction.Z) {
            quaternion.X = std::sqrt(1.0f + right.X - upwards.Y - direction.Z) * 0.5f;
            float recip = 1.0f / (4.0f * quaternion.X);
            quaternion.W = (upwards.Z - direction.Y) * recip;
            quaternion.Z = (direction.X + right.Z) * recip;
            quaternion.Y = (right.Y + upwards.X) * recip;
        }
        else if (upwards.Y > direction.Z) {
            quaternion.Y = std::sqrt(1.0f - right.X + upwards.Y - direction.Z) * 0.5f;
            float recip = 1.0f / (4.0f * quaternion.Y);
            quaternion.Z = (upwards.Z + direction.Y) * recip;
            quaternion.W = (direction.X - right.Z) * recip;
            quaternion.X = (right.Y + upwards.X) * recip;
        }
        else {
            quaternion.Z = std::sqrt(1.0f - right.X - upwards.Y + direction.Z) * 0.5f;
            float recip = 1.0f / (4.0f * quaternion.Z);
            quaternion.Y = (upwards.Z + direction.Y) * recip;
            quaternion.X = (direction.X + right.Z) * recip;
            quaternion.W = (right.Y - upwards.X) * recip;
        }
        return quaternion;
    }

    static Quaternion FromToRotation(const Vector3& from, const Vector3& to) {
        float dot = Vector3::Dot(from, to);
        float k = std::sqrt(SqrMagnitude(from) * SqrMagnitude(to));

        if (std::abs(dot / k + 1) < SMALL_FLOAT) {
            Vector3 ortho = Orthogonal(from);
            return Quaternion(Normalized(ortho).X, Normalized(ortho).Y, Normalized(ortho).Z, 0);
        }
        Vector3 cross = Vector3::Cross(from, to);
        return Normalized(Quaternion(cross.X, cross.Y, cross.Z, dot + k));
    }

    static Quaternion Normalized(const Quaternion& rotation) {
        float norm = Norm(rotation);
        return Quaternion(rotation.X / norm, rotation.Y / norm, rotation.Z / norm, rotation.W / norm);
    }

    static float Norm(const Quaternion& rotation) {
        return std::sqrt(rotation.X * rotation.X +
            rotation.Y * rotation.Y +
            rotation.Z * rotation.Z +
            rotation.W * rotation.W);
    }

    static Vector3 Orthogonal(const Vector3& v) {
        return v.Z < v.X ? Vector3(v.Y, -v.X, 0) : Vector3(0, -v.Z, v.Y);
    }

    static float Magnitude(const Vector3& vector) {
        return std::sqrt(SqrMagnitude(vector));
    }

    static float SqrMagnitude(const Vector3& v) {
        return v.X * v.X + v.Y * v.Y + v.Z * v.Z;
    }
};