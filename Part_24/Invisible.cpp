#include "Invisible.hpp"

#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <src/Globals.hpp>

// ===========================================================================
//  INVISIBLE / ANTI INVISIBLE   (port du code APK -> ecriture memoire)
//
//  Reproduction de ApplyInvisible() / hook_IsVisible() / hook_CanTakeDamage()
//  avec ce que le DLL peut faire : lire ecrire la memoire du guest.
//
//  NOTE IMPORTANTE : le cote "serveur" du code APK
//      - PlayerNetwork.SendChangeToGBFootBall() / SendCancelGBFootBall()
//      - AvatarManager.SetVisible() / SetUMARendererActive()
//      - hook_CanTakeDamage()  (godmode)
//      - hook_IsInGBFootBallTransform()  (camera football)
//  sont des APPELS / HOOKS de code ARM dans le guest : ils ne sont pas
//  reproductibles depuis un DLL Windows qui n'accede qu'a la memoire.
// ===========================================================================

// Notre avatar a-t-on desactive IsVisible ? -> permet de restaurer a l'OFF
static bool s_InvisibleApplied = false;

// Player -> AvatarManager(0x504) -> Avatar(0xA8)
static uint32_t GetAvatar(uint32_t player)
{
    if (!player) return 0;
    uint32_t avatarManager = Mem.Read<uint32_t>(player + Offsets::AvatarManager);
    if (!avatarManager) return 0;
    return Mem.Read<uint32_t>(avatarManager + Offsets::Avatar);
}

// UmaAvatarSimple.IsVisible (0x95)
static void SetAvatarVisible(uint32_t player, bool visible)
{
    uint32_t avatar = GetAvatar(player);
    if (!avatar) return;
    Mem.Write<bool>(avatar + Offsets::Avatar_IsVisible, visible);
}

void Inv::InvisibleRun(uint32_t localPlayer, uint32_t entities, uint32_t count)
{
    // ======================================================================
    // 7004  INVISIBLE -> masque NOTRE avatar (ApplyInvisible du code source)
    // ======================================================================
    if (g_Globals.Exploits.Invisible) {
        if (localPlayer) {
            SetAvatarVisible(localPlayer, false);
            s_InvisibleApplied = true;
        }
    }
    else if (s_InvisibleApplied) {
        // case decochee -> restaure l'etat d'origine
        if (localPlayer) SetAvatarVisible(localPlayer, true);
        s_InvisibleApplied = false;
    }

    // ======================================================================
    // 7008  ANTI INVISIBLE -> force TOUS les autres joueurs a etre visibles
    //        (equivalent de hook_IsInGBFootBallTransform -> return false)
    // ======================================================================
    if (!g_Globals.Exploits.AntiInvisible) return;
    if (!entities || count == 0) return;

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t entity = Mem.Read<uint32_t>(entities + i * 0x10);
        if (!entity || entity == localPlayer) continue;

        // les morts restent masques
        if (Mem.Read<bool>(entity + Offsets::Player_IsDead)) continue;

        uint32_t avatar = GetAvatar(entity);
        if (!avatar) continue;

        // n'ecrit que si le jeu a effectivement cache ce joueur
        if (!Mem.Read<bool>(avatar + Offsets::Avatar_IsVisible))
            Mem.Write<bool>(avatar + Offsets::Avatar_IsVisible, true);
    }
}
