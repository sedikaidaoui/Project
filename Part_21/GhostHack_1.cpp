#include <imgui.h>
#include <imgui_internal.h>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <src/Globals.hpp>
#include <EspLines/Offsets.hpp>
#include <unordered_map>
#include <iostream>
#include <atomic>
#include <thread>
#include <optional>
#include "GhostHack.hpp"

std::atomic<bool> ghostHackRunning{ false };
std::thread ghostHackThread;
static std::optional<Vector3> originalPosition = std::nullopt;
static bool lastKeyState = false;
static bool isToggled = false;

bool GhostHack::GhostHack::IsActive() {
    return ghostHackRunning && isToggled && originalPosition.has_value();
}

std::optional<Vector3> GhostHack::GhostHack::GetGhostBodyPosition() {
    return originalPosition;
}

void GhostHack::GhostHack::Start() {
    if (ghostHackRunning) return;
    ghostHackRunning = true;

    ghostHackThread = std::thread([] {
        while (ghostHackRunning) {
            try {
                bool currentKeyState = (GetAsyncKeyState(g_Globals.Exploits.GhostBind) & 0x8000) != 0;
                if (currentKeyState && !lastKeyState) {
                    isToggled = !isToggled;
                    if (!isToggled && originalPosition.has_value()) {
                        Mem.Write<Vector3>(g_Globals.EspConfig.LocalPlayer + Offsets::GhostMode, originalPosition.value());
                        originalPosition = std::nullopt;
                    }
                }
                lastKeyState = currentKeyState;
                if (g_Globals.Exploits.Ghost && isToggled) {
                    bool foundTarget = false;
                    for (const auto& pair : g_Globals.EspConfig.Entities) {
                        const Player& entity = pair.second;
                        if (!entity.IsKnown || entity.IsDead ||
                            (g_Globals.AimBot.IgnoreKnocked && entity.IsKnocked)) {
                            continue;
                        }

                        float distance = Vector3::Distance(g_Globals.EspConfig.MainCamera, entity.Head);
                        if (distance > g_Globals.AimBot.DistanceAim) continue;
                        uint32_t rootBone;
                        if (!Mem.ReadFast2<uint32_t>(entity.Address + Offsets::Bones::Root, &rootBone) || rootBone == 0) {
                            continue;
                        }
                        uint32_t transform1, transform2, matrix;
                        Vector3 targetPosition;

                        if (!Mem.ReadFast2<uint32_t>(rootBone + 0x8, &transform1) || transform1 == 0) continue;
                        if (!Mem.ReadFast2<uint32_t>(transform1 + 0x8, &transform2) || transform2 == 0) continue;
                        if (!Mem.ReadFast2<uint32_t>(transform2 + 0x20, &matrix) || matrix == 0) continue;
                        if (!Mem.ReadFast2<Vector3>(matrix + 0x60, &targetPosition)) continue;

                        if (!originalPosition.has_value()) {
                            Vector3 currentPos;
                            if (Mem.ReadFast2<Vector3>(g_Globals.EspConfig.LocalPlayer + Offsets::GhostMode, &currentPos)) {
                                originalPosition = currentPos;
                            }
                        }

                        Mem.Write<Vector3>(g_Globals.EspConfig.LocalPlayer + Offsets::GhostMode, targetPosition);
                        foundTarget = true;
                        break; 
                    }

                    if (!foundTarget && originalPosition.has_value()) {
                        Mem.Write<Vector3>(g_Globals.EspConfig.LocalPlayer + Offsets::GhostMode, originalPosition.value());
                        originalPosition = std::nullopt;
                    }
                }
                else if (originalPosition.has_value()) {
                    Mem.Write<Vector3>(g_Globals.EspConfig.LocalPlayer + Offsets::GhostMode, originalPosition.value());
                    originalPosition = std::nullopt;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            catch (const std::exception& e) {
                std::cerr << "GhostHack exception: " << e.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        if (originalPosition.has_value()) {
            Mem.Write<Vector3>(g_Globals.EspConfig.LocalPlayer + Offsets::GhostMode, originalPosition.value());
            originalPosition = std::nullopt;
        }
        });

    ghostHackThread.detach();
}

void GhostHack::GhostHack::Stop() {
    if (!ghostHackRunning) return;

    ghostHackRunning = false;
    isToggled = false;

    if (originalPosition.has_value()) {
        Mem.Write<Vector3>(g_Globals.EspConfig.LocalPlayer + Offsets::GhostMode, originalPosition.value());
        originalPosition = std::nullopt;
    }

    if (ghostHackThread.joinable()) {
        ghostHackThread.join();
    }
}

bool GhostHack::GhostHack::IsToggled() {
    return isToggled;
}

void GhostHack::GhostHack::SetToggle(bool state) {
    isToggled = state;

    // Se desativou, restaurar posi��o
    if (!state && originalPosition.has_value()) {
        Mem.Write<Vector3>(g_Globals.EspConfig.LocalPlayer + Offsets::GhostMode, originalPosition.value());
        originalPosition = std::nullopt;
    }
}