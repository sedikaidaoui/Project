#pragma once

#include <string>

// ---------------------------------------------------------------------------
// FakeLagFreeze  ->  "Freeze Enemy"  +  "Ghost Lag"  (ancien "Telekill Lag")
//
// Deux retentions de paquets via WinDivert (port du script Python) :
//
//  * FREEZE  (g_Globals.Exploits.FakeLag_Freeze)
//      Filtre Python FILTER_I : les paquets UDP ENTRANTS en provenance des
//      ports serveur 10000-10099 sont retenus en memoire. A la desactivation
//      tous les paquets sont reinjectes d'un coup puis le handle est ferme.
//
//  * TELEKILL LAG  (g_Globals.Exploits.FakeLag_Tele)
//      Filtre Python FILTER_O etendu : les paquets UDP SORTANTS vers les ports
//      serveur 10000-10099 sont retenus (le serveur ne voit plus rien de ce
//      que fait le joueur). Contrairement au script Python, il n'y a AUCUNE
//      limite de taille de paquet ("ip.Length > 54" laissait passer les
//      petits paquets qui faisaient revenir l'effet) et la retenue est plus
//      importante (16384 paquets / 8 Mo au lieu de 8192 / 8 Mo).
//
// Requis : WinDivert.dll + WinDivert64.sys (x86 : WinDivert32.sys) et droits
// administrateur. L'API est chargee dynamiquement (aucune dependance d'import).
// ---------------------------------------------------------------------------
namespace FakeLagFreeze {

    // Demarre les deux threads de capture (idempotent) — appele par MethodsAll::Initialize()
    void Initialize();

    // Arret propre : reinjection des paquets retenus, fermeture des handles, join
    void Shutdown();

    // Etat "Freeze Enemy" affiche dans le menu ("" = rien a signaler)
    std::string StatusText();

    // Etat "Ghost Lag" (ancien Telekill) affiche dans le menu ("" = rien a signaler)
    std::string TelekillStatusText();

} // namespace FakeLagFreeze
