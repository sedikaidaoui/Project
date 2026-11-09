#pragma once
#include "Math/Vector/Vector3.hpp"
#include <string>
#ifndef BOOL3_H
#define BOOL3_H

class Player {
public:
    enum class Bool3 { True, False, Unknown };
    bool IsBot;
    bool IsKnown;
    bool IsDead;
    bool IsKnocked;
    bool IsVisible;
    bool IsWukong;
    bool IsProne;
    
    float Distance;
    short Health;
    short WeaponID;
    short Pose;
    Bool3 IsTeam;
    uint32_t Address;
    uint64_t ID;
    int Level;
    int CurrentShield;
    uint32_t CachedColliderAddress; // The address where we write the lock
    uint32_t HeadColliderValue;     // The head collider ID
    uint32_t ChestColliderValue;    // The chest collider ID
    int ShotsFiredAtTarget = 0;
    std::string Name;

    Vector3 Head;
    Vector3 Neck;
    Vector3 Spine;
    Vector3 RightShoulder;
    Vector3 LeftShoulder;
    Vector3 RightElbow;
    Vector3 LeftElbow;
    Vector3 RightWrist;
    Vector3 LeftWrist;
    Vector3 Hip;
    Vector3 Groin;          // Virilha — connects Hip to legs
    Vector3 RightAnkle;
    Vector3 LeftAnkle;
    Vector3 LeftFoot;       // Pé esquerdo
    Vector3 RightFoot;      // Pé direito
    Vector3 LeftHand;       // Mão esquerda
    Vector3 RightHand;      // Mão direita
    Vector3 Root;
};

#endif
