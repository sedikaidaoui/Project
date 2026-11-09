#include "Upplayer.hpp"

#include <src/Globals.hpp>

#include <EspLines/Player.h>

#include <EspLines/Memory/Memory.hpp>

#include <EspLines/Offsets.hpp>

#include <EspLines/Math/Vector/Vector3.hpp>

#include <EspLines/Math/Vector/Vector2.hpp>

#include <EspLines/Math/Quaternion.hpp>

#include <EspLines/Math/WordToScreen.hpp>

#include <Windows.h>

#include <thread>

#include <cmath>

#include <algorithm>

#include <atomic>

#include <unordered_map>

#include <imgui.h>

#include <imgui_internal.h>

#include <EspLines/Memory/Memory.hpp>

#include <EspLines/Math/Vector/Vector3.hpp>

#include <EspLines/Math/Vector/Vector2.hpp>

#include <EspLines/Aimbot/Aimbot.hpp> 

#include <src/Globals.hpp>

#include <EspLines/Offsets.hpp>

#include <EspLines/Math/WordToScreen.hpp>

#include <iostream>

#include <unordered_map>

#include <thread>

#include <atomic>

namespace UpPlayer {

    // ===== FUNÇÃO: Normalizar Vector3 manualmente =====

    inline Vector3 NormalizeVector3(const Vector3& vec) {

        float length = std::sqrt(vec.X * vec.X + vec.Y * vec.Y + vec.Z * vec.Z);

        if (length > 0.0f) {

            return Vector3(vec.X / length, vec.Y / length, vec.Z / length);

        }

        return Vector3::Zero();

    }



    // ===== FUNÇÃO: Obter direção de fogo da matriz da câmera =====

    inline Vector3 GetFireDirectionInternal() {

        try {

            auto& matrix = g_Globals.EspConfig.ViewMatrix;



            // Vetor forward da matriz da câmera (terceira coluna)

            Vector3 fireDirection(matrix.m02, matrix.m12, matrix.m22);



            return NormalizeVector3(fireDirection);

        }

        catch (const std::exception&) {

            return Vector3::Zero();

        }

    }



    namespace {

        std::atomic<bool> upPlayerRunning{ false };

        std::thread upPlayerThread;



        struct LockedData {

            Vector3 originalPosition;

            Vector3 lockedTargetPosition;

            float pullProgress;

        };



        std::unordered_map<uintptr_t, LockedData> lockedPositions;

        uintptr_t currentTargetId = 0;

        bool lastKeyState = false;

        bool isToggled = false;

        bool isTargetPositionLocked = false;

    }



    // ===== DECLARAÇÕES FORWARD =====

    Player* FindClosestEntity();

    void LockTargetPosition(Player* entity);

    void ForceEntityToLockedPosition();

    void RestoreOriginalPosition();



    // ===== FUNÇÃO: Obter osso alvo (fixo em Head) =====

    inline Vector3 GetTargetBone(Player* target) {

        if (!target) return Vector3::Zero();

        return target->Head;

    }



    // ===== FUNÇÃO: Validar posição 2D =====

    inline bool IsValidScreenPosition(const Vector2& pos) {

        return pos.X >= 1.0f && pos.Y >= 1.0f;

    }



    // ===== FUNÇÃO: Validar Vector3 =====

    inline bool IsValidVector3(const Vector3& vec) {

        if (vec == Vector3::Zero()) return false;

        if (std::isnan(vec.X) || std::isnan(vec.Y) || std::isnan(vec.Z)) return false;

        if (std::isinf(vec.X) || std::isinf(vec.Y) || std::isinf(vec.Z)) return false;

        if (std::abs(vec.X) > 100000.0f || std::abs(vec.Y) > 100000.0f || std::abs(vec.Z) > 100000.0f) return false;

        return true;

    }



    // ===== FUNÇÃO: Verificar se o jogador local está atirando =====

    bool IsLocalFiring() {

        try {

            uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;

            if (!localPlayer) return false;



            bool isFiring = false;

            if (!Mem.Read<bool>(localPlayer + 0x4A0, isFiring)) {

                return false;

            }

            return isFiring;

        }

        catch (const std::exception&) {

            return false;

        }

    }



    // ===== FUNÇÃO: Obter direção de fogo da matriz da câmera =====

    Vector3 GetFireDirection() {

        try {

            auto& matrix = g_Globals.EspConfig.ViewMatrix;



            // Vetor forward da matriz da câmera (terceira coluna)

            Vector3 fireDirection(matrix.m02, matrix.m12, matrix.m22);



            return NormalizeVector3(fireDirection);

        }

        catch (const std::exception&) {

            return Vector3::Zero();

        }

    }



    bool UpPlayerr::IsActiveUpPlayer() {

        return upPlayerRunning && isToggled;

    }



    // ===== FUNÇÃO: Verificar se o jogador local está atirando =====

    bool UpPlayerr::IsLocalFiring() {

        try {

            uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;

            if (!localPlayer) return false;



            bool isFiring = false;

            if (!Mem.Read<bool>(localPlayer + 0x4A0, isFiring)) {

                return false;

            }

            return isFiring;

        }

        catch (const std::exception&) {

            return false;

        }

    }



    // ===== FUNÇÃO: Restaurar posição original =====

    void RestoreOriginalPosition() {

        if (!isTargetPositionLocked || currentTargetId == 0) return;



        auto it = lockedPositions.find(currentTargetId);

        if (it == lockedPositions.end()) return;



        try {

            uint32_t rootBone = 0;

            if (!Mem.Read<uint32_t>(currentTargetId + Offsets::Bones::Root, rootBone) || rootBone == 0) {

                return;

            }



            uint32_t transform1 = 0, transform2 = 0, matrix = 0;

            if (!Mem.Read<uint32_t>(rootBone + 0x8, transform1) || transform1 == 0) return;

            if (!Mem.Read<uint32_t>(transform1 + 0x8, transform2) || transform2 == 0) return;

            if (!Mem.Read<uint32_t>(transform2 + 0x20, matrix) || matrix == 0) return;



            Mem::Write<Vector3>(matrix + 0x60, it->second.originalPosition);

        }

        catch (const std::exception& e) {

            std::cerr << "[UpPlayer ERROR - RestoreOriginalPosition] " << e.what() << std::endl;

        }



        isTargetPositionLocked = false;

        currentTargetId = 0;

        lockedPositions.clear();

    }



    // ===== FUNÇÃO: Encontrar entidade mais próxima =====

    Player* FindClosestEntity() {

        if (g_Globals.EspConfig.Width == -1 ||

            g_Globals.EspConfig.Height == -1 ||

            !g_Globals.EspConfig.Matrix) {

            return nullptr;

        }



        Player* closestEntity = nullptr;

        float bestDist = FLT_MAX;

        Vector2 screenCenter(

            g_Globals.EspConfig.Width / 2.0f,

            g_Globals.EspConfig.Height / 2.0f

        );



        for (auto& pair : g_Globals.EspConfig.Entities) {

            Player& entity = pair.second;

            if (!entity.IsKnown || entity.IsDead ||

                (g_Globals.AimBot.IgnoreKnocked && entity.IsKnocked)) {

                continue;

            }



            Vector3 targetBone = GetTargetBone(&entity);

            if (!IsValidVector3(targetBone)) continue;



            float distance = Vector3::Distance(g_Globals.EspConfig.MainCamera, targetBone);

            if (std::isnan(distance) || std::isinf(distance) || distance <= 0.0f) continue;

            if (distance > g_Globals.AimBot.DistanceAim) continue;



            Vector2 screenPos = W2S::WorldToScreen(

                g_Globals.EspConfig.ViewMatrix,

                targetBone,

                g_Globals.EspConfig.Width,

                g_Globals.EspConfig.Height

            );



            if (!IsValidScreenPosition(screenPos)) continue;

            if (std::isnan(screenPos.X) || std::isnan(screenPos.Y)) continue;



            float dist = Vector2::Distance(screenCenter, screenPos);



            if (dist < bestDist) {

                bestDist = dist;

                closestEntity = &entity;

            }

        }



        return closestEntity;

    }



    // ===== FUNÇÃO: Travar posição do alvo =====

    void LockTargetPosition(Player* entity) {

        if (!entity || entity->Address == 0) return;



        currentTargetId = entity->Address;



        try {

            uint32_t rootBone = 0;

            if (!Mem.Read<uint32_t>(entity->Address + Offsets::Bones::Root, rootBone) || rootBone == 0) {

                return;

            }



            uint32_t transform1 = 0, transform2 = 0, matrix = 0;

            if (!Mem.Read<uint32_t>(rootBone + 0x8, transform1) || transform1 == 0) return;

            if (!Mem.Read<uint32_t>(transform1 + 0x8, transform2) || transform2 == 0) return;

            if (!Mem.Read<uint32_t>(transform2 + 0x20, matrix) || matrix == 0) return;



            Vector3 targetPos;

            if (!Mem.Read<Vector3>(matrix + 0x60, targetPos)) {

                return;

            }



            if (!IsValidVector3(targetPos)) return;



            // Armazena posição original se ainda não estiver armazenada

            if (lockedPositions.find(entity->Address) == lockedPositions.end()) {

                LockedData data;

                data.originalPosition = targetPos;

                data.pullProgress = 0.0f;

                lockedPositions[entity->Address] = data;

            }



            isTargetPositionLocked = true;

        }

        catch (const std::exception& e) {

            std::cerr << "[UpPlayer ERROR - LockTargetPosition] " << e.what() << std::endl;

        }

    }



    // ===== FUNÇÃO: Forçar entidade para posição travada =====

    void ForceEntityToLockedPosition() {

        if (currentTargetId == 0 || lockedPositions.find(currentTargetId) == lockedPositions.end()) {

            RestoreOriginalPosition();

            return;

        }



        try {

            // 🎯 Busca a entidade atual na lista de entidades

            Player* currentTarget = nullptr;

            for (auto& pair : g_Globals.EspConfig.Entities) {

                if (pair.second.Address == currentTargetId) {

                    currentTarget = &pair.second;

                    break;

                }

            }



            if (!currentTarget) {

                RestoreOriginalPosition();

                return;

            }



            // 🎯 Pega a posição da HEAD do alvo

            Vector3 targetHeadPos = currentTarget->Head;

            if (!IsValidVector3(targetHeadPos)) {

                RestoreOriginalPosition();

                return;

            }



            uint32_t rootBone = 0;

            if (!Mem.Read<uint32_t>(currentTargetId + Offsets::Bones::Root, rootBone) || rootBone == 0) {

                return;

            }



            uint32_t transform1 = 0, transform2 = 0, matrix = 0;

            if (!Mem.Read<uint32_t>(rootBone + 0x8, transform1) || transform1 == 0) return;

            if (!Mem.Read<uint32_t>(transform1 + 0x8, transform2) || transform2 == 0) return;

            if (!Mem.Read<uint32_t>(transform2 + 0x20, matrix) || matrix == 0) return;



            Vector3 currentPos;

            if (!Mem.Read<Vector3>(matrix + 0x60, currentPos)) {

                return;

            }



            LockedData& lockedData = lockedPositions[currentTargetId];



            // 🔥 Direção de firo da matriz da câmera (vetor forward)

            Vector3 fireDirection = GetFireDirectionInternal();

            if (!IsValidVector3(fireDirection)) return;



            // Posição da câmera (origem da linha)

            Vector3 camPos = g_Globals.EspConfig.MainCamera;



            // 📌 Projeção da HEAD na linha de fogo

            Vector3 toHead = Vector3(

                targetHeadPos.X - camPos.X,

                targetHeadPos.Y - camPos.Y,

                targetHeadPos.Z - camPos.Z

            );



            float projLength = (toHead.X * fireDirection.X) +

                (toHead.Y * fireDirection.Y) +

                (toHead.Z * fireDirection.Z);



            Vector3 linePoint = Vector3(

                camPos.X + fireDirection.X * projLength,

                camPos.Y + fireDirection.Y * projLength,

                camPos.Z + fireDirection.Z * projLength

            );



            // 🎯 Calcula offset da HEAD para o RootBone

            Vector3 headOffset = Vector3(

                targetHeadPos.X - currentPos.X,

                targetHeadPos.Y - currentPos.Y,

                targetHeadPos.Z - currentPos.Z

            );



            // Pull suave

            lockedData.pullProgress += 0.025f;

            float pullStrength = (std::min)(lockedData.pullProgress * 0.5f, 1.0f);



            // 🎯 Puxa o RootBone para que a HEAD fique na linha de fogo

            Vector3 desiredRootPos = Vector3(

                linePoint.X - headOffset.X,

                linePoint.Y - headOffset.Y,

                linePoint.Z - headOffset.Z

            );



            Vector3 pulledPosition = Vector3(

                currentPos.X + (desiredRootPos.X - currentPos.X) * pullStrength,

                currentPos.Y + (desiredRootPos.Y - currentPos.Y) * pullStrength,

                currentPos.Z + (desiredRootPos.Z - currentPos.Z) * pullStrength

            );



            // 📏 Limita distância máxima de pull (3D)

            float distance = std::sqrt(

                std::pow(pulledPosition.X - lockedData.originalPosition.X, 2) +

                std::pow(pulledPosition.Y - lockedData.originalPosition.Y, 2) +

                std::pow(pulledPosition.Z - lockedData.originalPosition.Z, 2)

            );



            float maxDistance = 15.0f;



            if (distance > maxDistance) {

                // Muito longe → reseta para posição original

                pulledPosition = lockedData.originalPosition;

            }



            Mem::Write<Vector3>(matrix + 0x60, pulledPosition);

        }

        catch (const std::exception& e) {

            std::cerr << "[UpPlayer ERROR - ForceEntityToLockedPosition] " << e.what() << std::endl;

        }

    }



    void UpPlayerr::UpPlayer() {

        if (upPlayerRunning) return;

        upPlayerRunning = true;



        upPlayerThread = std::thread([] {

            while (upPlayerRunning) {

                try {

                    // Detecta toggle da tecla

                    bool currentKeyState = (GetAsyncKeyState(g_Globals.Exploits.UpPlayerBind) & 0x8000) != 0;

                    if (currentKeyState && !lastKeyState) {

                        isToggled = !isToggled;

                        if (!isToggled) {

                            RestoreOriginalPosition();

                        }

                    }

                    lastKeyState = currentKeyState;



                    // Se não estiver habilitado ou toggleado, apenas limpa e continua

                    if (!g_Globals.Exploits.UpPlayer || !isToggled) {

                        if (isTargetPositionLocked) {

                            RestoreOriginalPosition();

                        }

                        std::this_thread::sleep_for(std::chrono::milliseconds(100));

                        continue;

                    }



                    // Validações básicas

                    if (g_Globals.EspConfig.Width == -1 ||

                        g_Globals.EspConfig.Height == -1 ||

                        !g_Globals.EspConfig.Matrix) {

                        std::this_thread::sleep_for(std::chrono::milliseconds(10));

                        continue;

                    }



                    uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;

                    if (!localPlayer) {

                        std::this_thread::sleep_for(std::chrono::milliseconds(10));

                        continue;

                    }



                    // 🔥 Só puxa quando estiver atirando

                    if (UpPlayerr::IsLocalFiring()) {

                        // Se não temos alvo travado, procura um novo

                        if (!isTargetPositionLocked) {

                            Player* closestEntity = FindClosestEntity();

                            if (closestEntity != nullptr) {

                                LockTargetPosition(closestEntity);

                            }

                        }



                        // Se temos alvo travado, força a posição

                        if (isTargetPositionLocked) {

                            ForceEntityToLockedPosition();

                        }

                    }

                    else {

                        // Não está atirando → restaura tudo

                        if (isTargetPositionLocked) {

                            RestoreOriginalPosition();

                        }

                    }



                    std::this_thread::sleep_for(std::chrono::milliseconds(6));

                }

                catch (const std::exception& e) {

                    std::cerr << "[UpPlayer ERROR] " << e.what() << std::endl;

                }

            }



            // Cleanup ao finalizar

            RestoreOriginalPosition();

            });



        upPlayerThread.detach();

    }



    void UpPlayerr::StopUpPlayer() {

        if (!upPlayerRunning) return;



        upPlayerRunning = false;

        isToggled = false;

        RestoreOriginalPosition();

    }



    void UpPlayerr::RestoreAllPositions() {

        RestoreOriginalPosition();

    }

}