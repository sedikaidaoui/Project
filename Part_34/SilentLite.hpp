#pragma once

#include <cstdint>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Math/Matrix4v4.hpp>

namespace Lite {

// Silent - Lite : code source recopie tel quel (voir SilentLite.cpp).
void SilentLiteRun(uint32_t localPlayer,
                   uint32_t entities,
                   uint32_t entitiesCount,
                   Vector3 mainPos,
                   Matrix4x4 viewMatrix,
                   int g_windowWidth,
                   int g_windowHeight);

} // namespace Lite
