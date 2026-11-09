#pragma once
#include <cstdint>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Player.h>
#include <src/Globals.hpp>
#include <EspLines/Memory/Memory.hpp>

struct ImVec2;

struct GameContext {
    uint32_t currentGame = 0;
    uint32_t currentMatch = 0;
    uint32_t localPlayer = 0;
    uint32_t activePlayer = 0;
};

namespace FWork {
    class Data {
    public:
        static void Work();
        static void Refresh();
        static void RestoreFakeDamage();   // BuffDamage -> 1.0f (unload / fin de match)
        static bool IsFakeDamageActive();  // valeur boostee en memoire
    private:
        static bool EntityData(uint32_t entity, Player& player, Vector3& mainPos);
        static uint32_t GetCurrentGame();
        static uint32_t GetCurrentMatch(uint32_t currentGame);
        static bool SetupLocalPlayerAndCamera(GameContext& ctx);
        static void ProcessEntities(const GameContext& ctx);
        //static bool IsInsideFOV(const ImVec2& pos);
    };
    bool IsInsideFOV(const ImVec2& pos);
}
