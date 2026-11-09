#pragma once

#include <atomic>
#include <thread>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Player.h>

namespace Aim {

class ExtremeAimbot {
public:
    static void Start();
    static void Stop();

private:
    static void Work();
    static Vector3 GetHitbox(const Player& entity);
    static bool FindBestTarget(Player& outTarget);
};

} // namespace Aim
