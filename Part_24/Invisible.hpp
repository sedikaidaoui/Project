#pragma once

#include <cstdint>

namespace Inv {

// INVISIBLE (7004) / ANTI INVISIBLE (7008)
//
// Port du code APK (backend.cpp) vers l'ecriture directe en memoire.
// Notre DLL tourne cote HOST (BlueStacks / VirtualBox) : on ne peut NI appeler
// NI hooker les methodes il2cpp du jeu guest (DobbyHook, SendChangeToGBFootBall,
// AvatarManager.SetVisible, MatchGame.CanTakeDamage ...).
// On reproduit donc tout ce qui est accessible par ecriture du champ
// UmaAvatarSimple.IsVisible  ->  Player + AvatarManager + Avatar + IsVisible.
void InvisibleRun(uint32_t localPlayer, uint32_t entities, uint32_t count);

} // namespace Inv
