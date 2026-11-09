// ============================================================================
//  CmdServer — port C++ du CheatCommandServer.cs (protocole identique)
//
//    Socket: Aim/Fire/Speed/Cover/Night/Rotation ON/OFF
//    Socket: All ON/OFF
//    FOV=%.1f
//    PMGLEAFAIGF log ON/OFF
//
//  BOUTONS WIRES (vers g_Globals, meme etat que le menu) :
//    AIM_ON / AIM_OFF      -> AimBot.Enabled        + lib Mode 1044 (AimkillSend)
//    FIRE_ON / FIRE_OFF    -> AimBot.AutoFire       + lib Mode 521  (AutofireAllGuns)
//    SPEED_ON / SPEED_OFF  -> Exploits.SpeedHack    + lib Mode 109  (speedHack)
//    ROTATION_ON / _OFF    -> Exploits.SpringHack   (pas d'equivalent lib)
//    COVER_ON / COVER_OFF  -> Exploits.WallHack     + lib Mode 1048 (CoverElimination)
//    ALL ON / ALL OFF      -> les 5 ci-dessus en bloc (+ relais lib)
//    FOV=<nombre>          -> AimBot.Fov + AimKill.AimFov + lib Mode 104
//                             (unites identiques : rayon pixel des deux cotes)
//
//  OPTIONS AIMKILL (couverture totale du tab, flags miroirs + relais lib) :
//    ACTIVATE_ALL_ON/OFF   -> Mode 3   (+ macro ESP natives au ON)
//    AIMKILL_SEND_ON/OFF   -> Mode 1044        SILENT_KILL_*     -> 889
//    SEND_ALL_FOV_*        -> Mode 1050        SAFE_AIM_*        -> 5661
//    REAL_AIM_V2_*         -> Mode 5662        AUTO_SWITCH_SEND* -> 1047
//    AUTO_SWITCH_NORMAL_*  -> Mode 508         COVER_ELIM_*      -> 1048
//    AIMKILL_NO_SWITCH_*   -> Mode 1049        HIDE_DAMAGE_*     -> 527
//    COVER_PULL_*          -> Mode 509         AIM_LOCK_*        -> 34834
//    TARGET_LOCK_*         -> Mode 523         INVINS_*          -> 532
//    AUTOFIRE_GUNS_*       -> Mode 521         DOWN_KILL_*       -> 504
//    DIVE_KILL_*           -> Mode 5660        BRUST_FIRE_*      -> 515
//    RAPID_FIRE_*          -> Mode 518         FAST_RELOAD_*     -> 519
//    FLY_X40_*             -> Mode 5659        STABLE_FLY_*      -> 5002
//    TELEPORT_MARK_*       -> Mode 56578       SEND_POSITION_*   -> 5663
//    FLY_HACK_*            -> Mode 5657        HIGH_FPS_*        -> 529
//
//  RELAIS CONNECT LIB : chaque commande est appliquee localement PUIS
//  relaiee au lib via AimKillLink::SendToggle/SendFov.  L'envoi est un
//  no-op si le lien n'est pas etabli (l'etat complet est de toute facon
//  rejoue par SyncAll() a chaque reconnexion du canal UDS).
//
//  JOURNAL UNIQUEMENT (pas de toggle correspondant dans le menu) :
//    NIGHT_*, PMGLEAFAIGF_*, tout token inconnu
//
//  LIAISON : 127.0.0.1 uniquement (IPAddress.Loopback dans le prototype C#).
//  Une seule connexion traitee a la fois — c'est un canal de controle local,
//  pas un serveur multi-clients.
// ============================================================================

// Winsock2 DOIT etre include avant <Windows.h> (voir adb_utils.cpp)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include <Windows.h>

#include <src/Globals.hpp>
#include <src/aimkill/AimKillLink.hpp>
#include "CmdServer.hpp"

#include <atomic>
#include <thread>
#include <string>
#include <cstdarg>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// console de debug allouee par initcmd() dans main.cpp
extern bool g_ConsoleAllocated;

namespace CmdServer {

static constexpr unsigned short kPort  = 5555;
static constexpr int            kTimeoutMs = 200;  // select : arret reactif sans fermer depuis l'exterieur

static std::atomic<bool> g_running{false};
static std::atomic<bool> g_stop{false};
static std::thread      g_thread;

// ---------------------------------------------------------------- journal ---
static void Log(const char* fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf_s(buf, _TRUNCATE, fmt, ap);
    va_end(ap);

    OutputDebugStringA(buf);
    OutputDebugStringA("\n");
    if (g_ConsoleAllocated)
        printf("%s\n", buf);
}

// ------------------------------------------------------------- dispatchers ---
static void SetToggle(bool& flag, bool on, const char* name)
{
    if (flag == on) {
        Log("[cmd] %s deja %s", name, on ? "ON" : "OFF");
        return;
    }
    flag = on;
    Log("[cmd] %s -> %s", name, on ? "ON" : "OFF");
}

// ------------------------------------------------- relais vers la lib ------
// Pose le champ miroir du tab AIMKILL (etat panel identique) puis relaie la
// commande au lib injecte via le canal CONNECT LIB.  SendToggle est un no-op
// si le lien n'est pas etabli ; SyncAll() rejoue tout a la reconnexion.
static void SetAkToggle(bool Globals::AimKill::* field, int mode, bool on, const char* name)
{
    bool& flag = g_Globals.AimKill.*field;
    if (flag == on) {
        Log("[cmd] %s deja %s (lib)", name, on ? "ON" : "OFF");
        return;
    }
    flag = on;
    AimKillLink::SendToggle(mode, on);
    Log("[cmd] %s -> %s (lib mode %d)", name, on ? "ON" : "OFF", mode);
}

// Table des 27 options du tab AIMKILL : token de commande -> champ miroir + mode wire.
struct AkCmd { const char* token; bool Globals::AimKill::* field; int mode; };
static const AkCmd kAkCmds[] = {
    { "ACTIVATE_ALL",      &Globals::AimKill::ActivateAll,      3     },
    { "AIMKILL_SEND",      &Globals::AimKill::AimkillSend,      1044  },
    { "SILENT_KILL",       &Globals::AimKill::SilentKill,       889   },
    { "SEND_ALL_FOV",      &Globals::AimKill::SendAllFov,       1050  },
    { "SAFE_AIM",          &Globals::AimKill::SafeAimkill,      5661  },
    { "REAL_AIM_V2",       &Globals::AimKill::RealAimkillV2,    5662  },
    { "AUTO_SWITCH_SEND",  &Globals::AimKill::AutoSwitchSend,   1047  },
    { "AUTO_SWITCH_NORMAL",&Globals::AimKill::AutoSwitchNormal, 508   },
    { "COVER_ELIM",        &Globals::AimKill::CoverElimination, 1048  },
    { "AIMKILL_NO_SWITCH", &Globals::AimKill::AimkillNoSwitch,  1049  },
    { "HIDE_DAMAGE",       &Globals::AimKill::HideDamage,       527   },
    { "COVER_PULL",        &Globals::AimKill::CoverPull,        509   },
    { "AIM_LOCK",          &Globals::AimKill::AimLock,          34834 },
    { "TARGET_LOCK",       &Globals::AimKill::TargetLock,       523   },
    { "INVINS",            &Globals::AimKill::InvinsHack,       532   },
    { "AUTOFIRE_GUNS",     &Globals::AimKill::AutofireAllGuns,  521   },
    { "DOWN_KILL",         &Globals::AimKill::DownKill,         504   },
    { "DIVE_KILL",         &Globals::AimKill::DiveKillNew,      5660  },
    { "BRUST_FIRE",        &Globals::AimKill::BrustFire,        515   },
    { "RAPID_FIRE",        &Globals::AimKill::RapidFire,        518   },
    { "FAST_RELOAD",       &Globals::AimKill::FastReload,       519   },
    { "FLY_X40",           &Globals::AimKill::FlyX40,           5659  },
    { "STABLE_FLY",        &Globals::AimKill::StableFly,        5002  },
    { "TELEPORT_MARK",     &Globals::AimKill::TeleportMark,     56578 },
    { "SEND_POSITION",     &Globals::AimKill::SendPosition,     5663  },
    { "FLY_HACK",          &Globals::AimKill::FlyhackMini,      5657  },
    { "HIGH_FPS",          &Globals::AimKill::HighFps,          529   },
};

static void ApplyAll(bool on)
{
    SetToggle(g_Globals.AimBot.Enabled,   on, "All/Aimbot");
    SetToggle(g_Globals.AimBot.AutoFire,  on, "All/Autofire");
    SetToggle(g_Globals.Exploits.SpeedHack, on, "All/Speed");
    SetToggle(g_Globals.Exploits.SpringHack, on, "All/Rotation");
    SetToggle(g_Globals.Exploits.WallHack,   on, "All/Cover");
    // relais vers la lib (les equivalents payload)
    SetAkToggle(&Globals::AimKill::AimkillSend,      1044, on, "All/AimkillSend");
    SetAkToggle(&Globals::AimKill::AutofireAllGuns,  521,  on, "All/AutofireGuns");
    SetAkToggle(&Globals::AimKill::CoverElimination, 1048, on, "All/CoverElim");
    AimKillLink::SendToggle(109, on);   // SpeedHack du lib (pas de champ AimKill dedie)
    Log("[cmd] All %s -> lib", on ? "ON" : "OFF");
}

static std::string Trim(const std::string& s)
{
    const char* ws = " \t\r\n";
    const size_t b = s.find_first_not_of(ws);
    if (b == std::string::npos) return std::string();
    const size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

static std::string ToUpper(const std::string& s)
{
    std::string out = s;
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = (char)toupper((unsigned char)out[i]);
    return out;
}

// Meme logique que CheatCommandServer.SplitKindOn (C#)
static void SplitKindOn(const std::string& upper, std::string& kind, bool& on)
{
    static const std::string kOn  = "_ON";
    static const std::string kOff = "_OFF";

    if (upper.size() > kOn.size() && upper.compare(upper.size() - kOn.size(), kOn.size(), kOn) == 0) {
        kind = upper.substr(0, upper.size() - kOn.size());
        on   = true;
        return;
    }
    if (upper.size() > kOff.size() && upper.compare(upper.size() - kOff.size(), kOff.size(), kOff) == 0) {
        kind = upper.substr(0, upper.size() - kOff.size());
        on   = false;
        return;
    }
    kind = upper;   // pas de suffixe -> considere comme ON (comme le C#)
    on   = true;
}

static void ApplyCommand(const std::string& rawLine)
{
    const std::string token = Trim(rawLine);
    if (token.empty()) return;

    const std::string upper = ToUpper(token);

    // ---- FOV=%.1f -----------------------------------------------------------
    if (upper.compare(0, 4, "FOV=") == 0) {
        const char* p = token.c_str() + 4;
        char* end = nullptr;
        const double v = strtod(p, &end);
        if (end != p && end && *end == '\0') {
            double clamped = v;
            if (clamped < 1.0)   clamped = 1.0;
            if (clamped > 180.0) clamped = 180.0;
            g_Globals.AimBot.Fov = (int)(clamped + 0.5);
            g_Globals.AimKill.AimFov = g_Globals.AimBot.Fov;  // miroir slider AIMKILL
            AimKillLink::SendFov(g_Globals.AimBot.Fov);        // relais lib Mode 104
            Log("[cmd] FOV=%.1f -> AimBot.Fov=%d + lib mode 104", clamped, g_Globals.AimBot.Fov);
            return;
        }
        Log("[cmd] FOV invalide '%s'", token.c_str());
        return;
    }

    // ---- ALL ON / ALL OFF ---------------------------------------------------
    if (upper == "ALL ON")  { ApplyAll(true);  return; }
    if (upper == "ALL OFF") { ApplyAll(false); return; }

    // ---- *_ON / *_OFF -------------------------------------------------------
    std::string kind;
    bool on = true;
    SplitKindOn(upper, kind, on);

    if      (kind == "AIM") {
        SetToggle(g_Globals.AimBot.Enabled, on, "Aim");
        SetAkToggle(&Globals::AimKill::AimkillSend, 1044, on, "Aim(lib)");
    }
    else if (kind == "FIRE") {
        SetToggle(g_Globals.AimBot.AutoFire, on, "Fire");
        SetAkToggle(&Globals::AimKill::AutofireAllGuns, 521, on, "Fire(lib)");
    }
    else if (kind == "SPEED") {
        SetToggle(g_Globals.Exploits.SpeedHack, on, "Speed");
        AimKillLink::SendToggle(109, on);   // speedHack du lib
        Log("[cmd] Speed -> lib mode 109 %s", on ? "ON" : "OFF");
    }
    else if (kind == "ROTATION")    SetToggle(g_Globals.Exploits.SpringHack, on, "Rotation");  // pas d'equivalent lib
    else if (kind == "COVER") {
        SetToggle(g_Globals.Exploits.WallHack, on, "Cover");
        SetAkToggle(&Globals::AimKill::CoverElimination, 1048, on, "Cover(lib)");
    }
    else if (kind == "NIGHT")       Log("[cmd] Night %s (aucun toggle — journal uniquement)", on ? "ON" : "OFF");
    else if (kind == "PMGLEAFAIGF") Log("[cmd] PMGLEAFAIGF log %s", on ? "ON" : "OFF");
    else {
        // ---- options AIMKILL (couverture totale) ----------------------------
        bool found = false;
        for (const AkCmd& c : kAkCmds) {
            if (kind != c.token) continue;
            found = true;
            SetAkToggle(c.field, c.mode, on, c.token);
            if (on && strcmp(c.token, "ACTIVATE_ALL") == 0) {
                // meme macro que la case "Activate All" du menu (id 102)
                g_Globals.Visuals.Lines     = true;
                g_Globals.Visuals.Box       = true;
                g_Globals.Visuals.HealthBar = true;
                g_Globals.Visuals.Name      = true;
                g_Globals.Visuals.Distance  = true;
                g_Globals.Misc.ShowAimbotFov = true;
            }
            break;
        }
        if (!found) Log("[cmd] unknown '%s'", token.c_str());
    }
}

// ---------------------------------------------------------------- client ----
static void HandleClient(SOCKET client)
{
    char buf[1024];
    std::string lineBuf;

    for (;;) {
        if (g_stop) break;

        fd_set rd;
        FD_ZERO(&rd);
        FD_SET(client, &rd);
        timeval tv;
        tv.tv_sec  = 0;
        tv.tv_usec = kTimeoutMs * 1000;

        const int r = select(0, &rd, nullptr, nullptr, &tv);
        if (r < 0) break;       // select en erreur -> client ferme
        if (r == 0) continue;   // timeout -> on reverifie g_stop

        const int n = recv(client, buf, (int)sizeof(buf), 0);
        if (n <= 0) break;      // 0 = fermeture propre

        for (int i = 0; i < n; ++i) {
            const char c = buf[i];
            if (c == '\n' || c == '\r') {
                if (!lineBuf.empty()) {
                    ApplyCommand(lineBuf);
                    lineBuf.clear();
                }
            }
            else if (lineBuf.size() < 4096) {
                lineBuf += c;
            }
        }
    }

    closesocket(client);
    Log("[cmd] disconnected");
}

// ---------------------------------------------------------------- thread ----
static void ThreadMain()
{
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        Log("[cmd] WSAStartup failed (%d)", WSAGetLastError());
        g_running = false;
        return;
    }

    const SOCKET ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ls == INVALID_SOCKET) {
        Log("[cmd] socket() failed (%d)", WSAGetLastError());
        WSACleanup();
        g_running = false;
        return;
    }

    int reuse = 1;
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(kPort);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");   // LOOPBACK SEULEMENT

    if (addr.sin_addr.s_addr == INADDR_NONE ||
        bind(ls, (const sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR ||
        listen(ls, 4) == SOCKET_ERROR)
    {
        Log("[cmd] bind/listen failed on 127.0.0.1:%u (%d)", (unsigned)kPort, WSAGetLastError());
        closesocket(ls);
        WSACleanup();
        g_running = false;
        return;
    }

    Log("[cmd] Socket: ready on port %u", (unsigned)kPort);

    while (!g_stop)
    {
        fd_set rd;
        FD_ZERO(&rd);
        FD_SET(ls, &rd);
        timeval tv;
        tv.tv_sec  = 0;
        tv.tv_usec = kTimeoutMs * 1000;

        const int r = select(0, &rd, nullptr, nullptr, &tv);
        if (r < 0) break;
        if (r == 0) continue;

        sockaddr_in cli{};
        int clen = sizeof(cli);
        const SOCKET c = accept(ls, (sockaddr*)&cli, &clen);
        if (c == INVALID_SOCKET) continue;

        Log("[cmd] connected from %u.%u.%u.%u:%u",
            (unsigned)((unsigned char*)&cli.sin_addr.s_addr)[0],
            (unsigned)((unsigned char*)&cli.sin_addr.s_addr)[1],
            (unsigned)((unsigned char*)&cli.sin_addr.s_addr)[2],
            (unsigned)((unsigned char*)&cli.sin_addr.s_addr)[3],
            (unsigned)ntohs(cli.sin_port));

        HandleClient(c);   // un client a la fois, canal de controle local
    }

    closesocket(ls);
    WSACleanup();
    g_running = false;
    Log("[cmd] server stopped");
}

// ------------------------------------------------------------------ API -----
void Start()
{
    if (g_running || g_thread.joinable())
        return;

    g_stop = false;
    g_running = true;
    g_thread = std::thread(ThreadMain);
}

void Stop()
{
    g_stop = true;                       // le thread sort de select() sous 200 ms
    if (g_thread.joinable())
        g_thread.join();
    g_running = false;
}

bool IsRunning()        { return g_running; }
unsigned short Port()   { return kPort;     }

} // namespace CmdServer
