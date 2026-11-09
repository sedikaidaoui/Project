#pragma once

#include <cstdint>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Math/Matrix4v4.hpp>

namespace Lite {

// ---------------------------------------------------------------------------
// MAGNET - LITE
// Tant que le joueur tire (saim1), l'ennemi le plus proche du viseur est
// deplace sur le rayon de tir (ecriture de la translation racine +0x60),
// avec lissage camira / direction / projection.
// Des que le tir s'arrete, TOUTES les positions d'origine sont restaurees.
// ---------------------------------------------------------------------------
void MagnetLiteRun(uint32_t localPlayer,
                   uint32_t entities,
                   uint32_t entitiesCount,
                   const Vector3& mainPos,
                   const Matrix4x4& viewMatrix,
                   int screenWidth,
                   int screenHeight);

} // namespace Lite
