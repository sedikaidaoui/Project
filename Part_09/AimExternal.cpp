#include "AimExternal.hpp"
#include "Offset.hpp"
#include "Unity.hh"
#include "Complementos.h"
#include <Windows.h>
#include <algorithm>
#include <cfloat>
#include <cmath>

// globals from Complementos.h
extern int g_windowWidth;
extern int g_windowHeight;

namespace AimExternal {
    std::atomic<bool> g_aimbotExternalBusy{false};
    std::atomic<bool> g_aimbotExternalPatched{false};
    bool prev_aimbot_memory_state = false;
    std::thread aimbotThread2;
    std::atomic<bool> aimbotRunning2{false};

    // persistent storage
    static std::unordered_map<uintptr_t, PersistentEntityCache> entityDatabase;
    static std::unordered_map<uintptr_t, bool> entityDeadStatus;
    static std::unordered_map<uintptr_t, bool> entityModifiedStatus;
    static uintptr_t currentTarget = 0;
    static std::mt19937 gen{std::random_device{}()};
    static bool wasKeyPressed = false;
    static std::chrono::steady_clock::time_point lastWriteTime;

    static void CleanupAllEntities() {
        for (auto& [addr, cache] : entityDatabase) {
            if (entityModifiedStatus[addr]) {
                WriteZ<uint32_t>(cache.hipAddr, cache.originalHip);
            }
        }
        entityDatabase.clear();
        entityDeadStatus.clear();
        entityModifiedStatus.clear();
        currentTarget = 0;
    }
    static void RestoreEntity(uintptr_t addr) {
        auto it = entityDatabase.find(addr);
        if (it != entityDatabase.end() && entityModifiedStatus[addr]) {
            WriteZ<uint32_t>(it->second.hipAddr, it->second.originalHip);
            entityModifiedStatus[addr] = false;
        }
    }
    static void RestoreDeadEntities() {
        for (auto& [addr, isDead] : entityDeadStatus) {
            if (isDead && entityModifiedStatus[addr]) {
                auto it = entityDatabase.find(addr);
                if (it != entityDatabase.end()) {
                    WriteZ<uint32_t>(it->second.hipAddr, it->second.originalHip);
                    entityModifiedStatus[addr] = false;
                }
            }
        }
    }

    void ExternalAimbotTick() {
        // This is called from the dedicated thread, but we need to access the current entity list.
        // For NINJA X we will do a minimal version: find best target via current loop is not available here,
        // so we will scan the entityDictionary directly similar to main loop.

        // If Silent not enabled, cleanup
        if (!AimbotRage) {
            if (!entityDatabase.empty()) CleanupAllEntities();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return;
        }

        if (g_windowWidth <= 0 || g_windowHeight <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            return;
        }

        // Input checks - use VK_LBUTTON as bind
        bool keyDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        bool keyJustReleased = (!keyDown && wasKeyPressed);
        wasKeyPressed = keyDown;

        CURSORINFO ci{}; ci.cbSize = sizeof(CURSORINFO);
        bool cursorVisible = true;
        if (GetCursorInfo(&ci)) cursorVisible = (ci.flags & CURSOR_SHOWING) != 0;
        bool shouldAim = (keyDown && !cursorVisible);
        if (keyJustReleased && currentTarget) { RestoreEntity(currentTarget); currentTarget = 0; }
        if (!shouldAim) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastWriteTime).count();
            if (elapsed > 10 && currentTarget) { RestoreEntity(currentTarget); currentTarget = 0; }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            return;
        }

        // Find best target - we need to iterate over entityDictionary
        // For simplicity, we will use a global copy of entities that main thread updates.
        // Here we do a direct memory scan similar to main.cpp
        uint32_t baseGameFacade = 0, gameFacade = 0, staticGameFacade = 0, currentGame = 0, currentMatch = 0;
        if (!ReadZ(Offsets::Il2Cpp + Offsets::InitBase, baseGameFacade) || !baseGameFacade) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); return; }
        if (!ReadZ(baseGameFacade, gameFacade) || !gameFacade) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); return; }
        if (!ReadZ(gameFacade + Offsets::StaticClass, staticGameFacade) || !staticGameFacade) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); return; }
        if (!ReadZ(staticGameFacade, currentGame) || !currentGame) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); return; }
        if (!ReadZ(currentGame + Offsets::CurrentMatch, currentMatch) || !currentMatch) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); return; }
        uint32_t entityDictionary = 0;
        if (!ReadZ(currentGame + Offsets::DictionaryEntities, entityDictionary) || !entityDictionary) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); return; }
        uint32_t entries = 0;
        if (!ReadZ(entityDictionary + 0xC, entries) || !entries) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); return; }
        entries += 0x1C;
        uint32_t entitiesCount = 0;
        if (!ReadZ(entityDictionary + 0x10, entitiesCount) || entitiesCount == 0 || entitiesCount > 150) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); return; }

        Vector2 screenCenter(g_windowWidth * 0.5f, g_windowHeight * 0.5f);
        float bestCrosshairDist = FLT_MAX;
        uintptr_t bestEntityAddr = 0;
        Vector3 bestBonePos{};

        // Need viewMatrix and mainPos - read from globals set by main thread
        // Read viewMatrix and camPos from current match - similar to main loop
        uint32_t lp = 0; ReadZ(currentMatch + Offsets::LocalPlayer, lp);
        if (!lp) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); return; }
        uint32_t mainTransform = 0; if (!ReadZ(lp + Offsets::MainCameraTransform, mainTransform) || !mainTransform) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); return; }
        Vector3 camPos; if (!GetPosition(mainTransform, camPos)) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); return; }
        uint32_t followCamera = 0; if (!ReadZ(lp + Offsets::FollowCamera, followCamera) || !followCamera) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); return; }
        uint32_t camera = 0; if (!ReadZ(followCamera + Offsets::Camera, camera) || !camera) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); return; }
        uint32_t cameraBase = 0; if (!ReadZ(camera + 0x8, cameraBase) || !cameraBase) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); return; }
        Matrix4x4 vm; if (!ReadZ(cameraBase + Offsets::ViewMatrix, vm)) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); return; }

        for (uint32_t i = 0; i < entitiesCount; ++i) {
            uint32_t entity = 0;
            if (!ReadZ(entries + i * 0x10, entity) || !entity || entity == lp) continue;
            uint32_t avatarManager = 0; if (!ReadZ(entity + Offsets::AvatarManager, avatarManager) || !avatarManager) continue;
            uint32_t avatar = 0; if (!ReadZ(avatarManager + Offsets::Avatar, avatar) || !avatar) continue;
            bool isVisible = false; if (!ReadZ(avatar + Offsets::Avatar_IsVisible, isVisible) || !isVisible) continue;
            uint32_t avatarData = 0; if (!ReadZ(avatar + Offsets::Avatar_Data, avatarData) || !avatarData) continue;
            bool isTeam = false; ReadZ(avatarData + Offsets::Avatar_Data_IsTeam, isTeam); if (isTeam) continue;
            bool isDead = false; ReadZ(entity + Offsets::Player_IsDead, isDead); if (isDead) continue;
            // use Head bone for distance
            uint32_t headBone = 0; if (!ReadZ(entity + Offsets::Bones::Head, headBone) || !headBone) continue;
            Vector3 headPos; if (!GetNodePosition(headBone, headPos)) continue;
            float worldDist = Vector3::Distance(camPos, headPos);
            if (worldDist > (float)fov * 2 && worldDist > 150) continue; // use fov as proxy for distance
            ImVec2 screenPos = WorldToScreenImVec2(vm, headPos, g_windowWidth, g_windowHeight);
            if (screenPos.x <= 0 || screenPos.y <= 0 || screenPos.x > g_windowWidth || screenPos.y > g_windowHeight) continue;
            float crossDist = std::hypot(screenPos.x - screenCenter.X, screenPos.y - screenCenter.Y);
            if (crossDist > fov) continue;
            if (crossDist < bestCrosshairDist) { bestCrosshairDist = crossDist; bestEntityAddr = entity; bestBonePos = headPos; }
        }

        if (!bestEntityAddr) {
            if (currentTarget) { RestoreEntity(currentTarget); currentTarget = 0; }
            return;
        }
        if (currentTarget && currentTarget != bestEntityAddr) { RestoreEntity(currentTarget); currentTarget = 0; }
        if (entityDatabase.find(bestEntityAddr) == entityDatabase.end()) {
            uintptr_t hipAddr = bestEntityAddr + Offsets::Bones::Hip;
            uint32_t originalHip = 0; ReadZ(hipAddr, originalHip);
            entityDatabase[bestEntityAddr] = {hipAddr, originalHip, bestBonePos, std::chrono::high_resolution_clock::now()};
            bool isDead = false; ReadZ(bestEntityAddr + Offsets::Player_IsDead, isDead);
            entityDeadStatus[bestEntityAddr] = isDead;
            entityModifiedStatus[bestEntityAddr] = false;
        }
        // bone offset - use Head
        uintptr_t boneOffset = Offsets::Bones::Head;
        // Apply
        auto itDead = entityDeadStatus.find(bestEntityAddr);
        if (itDead != entityDeadStatus.end() && !itDead->second) {
            uint32_t boneCollider = 0; ReadZ(bestEntityAddr + boneOffset, boneCollider);
            if (boneCollider) {
                auto& cache = entityDatabase[bestEntityAddr];
                WriteZ<uint32_t>(cache.hipAddr, boneCollider);
                entityModifiedStatus[bestEntityAddr] = true;
                lastWriteTime = std::chrono::steady_clock::now();
                currentTarget = bestEntityAddr;
            }
        }
        std::uniform_int_distribution<int> dist(3,8);
        std::this_thread::sleep_for(std::chrono::milliseconds(dist(gen)));
    }

    void StartAimbot() {
        if (aimbotRunning2.load()) return;
        aimbotRunning2 = true;
        aimbotThread2 = std::thread([](){ while (aimbotRunning2.load()) { ExternalAimbotTick(); } });
        aimbotThread2.detach();
    }
    void StopAimbot() {
        aimbotRunning2 = false;
        // detach already, just flag
        // cleanup
        for (auto& [addr, cache] : entityDatabase) {
            if (entityModifiedStatus[addr]) WriteZ<uint32_t>(cache.hipAddr, cache.originalHip);
        }
        entityDatabase.clear();
        entityDeadStatus.clear();
        entityModifiedStatus.clear();
        currentTarget = 0;
    }
    void AimbotExternalRunToggle(bool enable) {
        if (g_aimbotExternalBusy.exchange(true)) return;
        if (!enable && !g_aimbotExternalPatched.load()) { g_aimbotExternalBusy = false; return; }
        if (enable && g_aimbotExternalPatched.load()) { g_aimbotExternalBusy = false; return; }
        std::thread([enable](){
            if (enable) {
                StartAimbot();
                g_aimbotExternalPatched = true;
            } else {
                StopAimbot();
                g_aimbotExternalPatched = false;
            }
            g_aimbotExternalBusy = false;
        }).detach();
    }
}
