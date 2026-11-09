// ============================================================================
//  AimKillLink.cpp — pipeline "CONNECT LIB" (extraction ADB + injection + UDS)
//
//  Etapes (thread membre, idempotent) :
//    1. decouverte ADB (adb devices + pidof com.dts.freefireth, sinon ps)
//    2. adb forward tcp:15556 localabstract:FHKFKLFLF
//    3. pre-flight handshake InitMode(1)  -> si la lib repond deja :
//       on ne re-injecte pas (bouton idempotent)
//    4. extraction RCDATA -> %TEMP%\0xvoid_ak\, push x3 + chmod 777
//    5. injection : <su> -c '/data/local/tmp/libinject -pkg com.dts.freefireth
//       -lib /data/local/tmp/libTERMINALX999.so -open -dl_memfd'
//       (su BlueStacks /boot/.../bstk/su en priorite, fallbacks ensuite)
//    6. attente serveur (8x500ms) + SyncAll de toute l'etat UI
//
//  Protocole binaire (IncludeClient.h / server.cpp) :
//    envoi : u32 BE(taille) + octets bruts (struct 32 octets pour les toggles)
//    recoi : u32 BE(taille) + octets bruts (Response = 122 896 octets)
//    -> la reponse doit etre drainee EN ENTIER a chaque requete, sinon le
//       flux se desynchronise (serveur bloque sur son write suivant).
//
//  Classification du handshake (element clé du probe-first) :
//    - connect refuse OU EOF/reset avant reponse -> Hs::None  (pas de serveur
//      derriere le forward : adb referme la connexion locale)  -> INJECTION
//    - timeout de reponse (serveur la mais muet, ex. menu APK deja connecte :
//      le serveur ne fait Accept() qu'un client a la fois)     -> Hs::Fail
//    - reponse recue                                      -> Hs::Ok
//
//  Politique de fermeture (IMPORTANT) :
//    on ne ferme JAMAIS le socket pendant la vie du menu. Le serveur UDS du
//    lib tue son listenfd des que le premier client part (server.cpp Close())
//    et son thread tournerait alors en spin a 100% dans Free Fire. Meme en
//    cas de panne (AbortLink) l'etat passe en Error et une re-injection est
//    exigee ; a Unload() on JOIN le thread pipeline mais on laisse le socket
//    ouvert (0% CPU cote serveur jusqu'a la fermeture de BlueStacks).
//    Une re-injection apres coupure exige un restart de Free Fire (bionic
//    saute le constructeur au dlopen du meme SONAME) — message dedie.
// ============================================================================

// Winsock2 DOIT preceder <Windows.h> (voir CmdServer.cpp)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include <Windows.h>

#include "AimKillLink.hpp"
#include "../Globals.hpp"
#include "../../resource.h"

#include <atomic>
#include <cctype>
#include <cstring>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

extern HMODULE g_hModule;   // main.cpp

namespace AimKillLink {

// --------------------------------------------------------------- constantes --
static constexpr unsigned short kPort        = 15556;   // forward TCP local
static const char*              kAbstract    = "localabstract:FHKFKLFLF";
static const char*              kPkg         = "com.dts.freefireth";
static const char*              kLib         = "libTERMINALX999.so";
static const char*              kLibRemote   = "/data/local/tmp/libTERMINALX999.so";
static const char*              kAliasRemote = "/data/local/tmp/libmeuovo.so";
static const char*              kInjRemote   = "/data/local/tmp/libinject";

static constexpr DWORD  kSndTimeoutMs = 500;                    // SO_SNDTIMEO
static constexpr DWORD  kRcvTimeoutMs = 500;                    // SO_RCVTIMEO
static constexpr size_t kMaxResponse  = 4u * 1024u * 1024u;     // garde-fou drain

// ------------------------------------------------------------------- etat ---
namespace {
    std::mutex             g_statusMtx;         // protege g_status (copie UI)
    std::string            g_status = "Idle";
    std::atomic<LinkState> g_state{LinkState::Idle};
    std::atomic<bool>      g_busy{false};       // pipeline en cours
    std::atomic<bool>      g_shutdown{false};   // mis dans Unload()
    std::mutex             g_io;                // serialise le socket
    SOCKET                 g_sock = INVALID_SOCKET;
    std::thread            g_thread;            // thread pipeline (rejoignable)
}

static void SetStatus(LinkState s, const std::string& txt) {
    g_state = s;
    {
        std::lock_guard<std::mutex> lk(g_statusMtx);
        g_status = txt;
    }
    const std::string line = std::string("[aimkill] ") + txt + "\n";
    OutputDebugStringA(line.c_str());
}

// Winsock ref-counte : CmdServer fait aussi WSAStartup/WSACleanup, donc on
// ne nettoie jamais ici (son WSACleanup ne tombe pas a 0 tant qu'on vit).
static void EnsureWsa() {
    static std::once_flag once;
    std::call_once(once, []() {
        WSADATA wsa{};
        WSAStartup(MAKEWORD(2, 2), &wsa);
    });
}

// --------------------------------------------------------- runner / ADB ----
// Lance une commande, capture stdout+stderr, impose un deadline via
// PeekNamedPipe : ne JAMAIS bloquer le thread appelet.  Le pointeur 'stop'
// permet d'abandonner en <=15 ms (fermeture propre a l'Unload).
// Retour : code de sortie (-2 = timeout/arrete, -1 = CreateProcess KO).
static int RunProc(const std::string& cmd, std::string& out, DWORD timeoutMs,
                   const std::atomic<bool>* stop = nullptr) {
    out.clear();

    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
    HANDLE hRead = nullptr, hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return -1;
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = {};
    si.cb          = sizeof(si);
    si.hStdOutput  = hWrite;
    si.hStdError   = hWrite;
    si.dwFlags     = STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};
    std::string mutableCmd = cmd;   // CreateProcessA ecrit dedans
    if (!CreateProcessA(nullptr, &mutableCmd[0], nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hWrite);
        CloseHandle(hRead);
        return -1;
    }
    CloseHandle(hWrite);

    const DWORD start = GetTickCount();
    bool timedOut = false;
    bool aborted  = false;
    for (;;) {
        if (stop && stop->load()) { aborted = true; break; }

        DWORD avail = 0;
        if (PeekNamedPipe(hRead, nullptr, 0, nullptr, &avail, nullptr)) {
            if (avail) {
                char buf[4096];
                DWORD rd = 0;
                if (ReadFile(hRead, buf, sizeof(buf), &rd, nullptr) && rd)
                    out.append(buf, rd);
                continue;
            }
        }   // pipe casse = EOF cote enfant

        DWORD code = 0;
        if (GetExitCodeProcess(pi.hProcess, &code) && code != STILL_ACTIVE)
            break;
        if (GetTickCount() - start > timeoutMs) { timedOut = true; break; }
        Sleep(15);
    }

    if (timedOut || aborted) TerminateProcess(pi.hProcess, 9);
    WaitForSingleObject(pi.hProcess, 2000);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hRead);
    return (timedOut || aborted) ? -2 : static_cast<int>(code);
}

// Meme resolution que FWork::ADB::GetAdbPath (adb_utils.cpp), mais autonome :
// ce module n'utilise pas adb_utils (le pipeline a ses propres regles de
// timeout et de retry).
static std::string GetAdbPath() {
    char buf[MAX_PATH] = {};
    GetModuleFileNameA(NULL, buf, MAX_PATH);
    std::string exePath(buf);
    const size_t pos = exePath.find_last_of("\\/");
    const std::string exeDir = (pos != std::string::npos) ? exePath.substr(0, pos) : ".";

    if (GetFileAttributesA((exeDir + "\\HD-Adb.exe").c_str()) != INVALID_FILE_ATTRIBUTES)
        return exeDir + "\\HD-Adb.exe";
    if (GetFileAttributesA((exeDir + "\\adb.exe").c_str()) != INVALID_FILE_ATTRIBUTES)
        return exeDir + "\\adb.exe";

    const char* paths[] = {
        "C:\\Program Files\\BlueStacks_nxt\\HD-Adb.exe",
        "C:\\Program Files (x86)\\BlueStacks_nxt\\HD-Adb.exe",
        "C:\\Program Files\\BlueStacks\\HD-Adb.exe",
        "C:\\Program Files (x86)\\BlueStacks\\HD-Adb.exe",
    };
    for (const char* p : paths)
        if (GetFileAttributesA(p) != INVALID_FILE_ATTRIBUTES) return p;

    return "adb.exe";
}

static int Adb(const std::string& args, std::string& out, DWORD timeoutMs = 15000) {
    return RunProc("\"" + GetAdbPath() + "\" " + args, out, timeoutMs, &g_shutdown);
}

// ---------------------------------------------- decouverte appareil / pid ---
static bool LineIsDevice(const std::string& line, std::string& serial) {
    const size_t t = line.find('\t');
    if (t == std::string::npos) return false;
    const std::string st = line.substr(t + 1);
    if (st.rfind("device", 0) != 0) return false;   // offline / unauthorized : ignore
    serial = line.substr(0, t);
    return !serial.empty();
}

static void ParseDevices(const std::string& text, std::vector<std::string>& out) {
    std::istringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.find("List of devices") != std::string::npos) continue;
        std::string serial;
        if (LineIsDevice(line, serial)) out.push_back(serial);
    }
}

static std::string FindDevice() {
    std::string out;
    std::vector<std::string> serials;
    Adb("devices", out, 10000);
    ParseDevices(out, serials);

    if (serials.empty()) {   // fallback ports BlueStacks
        Adb("connect 127.0.0.1:5555", out, 8000);
        Adb("connect 127.0.0.1:5557", out, 8000);
        Adb("connect 127.0.0.1:5559", out, 8000);
        Adb("devices", out, 10000);
        ParseDevices(out, serials);
    }
    if (serials.empty()) return "";

    // preferer un appareil ou le jeu tourne (pidof)
    for (const auto& s : serials) {
        std::string pidOut;
        Adb("-s " + s + " shell pidof " + kPkg, pidOut, 6000);
        if (pidOut.find_first_of("0123456789") != std::string::npos) return s;
    }
    return serials[0];   // pidof peut ne pas exister : on tentera quand meme
}

static std::string ParsePid(const std::string& text) {
    std::string pid;
    for (char c : text) {
        if (c >= '0' && c <= '9') pid.push_back(c);
        else if (!pid.empty()) break;
    }
    return pid;
}

static std::string GamePid(const std::string& dev) {
    std::string out;
    Adb("-s " + dev + " shell pidof " + kPkg, out, 6000);
    std::string pid = ParsePid(out);
    if (!pid.empty()) return pid;

    // pidof absent sur certains builds -> ps (la ligne contient le package)
    const char* psCmds[] = { "ps -A", "ps" };
    for (const char* ps : psCmds) {
        Adb("-s " + dev + " shell " + ps, out, 8000);
        std::istringstream ss(out);
        std::string line;
        while (std::getline(ss, line)) {
            if (line.find(kPkg) == std::string::npos) continue;
            std::istringstream ls(line);
            std::string tok;
            while (ls >> tok) {
                if (!tok.empty() &&
                    tok.find_first_not_of("0123456789") == std::string::npos) {
                    pid = tok;
                    break;
                }
            }
            if (!pid.empty()) break;
        }
        if (!pid.empty()) break;
    }
    return pid;
}

// ------------------------------------------------------ extraction RCDATA ---
static std::string TempDir() {
    char buf[MAX_PATH] = {};
    GetTempPathA(MAX_PATH, buf);
    std::string dir = std::string(buf) + "0xvoid_ak\\";
    CreateDirectoryA(dir.c_str(), nullptr);
    return dir;
}

static bool ExtractRc(int id, const std::string& dest) {
    HRSRC hr = FindResourceW(g_hModule, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!hr) return false;
    HGLOBAL hg = LoadResource(g_hModule, hr);
    if (!hg) return false;
    const char* data = static_cast<const char*>(LockResource(hg));
    const DWORD size = SizeofResource(g_hModule, hr);
    if (!data || !size) return false;

    HANDLE h = CreateFileA(dest.c_str(), GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    const BOOL ok = WriteFile(h, data, size, &written, nullptr);
    CloseHandle(h);
    return ok && written == size;
}

// ------------------------------------------------------------ injection ----
// Fidele a Menu.java InjectX32 (2129-2158) : push, chmod 777, su -c inject,
// rm -f des fichiers pousses.  PAS de verification /proc/maps : le menu APK
// n'en fait pas, et en -dl_memfd le chemin affiche n'est pas forcement le
// nom du fichier -> le handshake est l'arbitre final.
static bool InjectPayload(const std::string& dev, std::string& errOut) {
    const std::string tmp = TempDir();

    SetStatus(LinkState::Busy, "Extracting embedded libs...");
    if (!ExtractRc(IDR_LIB_TERMINALX999, tmp + kLib)) {
        errOut = "extract payload failed"; return false;
    }
    if (!ExtractRc(IDR_LIB_INJECTEMU, tmp + "libinjectEmulator.so")) {
        errOut = "extract injector failed"; return false;
    }

    SetStatus(LinkState::Busy, "Pushing libs to device...");
    auto push = [&](const std::string& local, const std::string& remote,
                    const char* dbg) {
        std::string o;
        if (Adb("-s " + dev + " push \"" + local + "\" " + remote, o, 60000) == 0)
            return true;
        const std::string log = std::string("[aimkill] ") + dbg + " KO\n" + o;
        OutputDebugStringA(log.c_str());
        return false;
    };
    if (!push(tmp + "libinjectEmulator.so", kInjRemote, "push libinject")) {
        errOut = "adb push failed (permissions ?)"; return false;
    }
    // AndKittyInjector : -pkg + -lib suffisent.  -open n'est pas un drapeau
    // reconnu (ignore silencieusement) et -dl_memfd utilise dlopen_ext, non
    // supporte par houdini -> handle 0x0, lib jamais chargee.  Chargement par
    // chemin classique = seul mode qui fonctionne ici.
    const std::string inner = std::string(kInjRemote) + " -pkg " + kPkg +
                              " -lib " + kLibRemote;
    const char* suPaths[] = {
        "/boot/android/android/system/xbin/bstk/su",   // priorite BlueStacks (Menu.java)
        "/system/xbin/bstk/su",
        "/system/xbin/su",
        "su",
    };

    // Un nettoyeur externe peut supprimer /data/local/tmp/libinject en
    // quelques secondes (rc 127 a l'exec), et un kill-server adb concurrent
    // peut faire echouer un push : 2 tentatives completes push+chmod+su.
    bool injected = false;
    std::string lastErr;
    for (int attempt = 0; attempt < 2 && !injected && !g_shutdown; ++attempt) {
        if (attempt) {
            SetStatus(LinkState::Busy, "Retrying push / injection...");
            Sleep(400);
            if (!push(tmp + "libinjectEmulator.so", kInjRemote, "push libinject retry")) {
                lastErr = "adb push failed (permissions ?)"; continue;
            }
        }
        if (!push(tmp + kLib, kLibRemote, "push payload")) {
            lastErr = "adb push failed (permissions ?)"; continue;
        }
        if (!push(tmp + kLib, kAliasRemote, "push alias")) {
            lastErr = "adb push failed (permissions ?)"; continue;
        }

        {
            std::string o;
            Adb("-s " + dev + " shell chmod 777 " + kInjRemote + " " + kLibRemote +
                " " + kAliasRemote, o, 8000);
        }

        SetStatus(LinkState::Busy, "Injecting libTERMINALX999.so...");
        for (const char* su : suPaths) {
            if (g_shutdown) { errOut = "shutdown"; return false; }
            std::string o;
            // Toute la commande device est un SEUL argument Windows (double
            // quote) ; les quotes simples internes doivent etre PAIRES (4),
            // sinon mksh renvoie "no closing quote" et rien ne s'execute.
            const std::string devCmd = "'" + std::string(su) + "' -c '" + inner + "'";
            const int rc = Adb("-s " + dev + " shell \"" + devCmd + "\"", o, 30000);
            if (rc == 0) { injected = true; break; }
            const std::string dbg = std::string("[aimkill] su KO (") + su +
                                    ") rc=" + std::to_string(rc) + "\n" + o + "\n";
            OutputDebugStringA(dbg.c_str());
            lastErr = (rc == 127)
                ? "Injector missing on device - retry"
                : "Injection failed - lib not in game process";
        }
    }
    if (g_shutdown) { errOut = "shutdown"; return false; }
    if (!injected) {
        errOut = lastErr.empty() ? "Injection failed - lib not in game process"
                                 : lastErr;
        return false;
    }

    // le payload est deja en memoire (memfd) : on nettoie, comme Menu.java
    {
        std::string o;
        Adb("-s " + dev + " shell rm -f " + kInjRemote + " " + kLibRemote +
            " " + kAliasRemote, o, 8000);
    }
    return true;
}

// ------------------------------------------------------- forward UDS/adb ----
static bool SetupForward(const std::string& dev, std::string& errOut) {
    std::string o;
    Adb("-s " + dev + " forward --remove tcp:" + std::to_string(kPort), o, 8000);
    const int rc = Adb("-s " + dev + " forward tcp:" + std::to_string(kPort) +
                       " " + kAbstract, o, 8000);
    std::string low = o;
    for (char& c : low) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (rc != 0 || low.find("error") != std::string::npos ||
        low.find("failed") != std::string::npos || low.find("can't") != std::string::npos) {
        errOut = o.empty() ? ("adb forward rc=" + std::to_string(rc)) : o;
        while (!errOut.empty() && (errOut.back() == '\n' || errOut.back() == '\r'))
            errOut.pop_back();
        return false;
    }
    return true;
}

// ------------------------------------------------------------ Winsock -------
// err sortant : 0 = ok, -1 = fermeture propre (EOF), sinon WSAGetLastError().
static bool SendAll(SOCKET s, const char* buf, int len, int* err = nullptr) {
    while (len > 0) {
        const int r = send(s, buf, len, 0);
        if (r <= 0) { if (err) *err = WSAGetLastError(); return false; }
        buf += r; len -= r;
    }
    if (err) *err = 0;
    return true;
}

static bool RecvAll(SOCKET s, char* buf, int len, int* err = nullptr) {
    while (len > 0) {
        const int r = recv(s, buf, len, 0);
        if (r <= 0) {
            if (err) *err = (r == 0) ? -1 : WSAGetLastError();
            return false;
        }
        buf += r; len -= r;
    }
    if (err) *err = 0;
    return true;
}

static bool IsTimeoutErr(int err) {
    return err == WSAETIMEDOUT || err == WSAEWOULDBLOCK;
}

// Requete toggle : 32 octets, layout identique au backend (backend.cpp Request)
//   Mode@0 (i32) | boolean@4 (u8) | pad@5-7 | value@8 (i32)
//   ScreenWidth@12 | ScreenHeight@16 | pad@20-23 | playerPtr@24 (u64)
#pragma pack(push, 1)
struct Req {
    int32_t mode;
    uint8_t boolean;
    uint8_t pad1[3];
    int32_t value;
    int32_t sw;
    int32_t sh;
    uint8_t pad2[4];
    uint64_t ptr;
};
#pragma pack(pop)
static_assert(sizeof(Req) == 32, "Request wire size must be 32 bytes");

// Echange complet (appeler AVEC g_io pris) : requete + drain integral reponse.
//   success (optionnel) = premier octet de la Response = champ Success
//   errOut  (optionnel) = code de fin reseau (voir SendAll/RecvAll)
static bool ExchangeLocked(int mode, bool on, int32_t value,
                           bool* success = nullptr, int* errOut = nullptr) {
    if (errOut) *errOut = 0;
    if (g_sock == INVALID_SOCKET) {
        if (errOut) *errOut = -1;
        return false;
    }

    const uint32_t bodyLen = static_cast<uint32_t>(sizeof(Req));
    const uint32_t be = htonl(bodyLen);
    Req req = {};
    req.mode    = mode;
    req.boolean = on ? 1 : 0;
    req.value   = value;

    if (!SendAll(g_sock, reinterpret_cast<const char*>(&be), 4, errOut)) return false;
    if (!SendAll(g_sock, reinterpret_cast<const char*>(&req),
                 static_cast<int>(bodyLen), errOut)) return false;

    uint32_t rBe = 0;
    if (!RecvAll(g_sock, reinterpret_cast<char*>(&rBe), 4, errOut)) return false;
    const uint32_t rLen = ntohl(rBe);
    if (rLen == 0 || rLen > kMaxResponse) {
        if (errOut) *errOut = -1;
        return false;
    }

    std::vector<char> buf(rLen);
    if (!RecvAll(g_sock, buf.data(), static_cast<int>(rLen), errOut)) return false;
    if (success) *success = (buf[0] != 0);
    return true;
}

// Fermeture silencieuse de notre socket (aucun changement d'etat).
static void QuietClose() {
    std::lock_guard<std::mutex> io(g_io);
    if (g_sock != INVALID_SOCKET) {
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
    }
}

// Panne en cours de session : desynchronisation possible du flux -> on ferme
// (le serveur UDS meurt de toute facon) et on impose une re-injection.
static void AbortLink(const std::string& why) {
    QuietClose();
    SetStatus(LinkState::Error, why + " — press CONNECT LIB");
}

static bool TcpConnect() {
    if (g_shutdown) return false;
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return false;

    DWORD tv = kSndTimeoutMs;
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
    tv = kRcvTimeoutMs;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));

    sockaddr_in a = {};
    a.sin_family = AF_INET;
    a.sin_port   = htons(kPort);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);

    if (connect(s, reinterpret_cast<sockaddr*>(&a), sizeof(a)) == 0) {
        std::lock_guard<std::mutex> lk(g_io);
        if (g_sock != INVALID_SOCKET) closesocket(g_sock);
        g_sock = s;
        return true;
    }
    closesocket(s);
    return false;
}

// InitMode(1) : connecte + echange.  Ne touche PAS a l'etat global (c'est
// l'appelant qui decide du message d'erreur) ; ferme le socket si muet.
enum class Hs { None, Fail, Ok };

static Hs Handshake() {
    if (g_shutdown) return Hs::None;
    if (!TcpConnect()) return Hs::None;

    bool exchanged = false;
    int err = 0;
    {
        std::lock_guard<std::mutex> lk(g_io);
        exchanged = ExchangeLocked(1, true, 0, nullptr, &err);   // InitMode
    }
    if (exchanged) return Hs::Ok;

    QuietClose();
    // EOF/reset = pas de serveur derriere le forward (adb referme) -> None
    // timeout   = serveur la mais silencieux (client concurrent)      -> Fail
    return IsTimeoutErr(err) ? Hs::Fail : Hs::None;
}

// ------------------------------------------------------------- pipeline -----
static void ConnectThread() {
    // garantit g_busy = false a chaque sortie (declare en premier : detruit en dernier)
    struct BusyGuard {
        ~BusyGuard() { g_busy = false; }
    } guard;

    // --- 1. appareil + pid -------------------------------------------------
    SetStatus(LinkState::Busy, "Locating device (ADB)...");
    const std::string dev = FindDevice();
    if (dev.empty()) {
        SetStatus(LinkState::Error, "No ADB device — start BlueStacks & retry");
        return;
    }
    const std::string pid = GamePid(dev);
    if (pid.empty()) {
        SetStatus(LinkState::Error, "Free Fire not running — open the game & retry");
        return;
    }
    {
        const std::string m = "[aimkill] device=" + dev + " pid=" + pid + "\n";
        OutputDebugStringA(m.c_str());
    }

    // --- 2. forward UDS (necessaire aussi pour le pre-flight) ---------------
    SetStatus(LinkState::Busy, "Setting up UDS forward...");
    std::string err;
    if (!SetupForward(dev, err)) {
        SetStatus(LinkState::Error, "adb forward failed : " + err);
        return;
    }

    // --- 3. pre-flight : la lib est-elle deja branchee ? --------------------
    // (bouton idempotent : on ne re-injecte pas si le canal repond deja)
    const Hs pre = Handshake();
    if (pre == Hs::Ok) {
        SetStatus(LinkState::Busy, "Lib already connected — syncing...");
        SyncAll();
        if (g_state != LinkState::Error && !g_shutdown)
            SetStatus(LinkState::Connected, "CONNECTED 127.0.0.1:" + std::to_string(kPort));
        return;
    }
    if (g_shutdown) return;
    if (pre == Hs::Fail) {
        SetStatus(LinkState::Error,
                  "Lib server busy (menu APK ouvert ?) — close it or restart Free Fire");
        return;
    }
    OutputDebugStringA("[aimkill] pre-flight KO — injection requise\n");

    // --- 4. injection -------------------------------------------------------
    if (!InjectPayload(dev, err)) {
        if (!g_shutdown) SetStatus(LinkState::Error, err);
        return;
    }
    if (g_shutdown) return;

    // --- 5. forward de nouveau (au cas ou) + attente du serveur -------------
    if (!SetupForward(dev, err)) {
        SetStatus(LinkState::Error, "adb forward failed : " + err);
        return;
    }
    SetStatus(LinkState::Busy, "Waiting for lib server...");
    Hs h = Hs::None;
    for (int i = 0; i < 8 && !g_shutdown; ++i) {
        SetupForward(dev, err);   // kill-server adb externe : regle perdue
        h = Handshake();
        if (h != Hs::None) break;
        SetStatus(LinkState::Busy, "Waiting for lib server... (" +
                  std::to_string(i + 1) + "/8)");
        Sleep(500);
    }
    if (h != Hs::Ok) {
        if (!g_shutdown) {
            if (h == Hs::Fail)
                SetStatus(LinkState::Error,
                          "Lib server busy (menu APK ouvert ?) — close it or restart Free Fire");
            else
                SetStatus(LinkState::Error, "Lib not answering - restart Free Fire, then retry");
        }
        return;
    }

    // --- 6. replay de toute l'etat UI ---------------------------------------
    SetStatus(LinkState::Busy, "Syncing options...");
    SyncAll();
    if (g_state == LinkState::Error || g_shutdown) return;   // AbortLink a parle

    SetStatus(LinkState::Connected, "CONNECTED 127.0.0.1:" + std::to_string(kPort) +
                                     " — lib injected");
}

// ------------------------------------------------------------------ API -----
void StartConnect() {
    if (g_shutdown) {
        SetStatus(LinkState::Error, "Link unloaded — reinject to use again");
        return;
    }
    if (g_busy) return;   // pipeline deja en cours (bouton aussi desactive)

    // Deja connecte et socket vivant : simple re-synchronisation.
    // (verifier sous g_io, MAIS SyncAll prend g_io lui-meme -> on libere d'abord)
    bool alreadyConnected = false;
    {
        std::lock_guard<std::mutex> io(g_io);
        alreadyConnected = (g_state == LinkState::Connected && g_sock != INVALID_SOCKET);
    }
    if (alreadyConnected) {
        SetStatus(LinkState::Busy, "Re-syncing options...");
        SyncAll();
        if (g_state != LinkState::Error)
            SetStatus(LinkState::Connected, "CONNECTED 127.0.0.1:" +
                      std::to_string(kPort) + " — re-synced");
        return;
    }

    if (g_thread.joinable()) g_thread.join();   // precedent deja termine (g_busy=false)
    EnsureWsa();
    g_busy = true;
    SetStatus(LinkState::Busy, "Starting...");
    try {
        g_thread = std::thread(ConnectThread);
    } catch (...) {
        g_busy = false;
        SetStatus(LinkState::Error, "thread start failed");
    }
}

bool IsBusy() {
    return g_state == LinkState::Busy;
}

LinkState State() {
    return g_state;
}

std::string Status() {
    std::lock_guard<std::mutex> lk(g_statusMtx);
    return g_status;
}

bool IsReady() {
    std::lock_guard<std::mutex> io(g_io);
    return g_state == LinkState::Connected && g_sock != INVALID_SOCKET;
}

void SendToggle(int mode, bool on) {
    bool failed = false;
    {
        std::lock_guard<std::mutex> io(g_io);
        if (g_sock == INVALID_SOCKET || g_state != LinkState::Connected) return;
        bool success = false;
        failed = !ExchangeLocked(mode, on, 0, &success, nullptr);
        (void)success;   // mode inconnu du backend -> Success=false, sans effet
    }
    if (failed) AbortLink("Link lost (toggle)");
}

void SendFov(int value) {
    bool failed = false;
    {
        std::lock_guard<std::mutex> io(g_io);
        if (g_sock == INVALID_SOCKET || g_state != LinkState::Connected) return;
        failed = !ExchangeLocked(104, false, value, nullptr, nullptr);
    }
    if (failed) AbortLink("Link lost (FOV)");
}

void SyncAll() {
    // couple (mode wire, champ AimKill) de toutes les options qui parlent au lib
    struct Entry { int mode; bool Globals::AimKill::* field; };
    static const Entry kToggles[] = {
        // --- groupe AIM ---
        { 3,     &Globals::AimKill::ActivateAll },        // 102  -> Mode 3
        { 1044,  &Globals::AimKill::AimkillSend },        // 105  -> Mode 1044
        { 889,   &Globals::AimKill::SilentKill },         // 889  -> Mode 889
        { 1050,  &Globals::AimKill::SendAllFov },         // 1050 (no-op backend, fidele)
        { 5661,  &Globals::AimKill::SafeAimkill },        // 5661 -> Mode 5661
        { 5662,  &Globals::AimKill::RealAimkillV2 },      // 5662 -> Mode 5662
        { 1047,  &Globals::AimKill::AutoSwitchSend },     // 106  -> Mode 1047
        { 508,   &Globals::AimKill::AutoSwitchNormal },   // 508  -> Mode 508
        { 1048,  &Globals::AimKill::CoverElimination },   // 107  -> Mode 1048
        { 1049,  &Globals::AimKill::AimkillNoSwitch },    // 110  -> Mode 1049
        { 527,   &Globals::AimKill::HideDamage },         // 527  -> Mode 527
        { 509,   &Globals::AimKill::CoverPull },          // 509  -> Mode 509
        { 34834, &Globals::AimKill::AimLock },            // 34834 -> Mode 34834
        { 523,   &Globals::AimKill::TargetLock },         // 523  -> Mode 523
        // --- groupe BRTL ---
        { 532,   &Globals::AimKill::InvinsHack },         // 532  -> Mode 532
        { 521,   &Globals::AimKill::AutofireAllGuns },    // 521  -> Mode 521
        { 504,   &Globals::AimKill::DownKill },           // 5658 -> Mode 504
        { 5660,  &Globals::AimKill::DiveKillNew },        // 5660 -> Mode 5660
        { 515,   &Globals::AimKill::BrustFire },          // 515  -> Mode 515
        { 518,   &Globals::AimKill::RapidFire },          // 518  -> Mode 518
        { 519,   &Globals::AimKill::FastReload },         // 519  -> Mode 519
        { 5659,  &Globals::AimKill::FlyX40 },             // 5659 -> Mode 5659
        { 5002,  &Globals::AimKill::StableFly },          // 5002 -> Mode 5002
        { 56578, &Globals::AimKill::TeleportMark },       // 56578 -> Mode 56578
        { 5663,  &Globals::AimKill::SendPosition },       // 5663 -> Mode 5663
        // --- groupe KEYBIND ---
        { 5657,  &Globals::AimKill::FlyhackMini },        // 111  -> Mode 5657
        { 529,   &Globals::AimKill::HighFps },            // 529  -> Mode 529
    };

    bool failed = false;
    {
        std::lock_guard<std::mutex> io(g_io);
        if (g_sock == INVALID_SOCKET) return;
        for (const Entry& e : kToggles) {
            if (!ExchangeLocked(e.mode, g_Globals.AimKill.*(e.field), 0, nullptr, nullptr)) {
                failed = true;
                break;
            }
        }
        // AIMFOV : valeur entiere brute (backend lit int puis convertit en float)
        if (!failed)
            failed = !ExchangeLocked(104, false, g_Globals.AimKill.AimFov, nullptr, nullptr);
    }
    if (failed) { AbortLink("Link lost (sync)"); return; }

    // --- macro ACTIVATE ALL (Menu.java id 102) : force les ESP natives 0xVoid
    if (g_Globals.AimKill.ActivateAll) {
        g_Globals.Visuals.Lines      = true;   // ESP LINE     (id 1)
        g_Globals.Visuals.Box        = true;   // ESP BOX      (id 2)
        g_Globals.Visuals.HealthBar  = true;   // ESP HEALTH   (id 3)
        g_Globals.Visuals.Name       = true;   // ESP NAME     (id 9)
        g_Globals.Visuals.Distance   = true;   // ESP DISTANCE (id 4)
        g_Globals.Misc.ShowAimbotFov = true;   // DRAW FOV     (id 16)
    }
}

void Shutdown() {
    g_shutdown = true;                              // RunProc/loops sortent vite
    if (g_thread.joinable()) g_thread.join();       // attendre le pipeline (<= ~1s)

    // Socket volontairement NON ferme (voir en-tete) : fermer declencherait
    // Close() du serveur UDS + spin 100% CPU dans Free Fire. On marque l'etat.
    g_state = LinkState::Idle;
    {
        std::lock_guard<std::mutex> lk(g_statusMtx);
        g_status = "Disconnected";
    }
}

}   // namespace AimKillLink
