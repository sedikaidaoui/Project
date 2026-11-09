#pragma once
#include <cstdint>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Player.h>

namespace Aim {
    class SilentMem {
    public:
        static void Work();
        static void Start();
        static void Stop();
    private:
        static bool GetTarget(Player& outTarget);
        static bool GetTargetByClosestToCrosshair(Player& outTarget);
        static bool GetTarget360(Player& outTarget);
        static bool GetTargetByLowestHealth(Player& outTarget);
        static bool GetTargetByClosestToPlayer(Player& outTarget);
        static void PushFrame(const Player& target);
    };
}