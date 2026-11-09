#include <src/adb/adb_utils.hpp>
#include <src/adb/ProcessUtils.hpp>
#include <Windows.h>
#include <shellapi.h>
#include <Psapi.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <EspLines/Offsets.hpp>

using namespace ProcessUtils;

namespace FWork {
    namespace ADB {

        // -------------------------------------------------------
        // Tìm đường dẫn HD-Adb.exe / adb.exe (BlueStacks, MSI App Player,
        // LDPlayer, MEmu, Nox...) — cached sau lần dò đầu tiên
        // -------------------------------------------------------
        static std::string ResolveAdbPath() {
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

            // 2. Đường dẫn chuẩn của các emulator phổ biến
            const char* paths[] = {
                "C:\\Program Files\\BlueStacks_msi5\\HD-Adb.exe",
                "C:\\Program Files\\BlueStacks_nxt\\HD-Adb.exe",
                "C:\\Program Files (x86)\\BlueStacks_nxt\\HD-Adb.exe",
                "C:\\Program Files\\BlueStacks\\HD-Adb.exe",
                "C:\\Program Files (x86)\\BlueStacks\\HD-Adb.exe",
                "C:\\Program Files\\MSI\\MSI App Player\\HD-Adb.exe",
                "C:\\Program Files (x86)\\MSI\\MSI App Player\\HD-Adb.exe",
                "C:\\Program Files\\MSI App Player\\HD-Adb.exe",
                "C:\\Program Files (x86)\\MSI App Player\\HD-Adb.exe",
                "C:\\Program Files\\Nox\\bin\\adb.exe",
                "C:\\Program Files (x86)\\Nox\\bin\\adb.exe",
                "C:\\Program Files\\Microvirt\\MEmu\\adb.exe",
                "C:\\Program Files\\Tencent\\GameLoop\\adb.exe",
                "C:\\LDPlayer\\LDPlayer9\\adb.exe",
                "C:\\LDPlayer\\LDPlayer4\\adb.exe",
            };
            for (const char* p : paths) {
                if (GetFileAttributesA(p) != INVALID_FILE_ATTRIBUTES)
                    return p;
            }

            // 3. Quét 1 cấp trong Program Files / Program Files (x86)
            const char* roots[] = { "C:\\Program Files", "C:\\Program Files (x86)" };
            for (const char* root : roots) {
                std::string pattern = std::string(root) + "\\*";
                WIN32_FIND_DATAA fd = {};
                HANDLE h = FindFirstFileA(pattern.c_str(), &fd);
                if (h == INVALID_HANDLE_VALUE) continue;
                do {
                    if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
                    if (fd.cFileName[0] == '.') continue;
                    std::string base = std::string(root) + "\\" + fd.cFileName;
                    const char* subs[] = { "\\HD-Adb.exe", "\\adb.exe", "\\bin\\adb.exe" };
                    for (const char* sub : subs) {
                        std::string candidate = base + sub;
                        if (GetFileAttributesA(candidate.c_str()) != INVALID_FILE_ATTRIBUTES) {
                            FindClose(h);
                            return candidate;
                        }
                    }
                } while (FindNextFileA(h, &fd));
                FindClose(h);
            }

            return "adb.exe"; // fallback
        }

        static std::string GetAdbPath() {
            static const std::string cached = ResolveAdbPath();
            return cached;
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
        static std::string g_LastError;

        const std::string& GetLastError() { return g_LastError; }

        static bool HasRootAccess(const std::string& dev) {
            std::string out = ExecuteAdbRaw("-s " + dev + " shell \"su -c id\"");
            return out.find("uid=0") != std::string::npos;
        }

        void DisconnectAll() {
            // Graceful cleanup of stale entries left by previous connect loop.
            // Without this, "adb devices" stays polluted and next run skips
            // the manual connect phase, appearing as 5555 -> 5556 shift.
            ExecuteAdbRaw("disconnect");
        }

        void ResetTargetDevice() {
            g_TargetDevice = "";
        }

        void ShutdownADB() {
            // Called from Unload / emulator close: leave adb server clean
            // so next emulator instance can rebind 5555 instead of 5556.
            DisconnectAll();
            g_TargetDevice = "";
            Offsets::Il2Cpp = 0;
        }

        static bool DeviceHasFreeFire(const std::string& dev) {
            std::string pidOut = ExecuteAdbRaw("-s " + dev + " shell pidof com.dts.freefireth");
            return (pidOut.find_first_of("0123456789") != std::string::npos &&
                    pidOut.find("error")     == std::string::npos &&
                    pidOut.find("not found") == std::string::npos);
        }

        static std::vector<int> GetConfiguredAdbPorts();

        static std::string GetTargetDevice() {
            // FIX: re-validate cached device, old emulator may be closed
            // and port reused by a new instance.
            if (!g_TargetDevice.empty()) {
                if (DeviceHasFreeFire(g_TargetDevice))
                    return g_TargetDevice;
                g_TargetDevice = "";
            }

            // Lấy danh sách device đang kết nối
            std::string devOut = ExecuteAdbRaw("devices");
            std::stringstream ss(devOut);
            std::string line;
            std::vector<std::string> devices;

            while (std::getline(ss, line)) {
                if (line.find("List") != std::string::npos) continue;
                if (line.find("device") != std::string::npos && line.find("offline") == std::string::npos) {
                    size_t tab = line.find('\t');
                    if (tab != std::string::npos) {
                        std::string dev = line.substr(0, tab);
                        // trim \r
                        dev.erase(std::remove(dev.begin(), dev.end(), '\r'), dev.end());
                        if (!dev.empty())
                            devices.push_back(dev);
                    }
                }
            }

            // Kiểm tra device nào đang chạy Free Fire (com.dts.freefireth)
            for (const auto& dev : devices) {
                if (DeviceHasFreeFire(dev)) {
                    g_TargetDevice = dev;
                    std::cout << "[ADB] Found Free Fire on device: " << dev << std::endl;
                    return g_TargetDevice;
                }
            }

            // Danh sách cổng: đọc từ bluestacks.conf (MSI App Player /
            // BlueStacks 5) + các cổng mặc định.
            const std::vector<int> ports = GetConfiguredAdbPorts();
            std::vector<std::string> connectedNow;
            for (int port : ports) {
                std::string dev = "127.0.0.1:" + std::to_string(port);
                // Skip ports already present in devices list (already tested above)
                bool alreadyListed = false;
                for (const auto& d : devices) { if (d == dev) { alreadyListed = true; break; } }
                if (!alreadyListed) {
                    ExecuteAdbRaw("disconnect " + dev);
                    ExecuteAdbRaw("connect " + dev);
                    connectedNow.push_back(dev);
                }
                if (DeviceHasFreeFire(dev)) {
                    g_TargetDevice = dev;
                    std::cout << "[ADB] Found Free Fire on port: " << port << std::endl;
                    // Cleanup siblings we just connected that have no FF
                    for (const auto& c : connectedNow) {
                        if (c != dev)
                            ExecuteAdbRaw("disconnect " + c);
                    }
                    return g_TargetDevice;
                } else if (!alreadyListed) {
                    // Immediate cleanup: don't leave failed connects behind
                    ExecuteAdbRaw("disconnect " + dev);
                    connectedNow.pop_back();
                }
            }

            // Cleanup stale 127.0.0.1 entries from previous runs
            for (const auto& dev : devices) {
                if (dev.find("127.0.0.1:") == 0 && !DeviceHasFreeFire(dev))
                    ExecuteAdbRaw("disconnect " + dev);
            }

            return "";
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
        // Cổng ADB đọc từ bluestacks.conf (MSI App Player / BlueStacks 5)
        // -------------------------------------------------------
        static std::vector<int> GetConfiguredAdbPorts() {
            std::vector<int> ports;
            const char* confs[] = {
                "C:\\ProgramData\\BlueStacks_msi5\\bluestacks.conf",
                "C:\\ProgramData\\BlueStacks_nxt\\bluestacks.conf",
                "C:\\ProgramData\\BlueStacks\\bluestacks.conf",
                "C:\\ProgramData\\BlueStacks_msi\\bluestacks.conf",
                "C:\\ProgramData\\BlueStacks_msi2\\bluestacks.conf",
                "C:\\ProgramData\\MSI\\MSI App Player\\bluestacks.conf",
                "C:\\ProgramData\\MSI App Player\\bluestacks.conf",
            };
            for (const char* conf : confs) {
                std::ifstream in(conf);
                if (!in.is_open()) continue;
                std::string line;
                while (std::getline(in, line)) {
                    if (line.find("adb_port") == std::string::npos) continue;
                    size_t q = line.find('"');
                    size_t start = (q != std::string::npos)
                        ? line.find_first_of("0123456789", q)
                        : line.find_first_of("0123456789");
                    if (start == std::string::npos) continue;
                    int port = std::atoi(line.c_str() + start);
                    if (port > 0) ports.push_back(port);
                }
            }

            for (int p : { 5555, 5556, 5557, 5558, 5565, 5575, 5585, 5595 })
                ports.push_back(p);

            std::vector<int> unique;
            for (int p : ports)
                if (std::find(unique.begin(), unique.end(), p) == unique.end())
                    unique.push_back(p);
            return unique;
        }

        static std::vector<std::string> ListOnlineDevices() {
            std::vector<std::string> devices;
            std::string devOut = ExecuteAdbRaw("devices");
            std::stringstream ss(devOut);
            std::string line;
            while (std::getline(ss, line)) {
                if (line.find("List") != std::string::npos) continue;
                size_t tab = line.find('\t');
                if (tab == std::string::npos) continue;
                std::string rest = line.substr(tab + 1);
                if (rest.find("device") == std::string::npos) continue;
                if (rest.find("offline") != std::string::npos) continue;
                if (rest.find("unauthorized") != std::string::npos) continue;
                std::string dev = line.substr(0, tab);
                dev.erase(std::remove(dev.begin(), dev.end(), '\r'), dev.end());
                if (!dev.empty()) devices.push_back(dev);
            }
            return devices;
        }

        static bool DeviceHasFreeFirePackage(const std::string& dev) {
            std::string out = ExecuteAdbRaw("-s " + dev + " shell pm path com.dts.freefireth");
            return out.find("package:") != std::string::npos;
        }

        // Device online, ưu tiên device đã cài Free Fire
        static std::string FindAnyOnlineDevice() {
            const std::vector<int> ports = GetConfiguredAdbPorts();

            std::vector<std::string> devices = ListOnlineDevices();
            for (const auto& d : devices)
                if (DeviceHasFreeFirePackage(d)) return d;
            if (!devices.empty()) return devices.front();

            for (int port : ports) {
                std::string dev = "127.0.0.1:" + std::to_string(port);
                ExecuteAdbRaw("connect " + dev);
                Sleep(250);
                if (DeviceHasFreeFirePackage(dev)) return dev;
            }

            devices = ListOnlineDevices();
            if (!devices.empty()) return devices.front();

            for (int port : ports) {
                std::string dev = "127.0.0.1:" + std::to_string(port);
                std::string listOut = ExecuteAdbRaw("devices");
                if (listOut.find(dev + "\tdevice") != std::string::npos)
                    return dev;
            }
            return "";
        }

        // Tên instance trong bluestacks.conf (mặc định Pie64)
        static std::string GetInstanceName() {
            const char* confs[] = {
                "C:\\ProgramData\\BlueStacks_msi5\\bluestacks.conf",
                "C:\\ProgramData\\BlueStacks_nxt\\bluestacks.conf",
                "C:\\ProgramData\\BlueStacks\\bluestacks.conf",
                "C:\\ProgramData\\MSI\\MSI App Player\\bluestacks.conf",
                "C:\\ProgramData\\MSI App Player\\bluestacks.conf",
            };
            for (const char* conf : confs) {
                std::ifstream in(conf);
                if (!in.is_open()) continue;
                std::string line;
                while (std::getline(in, line)) {
                    const std::string key = "bst.instance.";
                    size_t at = line.find(key);
                    if (at == std::string::npos) continue;
                    if (line.find(".display_name") == std::string::npos) continue;
                    size_t nameStart = at + key.size();
                    size_t nameEnd = line.find('.', nameStart);
                    if (nameEnd == std::string::npos) continue;
                    std::string name = line.substr(nameStart, nameEnd - nameStart);
                    if (!name.empty()) return name;
                }
            }
            return "Pie64";
        }

        // Mở emulator nếu chưa chạy (MSI App Player / BlueStacks)
        static bool StartBlueStacksPlayer() {
            std::string adbPath = GetAdbPath();
            size_t pos = adbPath.find_last_of("\\/");
            std::string dir = (pos != std::string::npos) ? adbPath.substr(0, pos) : "";

            std::vector<std::string> candidates;

            // Nếu máy có MSI App Player (BlueStacks_msi5) thì ưu tiên nó
            if (GetFileAttributesA("C:\\ProgramData\\BlueStacks_msi5\\bluestacks.conf") != INVALID_FILE_ATTRIBUTES)
                candidates.push_back("C:\\Program Files\\BlueStacks_msi5\\HD-Player.exe");

            candidates.push_back("C:\\Program Files\\BlueStacks_msi5\\HD-Player.exe");
            candidates.push_back("C:\\Program Files\\BlueStacks_nxt\\HD-Player.exe");
            if (!dir.empty()) candidates.push_back(dir + "\\HD-Player.exe");
            candidates.push_back("C:\\Program Files (x86)\\BlueStacks_nxt\\HD-Player.exe");
            candidates.push_back("C:\\Program Files\\BlueStacks\\HD-Player.exe");
            candidates.push_back("C:\\Program Files (x86)\\BlueStacks\\HD-Player.exe");
            candidates.push_back("C:\\Program Files\\MSI\\MSI App Player\\HD-Player.exe");
            candidates.push_back("C:\\Program Files (x86)\\MSI\\MSI App Player\\HD-Player.exe");
            candidates.push_back("C:\\Program Files\\MSI App Player\\HD-Player.exe");
            candidates.push_back("C:\\Program Files (x86)\\MSI App Player\\HD-Player.exe");
            candidates.push_back("C:\\Program Files\\BlueStacks\\Bluestacks.exe");
            candidates.push_back("C:\\Program Files (x86)\\BlueStacks\\Bluestacks.exe");

            const std::string instanceArg = "--instance " + GetInstanceName();

            for (const auto& path : candidates) {
                if (GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES)
                    continue;

                // HD-Player.exe cần --instance <tên> mới chịu mở
                bool needsInstance = path.size() >= 13 &&
                    _stricmp(path.c_str() + path.size() - 13, "HD-Player.exe") == 0;
                const char* params = needsInstance ? instanceArg.c_str() : nullptr;

                std::cout << "[ADB] Starting emulator: " << path
                          << (needsInstance ? (" " + instanceArg) : "") << std::endl;
                HINSTANCE result = ShellExecuteA(nullptr, "open", path.c_str(), params, nullptr, SW_SHOWNORMAL);
                if ((INT_PTR)result > 32)
                    return true;
            }
            return false;
        }

        static void StartFreeFireApp(const std::string& dev) {
            ExecuteAdbRaw("-s " + dev + " shell input keyevent KEYCODE_WAKEUP");

            // Tìm activity chính xác rồi start, sau đó fallback bằng monkey
            std::string resolved = ExecuteAdbRaw("-s " + dev + " shell cmd package resolve-activity --brief com.dts.freefireth");
            size_t actor = resolved.find("com.dts.freefireth/");
            if (actor != std::string::npos) {
                size_t end = resolved.find_first_of("\r\n", actor);
                std::string activity = resolved.substr(actor, end == std::string::npos ? std::string::npos : end - actor);
                ExecuteAdbRaw("-s " + dev + " shell am start -n " + activity);
            }
            ExecuteAdbRaw("-s " + dev + " shell monkey -p com.dts.freefireth -c android.intent.category.LAUNCHER 1");
        }

        // -------------------------------------------------------
        // Mở Free Fire (và emulator nếu cần) rồi chờ game chạy
        // -------------------------------------------------------
        bool LaunchFreeFire(int timeoutMs) {
            std::cout << "[ADB] LaunchFreeFire: start (adb=" << GetAdbPath() << ")" << std::endl;
            ExecuteAdbRaw("start-server");
            Sleep(300);

            std::string device = FindAnyOnlineDevice();
            if (device.empty() && StartBlueStacksPlayer()) {
                std::cout << "[ADB] Waiting for emulator ADB..." << std::endl;
                for (int waited = 0; waited < 180000 && device.empty(); waited += 1000) {
                    Sleep(1000);
                    device = FindAnyOnlineDevice();
                }
            }

            if (device.empty()) {
                std::cerr << "[ADB] No emulator/device online." << std::endl;
                return false;
            }

            // Đảm bảo device được chọn có cài Free Fire
            if (!DeviceHasFreeFirePackage(device)) {
                for (const auto& d : ListOnlineDevices()) {
                    if (DeviceHasFreeFirePackage(d)) { device = d; break; }
                }
            }

            // Chờ emulator boot xong (nếu vừa mở)
            for (int i = 0; i < 180; ++i) {
                std::string boot = ExecuteAdbRaw("-s " + device + " shell getprop sys.boot_completed");
                if (boot.find('1') != std::string::npos) break;
                Sleep(1000);
            }

            std::string pidOut = ExecuteAdbRaw("-s " + device + " shell pidof com.dts.freefireth");
            if (pidOut.find_first_of("0123456789") != std::string::npos) {
                g_TargetDevice = device;
                std::cout << "[ADB] Free Fire already running on " << device << std::endl;
                return true;
            }

            std::cout << "[ADB] Launching com.dts.freefireth on " << device << std::endl;
            StartFreeFireApp(device);

            for (int waited = 0; waited < timeoutMs; waited += 1000) {
                Sleep(1000);
                pidOut = ExecuteAdbRaw("-s " + device + " shell pidof com.dts.freefireth");
                if (pidOut.find_first_of("0123456789") != std::string::npos) {
                    g_TargetDevice = device;
                    std::cout << "[ADB] Free Fire launched on " << device << std::endl;
                    return true;
                }
            }

            std::cerr << "[ADB] Free Fire did not start in time." << std::endl;
            return false;
        }

        // -------------------------------------------------------
        // Entry point chính — tìm base của libil2cpp.so qua ADB
        // -------------------------------------------------------
        bool InitializeADB() {
            std::cout << "[ADB] Initializing via BlueStacks ADB..." << std::endl;

            // Reset cached device khi retry
            g_TargetDevice = "";

            // FIX v2: NEVER kill HD-Adb.exe / adb.exe here.
            // TerminateProcess leaves 127.0.0.1:5555 in TIME_WAIT so the
            // next emulator instance can't rebind it -> BlueStacks moves
            // to 5556. Just ensure daemon running, keep server alive.
            ExecuteAdbRaw("start-server");
            Sleep(300);

            std::string device = GetTargetDevice();
            if (device.empty()) {
                std::cerr << "[ADB] Free Fire (com.dts.freefireth) not found on any BlueStacks device." << std::endl;
                g_LastError = "No emulator device found";
                Offsets::Il2Cpp = 0;
                return false;
            }

            // Lấy PID của Free Fire
            std::string pidOut = ExecuteAdbRaw("-s " + device + " shell pidof com.dts.freefireth");
            // Trim whitespace
            pidOut.erase(std::remove_if(pidOut.begin(), pidOut.end(), [](char c){ return c == '\r' || c == '\n' || c == ' '; }), pidOut.end());

            if (pidOut.empty() || pidOut.find_first_not_of("0123456789") != std::string::npos) {
                std::cerr << "[ADB] Failed to get PID of Free Fire." << std::endl;
                g_LastError = "Free Fire is not running";
                Offsets::Il2Cpp = 0;
                return false;
            }
            std::cout << "[ADB] Free Fire PID: " << pidOut << std::endl;

            // Tìm libil2cpp.so trong /proc/PID/maps
            std::string mapsCmd = "grep libil2cpp.so /proc/" + pidOut + "/maps";
            std::string addrStr = ExecuteShellWithSu(mapsCmd);

            if (addrStr.empty()) {
                std::cerr << "[ADB] libil2cpp.so not found in maps." << std::endl;
                g_LastError = HasRootAccess(device)
                    ? "libil2cpp.so not ready - retry"
                    : "Enable ROOT access in emulator!";
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