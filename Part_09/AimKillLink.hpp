#pragma once

// ============================================================================
//  AimKillLink — pont vers la lib du projet APK UPDATEDOB55 (BLACK CORPS)
//
//  Implementation dans AimKillLink.cpp :
//    - extract des .so embarques (RCDATA) -> %TEMP%\0xvoid_ak\
//    - decouverte ADB (BlueStacks) + injection Identique a Menu.java InjectX32
//    - canal binaire UDS : adb forward tcp:15556 localabstract:FHKFKLFLF
//    - requetes 32 octets (Mode/boolean/value) + longueur BE, reponse drainee
//
//  La connexion est maintenue ouverte PENDANT TOUTE LA VIE du menu :
//  le serveur UDS du lib meurt des que le client se deconnecte (Close()
//  du serveur referme egalement le listenfd, ce qui laisserait son thread
//  en spin a 100% CPU dans Free Fire).  Shutdown() est appele une seule
//  fois, dans Unload() : il joint le thread pipeline mais NE referme PAS
//  le socket.
// ============================================================================

#include <string>

namespace AimKillLink
{
    enum class LinkState { Idle, Busy, Connected, Error };

    // Lance le pipeline complet en thread membre (idempotent, non bloquant).
    void      StartConnect();
    bool      IsBusy();
    LinkState State();
    std::string Status();      // copie sous mutex, prete pour l'affichage
    bool      IsReady();       // Connected + socket vivant

    // Envoi immediat (no-op si pas pret) — l'etat est de toute facon
    // rejoue par SyncAll() a chaque reconnexion.
    void SendToggle(int mode, bool on);
    void SendFov(int value);   // Mode 104, valeur entiere brute (backend int->float)

    void SyncAll();            // rejoue toutes les options UI vers le lib

    void Shutdown();           // Unload UNIQUEMENT : joint le thread, socket laisse ouvert
}
