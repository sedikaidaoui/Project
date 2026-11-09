#pragma once

// ============================================================================
//  CmdServer — serveur de commandes TCP (boucle locale, port 5555)
//
//  Implementation dans CmdServer.cpp (winsock2 DOIT rester dans le .cpp :
//  il doit etre include AVANT <Windows.h>, sinon conflit winsock.h/winsock2.h).
//
//  Demarre automatiquement a l'injection (Cheat::Initialize) et s'arrete
//  proprement dans Unload().
// ============================================================================

namespace CmdServer
{
    void Start();        // lance le thread serveur (idempotent)
    void Stop();         // arret propre + join (idempotent)
    bool IsRunning();    // true tant que le thread serveur est vivant
    unsigned short Port();
}
