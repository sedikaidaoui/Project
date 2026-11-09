#include "SpeedHack.hpp"

#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <src/Globals.hpp>
#include <cmath>

namespace SpeedHack {

void Run() {
    if (Offsets::Il2Cpp == 0)
        return;

    uint32_t baseGameFacade = 0, gameFacade = 0, staticGameFacade = 0, currentGame = 0, speedHackPtr = 0;

    if (!Mem.Read<uint32_t>(Offsets::Il2Cpp + Offsets::InitBase, baseGameFacade) || baseGameFacade == 0) return;
    if (!Mem.Read<uint32_t>(baseGameFacade, gameFacade) || gameFacade == 0) return;
    if (!Mem.Read<uint32_t>(gameFacade + Offsets::StaticClass, staticGameFacade) || staticGameFacade == 0) return;
    if (!Mem.Read<uint32_t>(staticGameFacade, currentGame) || currentGame == 0) return;
    if (!Mem.Read<uint32_t>(currentGame + Offsets::GameTimer, speedHackPtr) || speedHackPtr == 0) return;

    const float kBaseFixedDelta = 0.033f;

    if (g_Globals.Exploits.SpeedHack) {
        const float targetSpeed = kBaseFixedDelta * (g_Globals.Exploits.SpeedHackPercent / 100.0f);
        Mem.Write<float>(speedHackPtr + Offsets::FixedDeltaTime, targetSpeed);
    }
    else {
        float speedVal = 0.0f;
        if (Mem.Read<float>(speedHackPtr + Offsets::FixedDeltaTime, speedVal) &&
            std::fabs(speedVal - kBaseFixedDelta) > 0.001f) {
            Mem.Write<float>(speedHackPtr + Offsets::FixedDeltaTime, kBaseFixedDelta);
        }
    }
}

} // namespace SpeedHack