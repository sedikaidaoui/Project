// Winsock2 DOIT etre include avant <windows.h> (sinon conflit winsock.h/winsock2.h)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include <src/adb/adb_utils.hpp>
#include <src/adb/ProcessUtils.hpp>
#include <Windows.h>
#include <Psapi.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <EspLines/Offsets.hpp>

using namespace ProcessUtils;

namespace FWork {
    namespace ADB {

        // -------------------------------------------------------
        // Tìm đường dẫn HD-Adb.exe của BlueStacks
        // -------------------------------------------------------
        static std::string GetAdbPath() {
            // 1. Thư mục cạnh DLL/EXE
            char buf[MAX_PATH] = {};
            GetModuleFileNameA(NULL, buf, MAX_PATH);
            std::string exePath(buf);
            size_t pos = exePath.find_last_of("\\/");
            std::string exeDir = (pos != std::string::npos) ? exePath.substr(0, pos) : ".";

            if (GetFileAttributesA((exeDir + "\\HD-Adb.exe").c_str()) != INVALID_FILE_ATTRIBUTES)
                return exeDir + "\\HD-Adb.exe";
            if (GetFileAttributesA((exeDir + "\\adb.exe").c_str()) != INVALID_FILE_ATTRIBUTES)
                return exeDir + "\\adb.exe";

            // 2. BlueStacks standard paths only
            const char* paths[] = {
                "C:\\Program Files\\BlueStacks_nxt\\HD-Adb.exe",
                "C:\\Program Files (x86)\\BlueStacks_nxt\\HD-Adb.exe",
                "C:\\Program Files\\BlueStacks\\HD-Adb.exe",
                "C:\\Program Files (x86)\\BlueStacks\\HD-Adb.exe",
            };
            for (const char* p : paths) {
                if (GetFileAttributesA(p) != INVALID_FILE_ATTRIBUTES)
                    return p;
            }

            return "adb.exe"; // fallback
        }

        // -------------------------------------------------------
        // Chạy lệnh ADB và trả về output
        // -------------------------------------------------------
        static std::string ExecuteAdbRaw(const std::string& command) {
            std::string adbPath = GetAdbPath();
            std::string fullCmd = "\"" + adbPath + "\" " + command;

            HANDLE hRead, hWrite;
            SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
            if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return "";
            SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

            STARTUPINFOA si = {};
            si.cb = sizeof(si);
            si.hStdOutput = hWrite;
            si.hStdError  = hWrite;
            si.dwFlags    = STARTF_USESTDHANDLES;
            si.wShowWindow = SW_HIDE;

            PROCESS_INFORMATION pi = {};
            if (!CreateProcessA(NULL, (LPSTR)fullCmd.c_str(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                CloseHandle(hWrite);
                CloseHandle(hRead);
                return "";
            }
            CloseHandle(hWrite);

            char buffer[256];
            DWORD read;
            std::string output;
            while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &read, NULL) && read > 0) {
                buffer[read] = '\0';
                output += buffer;
            }

            WaitForSingleObject(pi.hProcess, 3000);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            CloseHandle(hRead);
            return output;
        }

        // -------------------------------------------------------
        // Tìm device BlueStacks đang chạy Free Fire
        // -------------------------------------------------------
        static std::string g_TargetDevice;

        static std::string GetTargetDevice() {
            if (!g_TargetDevice.empty()) return g_TargetDevice;

            // Lấy danh sách device đang kết nối
            std::string devOut = ExecuteAdbRaw("devices");
            std::stringstream ss(devOut);
            std::string line;
            std::vector<std::string> devices;

            while (std::getline(ss, line)) {
                if (line.find("List") != std::string::npos) continue;
                if (line.find("device") != std::string::npos && line.find("offline") == std::string::npos) {
                    size_t tab = line.find('\t');
                    if (tab != std::string::npos)
                        devices.push_back(line.substr(0, tab));
                }
            }

            // Kiểm tra device nào đang chạy Free Fire (com.dts.freefireth)
            for (const auto& dev : devices) {
                std::string pidOut = ExecuteAdbRaw("-s " + dev + " shell pidof com.dts.freefireth");
                if (pidOut.find_first_of("0123456789") != std::string::npos &&
                    pidOut.find("error")     == std::string::npos &&
                    pidOut.find("not found") == std::string::npos) {
                    g_TargetDevice = dev;
                    std::cout << "[ADB] Found Free Fire on device: " << dev << std::endl;
                    return g_TargetDevice;
                }
            }

            // Nếu chưa có device nào → thử port BlueStacks phổ biến
            if (devices.empty()) {
                int ports[] = { 5554, 5555, 5556, 5557, 5558, 5565, 5575, 5585, 5595 };
                for (int port : ports) {
                    std::string dev = "127.0.0.1:" + std::to_string(port);
                    ExecuteAdbRaw("connect " + dev);
                    std::string pidOut = ExecuteAdbRaw("-s " + dev + " shell pidof com.dts.freefireth");
                    if (pidOut.find_first_of("0123456789") != std::string::npos &&
                        pidOut.find("error")     == std::string::npos &&
                        pidOut.find("not found") == std::string::npos) {
                        g_TargetDevice = dev;
                        std::cout << "[ADB] Found Free Fire on port: " << port << std::endl;
                        return g_TargetDevice;
                    }
                }
            }

            return "";
        }

        // -------------------------------------------------------
        // Chọn bất kỳ device nào đang kết nối (KHÔNG cần Free Fire
        // đang chạy — dùng để khởi động game lần đầu)
        // -------------------------------------------------------
        static std::string GetAnyDevice() {
            if (!g_TargetDevice.empty()) return g_TargetDevice;

            std::string devOut = ExecuteAdbRaw("devices");
            std::stringstream ss(devOut);
            std::string line;
            while (std::getline(ss, line)) {
                if (line.find("List") != std::string::npos) continue;
                if (line.find("device") != std::string::npos &&
                    line.find("offline") == std::string::npos) {
                    size_t tab = line.find('\t');
                    if (tab != std::string::npos)
                        return line.substr(0, tab);
                }
            }

            int ports[] = { 5555, 5554, 5556, 5557, 5558, 5565, 5575, 5585, 5595 };
            for (int port : ports) {
                std::string dev = "127.0.0.1:" + std::to_string(port);
                ExecuteAdbRaw("connect " + dev);
                std::string out = ExecuteAdbRaw("-s " + dev + " shell echo ok");
                if (out.find("ok") != std::string::npos)
                    return dev;
            }

            return "";
        }

        // -------------------------------------------------------
        // Ping de l'emulator — RTT TCP vers 127.0.0.1:5555 (BlueStacks).
        // 100% non bloquant : une machine d'etat qui avance toute seule
        // a chaque appel. Une seule tentative en vol a la fois.
        // -------------------------------------------------------
        static bool           g_PingWsaOk   = false;
        static bool           g_PingChecked = false;   // g_PingWsaOk deja fait
        static int            g_PingPortIdx = 0;
        static SOCKET         g_PingSock    = INVALID_SOCKET;
        static unsigned long  g_PingT0      = 0;       // debut de l'essai en cours
        static unsigned long  g_PingNext    = 0;       // prochain tick autorise
        static int            g_PingLastMs  = -1;
        static const int      kPingPorts[]  = { 5555, 5554, 5556, 5557, 5558, 5565, 5575, 5585, 5595 };
        static const int      kPingNPorts   = (int)(sizeof(kPingPorts) / sizeof(kPingPorts[0]));
        static const unsigned kPingTimeout  = 400;     // ms max par tentative
        static const unsigned kPingOkWait   = 2000;    // ms avant de retester apres succes
        static const unsigned kPingGap      = 250;     // ms entre deux ports apres echec

        static void PingCloseSocket() {
            if (g_PingSock != INVALID_SOCKET) {
                closesocket(g_PingSock);
                g_PingSock = INVALID_SOCKET;
            }
        }

        int GetEmulatorPingMs() {
            const unsigned long now = GetTickCount();

            // Winsock, une seule fois
            if (!g_PingChecked) {
                WSADATA wsa;
                g_PingWsaOk   = (WSAStartup(MAKEWORD(2, 2), &wsa) == 0);
                g_PingChecked = true;
                if (!g_PingWsaOk) return -1;
            }

            // Rien a faire pour l'instant (on renvoie le dernier resultat)
            if (now < g_PingNext) return g_PingLastMs;

            // Pas d'appareil -> on coupe et on reessaie plus tard
            if (g_TargetDevice.empty() && GetTargetDevice().empty()) {
                PingCloseSocket();
                g_PingLastMs  = -1;
                g_PingNext    = now + 1500;
                g_PingPortIdx = 0;
                return -1;
            }

            // Pas de socket -> on lance une connexion non bloquante
            if (g_PingSock == INVALID_SOCKET) {
                g_PingSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (g_PingSock == INVALID_SOCKET) {
                    g_PingLastMs = -1;
                    g_PingNext   = now + 1000;
                    return -1;
                }

                u_long nb = 1;
                ioctlsocket(g_PingSock, FIONBIO, &nb);

                sockaddr_in sa = {};
                sa.sin_family      = AF_INET;
                sa.sin_port        = htons((u_short)kPingPorts[g_PingPortIdx]);
                sa.sin_addr.s_addr = inet_addr("127.0.0.1");

                g_PingT0 = now;
                if (connect(g_PingSock, (sockaddr*)&sa, sizeof(sa)) == 0) {
                    // Connecte instantanement = meme machine -> 0 ms
                    PingCloseSocket();
                    g_PingLastMs = 0;
                    g_PingNext   = now + kPingOkWait;
                    return 0;
                }
                // Sinon WSAEWOULDBLOCK : on attend, plus bas
            }

            // La connexion a-t-elle abouti / echoue ?
            fd_set wr; FD_ZERO(&wr); FD_SET(g_PingSock, &wr);
            fd_set er; FD_ZERO(&er); FD_SET(g_PingSock, &er);
            timeval tv; tv.tv_sec = 0; tv.tv_usec = 0;   // select non bloquant
            const int r = select(0, nullptr, &wr, &er, &tv);

            if (r > 0 && (FD_ISSET(g_PingSock, &wr) || FD_ISSET(g_PingSock, &er))) {
                int soErr = 0; int len = sizeof(soErr);
                getsockopt(g_PingSock, SOL_SOCKET, SO_ERROR, (char*)&soErr, &len);
                if (soErr == 0) {
                    // Succes -> RTT = temps mis pour se connecter
                    g_PingLastMs = (int)(now - g_PingT0);
                    PingCloseSocket();
                    g_PingNext = now + kPingOkWait;
                    return g_PingLastMs;
                }
                // Refuse / reset -> port suivant
            }

            // Timeout ou erreur -> on passe au port suivant
            if (r > 0 || (now - g_PingT0) >= kPingTimeout) {
                PingCloseSocket();
                g_PingPortIdx = (g_PingPortIdx + 1) % kPingNPorts;
                g_PingLastMs  = -1;
                g_PingNext    = now + kPingGap;
                return -1;
            }

            // Toujours en cours de connexion
            return g_PingLastMs;
        }

        // -------------------------------------------------------
        // Khởi động Free Fire trên emulator
        // -------------------------------------------------------
        bool LaunchFreeFire() {
            std::string device = GetAnyDevice();
            if (device.empty()) {
                std::cout << "[ADB] LaunchFreeFire: no device found" << std::endl;
                return false;
            }

            const char* pkg = "com.dts.freefireth";

            ExecuteAdbRaw("-s " + device + " shell input keyevent KEYCODE_WAKEUP");
            ExecuteAdbRaw("-s " + device + " shell am force-stop " + pkg);

            // 1) monkey — không cần biết tên activity
            std::string out = ExecuteAdbRaw(
                "-s " + device + " shell monkey -p " + pkg +
                " -c android.intent.category.LAUNCHER 1");

            if (out.find("Events injected") != std::string::npos) {
                std::cout << "[ADB] Free Fire launched (monkey) on " << device << std::endl;
                return true;
            }

            // 2) fallback — am start trên activity chính
            ExecuteAdbRaw("-s " + device +
                " shell am start -n " + pkg + "/com.garena.game.ff");

            std::cout << "[ADB] Free Fire launched (am start) on " << device << std::endl;
            return true;
        }

        // -------------------------------------------------------
        // Trích xuất địa chỉ đầu tiên từ output /proc/maps
        // -------------------------------------------------------
        static std::string ExtractLibAddress(const std::string& input) {
            std::stringstream ss(input);
            std::string line;
            while (std::getline(ss, line)) {
                size_t dashPos = line.find('-');
                if (dashPos != std::string::npos && dashPos > 0 && dashPos < 20) {
                    if (isxdigit((unsigned char)line[dashPos - 1]) &&
                        isxdigit((unsigned char)line[dashPos + 1])) {
                        size_t start = dashPos;
                        while (start > 0 && isxdigit((unsigned char)line[start - 1]))
                            start--;
                        return line.substr(start, dashPos - start);
                    }
                }
            }
            return "";
        }

        // -------------------------------------------------------
        // Chạy lệnh shell với su (thử nhiều su path của BlueStacks)
        // -------------------------------------------------------
        static std::string ExecuteShellWithSu(const std::string& shellCmd) {
            std::string adbPath = GetAdbPath();
            std::string target  = GetTargetDevice();
            std::string targetPrefix = target.empty() ? "" : "-s " + target + " ";

            const char* suPaths[] = {
                "/boot/android/android/system/xbin/bstk/su",
                "/system/xbin/bstk/su",
                "/system/xbin/su",
                "su",
                ""  // no su
            };

            // Escape dấu " trong lệnh
            std::string escaped;
            for (char c : shellCmd) {
                if (c == '"') escaped += "\\\"";
                else escaped += c;
            }

            for (const char* su : suPaths) {
                std::string fullCmd;
                if (strlen(su) > 0)
                    fullCmd = "\"" + adbPath + "\" " + targetPrefix + "shell \"" + su + " -c '" + escaped + "'\"";
                else
                    fullCmd = "\"" + adbPath + "\" " + targetPrefix + "shell \"" + escaped + "\"";

                HANDLE hRead, hWrite;
                SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
                if (!CreatePipe(&hRead, &hWrite, &sa, 0)) continue;
                SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

                STARTUPINFOA si = {};
                si.cb = sizeof(si);
                si.hStdOutput = hWrite;
                si.hStdError  = hWrite;
                si.dwFlags    = STARTF_USESTDHANDLES;
                si.wShowWindow = SW_HIDE;

                PROCESS_INFORMATION pi = {};
                if (!CreateProcessA(NULL, (LPSTR)fullCmd.c_str(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                    CloseHandle(hWrite);
                    CloseHandle(hRead);
                    continue;
                }
                CloseHandle(hWrite);

                char buffer[256];
                DWORD read;
                std::string output;
                while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &read, NULL) && read > 0) {
                    buffer[read] = '\0';
                    output += buffer;
                    if (!ExtractLibAddress(output).empty()) {
                        TerminateProcess(pi.hProcess, 0);
                        break;
                    }
                }

                WaitForSingleObject(pi.hProcess, 3000);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                CloseHandle(hRead);

                std::string result = ExtractLibAddress(output);
                if (!result.empty()) return result;
            }

            return "";
        }

        // -------------------------------------------------------
        // Kill ADB processes (BlueStacks)
        // -------------------------------------------------------
        void KillAdb() {
            DWORD pid = GetProc(L"adb.exe");
            KillProc(pid);
            DWORD pid2 = 0;
            if (IsProcRun(L"adb.exe", pid2)) ForgeKillProc(pid2);

            DWORD hdpid = GetProc(L"HD-Adb.exe");
            KillProc(hdpid);
            DWORD hdpid2 = 0;
            if (IsProcRun(L"HD-Adb.exe", hdpid2)) ForgeKillProc(hdpid2);
        }

        // -------------------------------------------------------
        // Entry point chính — tìm base của libil2cpp.so qua ADB
        // -------------------------------------------------------
        bool InitializeADB() {
            std::cout << "[ADB] Initializing via BlueStacks ADB..." << std::endl;

            // Reset cached device khi retry
            g_TargetDevice = "";

            // Kill ADB cũ để tránh conflict
            KillAdb();
            Sleep(300);

            std::string device = GetTargetDevice();
            if (device.empty()) {
                std::cerr << "[ADB] Free Fire (com.dts.freefireth) not found on any BlueStacks device." << std::endl;
                Offsets::Il2Cpp = 0;
                return false;
            }

            // Lấy PID của Free Fire
            std::string pidOut = ExecuteAdbRaw("-s " + device + " shell pidof com.dts.freefireth");
            // Trim whitespace
            pidOut.erase(std::remove_if(pidOut.begin(), pidOut.end(), [](char c){ return c == '\r' || c == '\n' || c == ' '; }), pidOut.end());

            if (pidOut.empty() || pidOut.find_first_not_of("0123456789") != std::string::npos) {
                std::cerr << "[ADB] Failed to get PID of Free Fire." << std::endl;
                Offsets::Il2Cpp = 0;
                return false;
            }
            std::cout << "[ADB] Free Fire PID: " << pidOut << std::endl;

            // Tìm libil2cpp.so trong /proc/PID/maps
            std::string mapsCmd = "grep libil2cpp.so /proc/" + pidOut + "/maps";
            std::string addrStr = ExecuteShellWithSu(mapsCmd);

            if (addrStr.empty()) {
                std::cerr << "[ADB] libil2cpp.so not found in maps." << std::endl;
                Offsets::Il2Cpp = 0;
                return false;
            }

            // Chuyển hex string → uintptr_t
            uintptr_t base = 0;
            try {
                base = (uintptr_t)strtoull(addrStr.c_str(), nullptr, 16);
            } catch (...) {
                base = 0;
            }

            if (base == 0) {
                std::cerr << "[ADB] Failed to parse address: " << addrStr << std::endl;
                Offsets::Il2Cpp = 0;
                return false;
            }

            Offsets::Il2Cpp = base;
            std::cout << "[ADB] libil2cpp.so base: 0x" << std::hex << base << std::dec << std::endl;
            return true;
        }

    } // namespace ADB
} // namespace FWork