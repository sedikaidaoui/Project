#include "FakeLag.hpp"
#include <Auth/BurstAxisPaths.hpp>
#include <Auth/BurstAxisFs.hpp>
#include <src/Globals.hpp>
#include <fstream>
#include <string>
#include <windows.h>
#include <imgui.h>
#include <tlhelp32.h>
#include <chrono>

#pragma comment(lib, "Shlwapi.lib")

namespace Features {

    void WriteState();
    bool IsProcessRunning(const std::string& processName);

    namespace {
        using namespace BurstAxis::Paths;

        bool FileExistsA(const std::string& path) {
            return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
        }

        bool IsPeExecutable(const std::string& path) {
            HANDLE file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
            if (file == INVALID_HANDLE_VALUE)
                return false;

            WORD mz = 0;
            DWORD read = 0;
            const bool ok = ReadFile(file, &mz, sizeof(mz), &read, NULL) && read == sizeof(mz) && mz == 0x5A4D;
            CloseHandle(file);
            return ok;
        }

        // PyInstaller one-file = single huge exe, no python DLL beside it -> VCRUNTIME extract errors.
        bool IsHealthyOnedirDeployment(const std::string& dir) {
            const std::string exe = dir + "\\" + kNetworkExeName;
            if (!FileExistsA(exe) || !IsPeExecutable(exe))
                return false;

            if (FileExistsA(dir + "\\python3.dll") ||
                FileExistsA(dir + "\\python314.dll") ||
                FileExistsA(dir + "\\python312.dll") ||
                FileExistsA(dir + "\\base_library.zip") ||
                FileExistsA(dir + "\\_internal\\" + kNetworkExeName)) {
                return true;
            }

            WIN32_FIND_DATAA fd{};
            HANDLE h = FindFirstFileA((dir + "\\*.dll").c_str(), &fd);
            if (h == INVALID_HANDLE_VALUE)
                return false;

            int dllCount = 0;
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                    dllCount++;
            } while (FindNextFileA(h, &fd) && dllCount < 3);
            FindClose(h);
            return dllCount >= 2;
        }

        void RemoveLegacyOneFileHelper() {
            if (FileExistsA(kLegacyOneFileExe))
                DeleteFileA(kLegacyOneFileExe);
            const std::string legacyDllsExe = std::string(kLegacyPublicBase) + "\\" + kNetworkExeName;
            if (FileExistsA(legacyDllsExe))
                DeleteFileA(legacyDllsExe.c_str());
        }

        void RemoveBrokenNetworkDeployment() {
            const std::string exe = std::string(kNetworkDir) + "\\" + kNetworkExeName;
            if (!FileExistsA(exe))
                return;
            if (IsHealthyOnedirDeployment(kNetworkDir))
                return;

            WIN32_FIND_DATAA fd{};
            HANDLE h = FindFirstFileA((std::string(kNetworkDir) + "\\*").c_str(), &fd);
            if (h == INVALID_HANDLE_VALUE)
                return;

            do {
                if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
                    continue;
                std::string path = std::string(kNetworkDir) + "\\" + fd.cFileName;
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    continue;
                DeleteFileA(path.c_str());
            } while (FindNextFileA(h, &fd));
            FindClose(h);
        }

        bool CopyDirectoryRecursive(const std::string& from, const std::string& to) {
            if (!FileExistsA(from))
                return false;

            BurstAxis::Fs::CreateHiddenDirectory(to.c_str());

            std::string search = from + "\\*";
            WIN32_FIND_DATAA fd{};
            HANDLE h = FindFirstFileA(search.c_str(), &fd);
            if (h == INVALID_HANDLE_VALUE)
                return false;

            bool ok = true;
            do {
                if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
                    continue;

                std::string srcPath = from + "\\" + fd.cFileName;
                std::string dstPath = to + "\\" + fd.cFileName;

                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (!CopyDirectoryRecursive(srcPath, dstPath))
                        ok = false;
                } else if (!CopyFileA(srcPath.c_str(), dstPath.c_str(), FALSE)) {
                    ok = false;
                }
            } while (FindNextFileA(h, &fd));

            FindClose(h);
            return ok;
        }

        void MigrateLegacyBurstaxisPaths() {
            BurstAxis::Fs::EnsureHiddenDataDirectories();

            if (!IsHealthyOnedirDeployment(kNetworkDir) && IsHealthyOnedirDeployment(kLegacyNetworkDir))
                CopyDirectoryRecursive(kLegacyNetworkDir, kNetworkDir);

            if (!IsHealthyOnedirDeployment(kDllsNetworkDir) && IsHealthyOnedirDeployment(kNetworkDir))
                CopyDirectoryRecursive(kNetworkDir, kDllsNetworkDir);

            if (!FileExistsA(kStateFile) && FileExistsA(kLegacyStateFile))
                CopyFileA(kLegacyStateFile, kStateFile, FALSE);

            if (!FileExistsA(kKeyFile) && FileExistsA(kLegacyKeyFile))
                CopyFileA(kLegacyKeyFile, kKeyFile, FALSE);

            RemoveLegacyOneFileHelper();
            BurstAxis::Fs::SecureBurstaxisLayout();
        }

        void EnsureVcRuntimeInDirectory(const std::string& directory) {
            if (directory.empty())
                return;

            BurstAxis::Fs::CreateHiddenDirectory(directory.c_str());

            const char* runtimeDlls[] = {
                "vcruntime140.dll",
                "vcruntime140_1.dll",
                "msvcp140.dll",
                "concrt140.dll"
            };

            char systemDir[MAX_PATH]{};
            if (!GetSystemDirectoryA(systemDir, MAX_PATH))
                return;

            for (const char* dll : runtimeDlls) {
                std::string target = directory + "\\" + dll;
                if (FileExistsA(target))
                    continue;

                std::string source = std::string(systemDir) + "\\" + dll;
                if (!FileExistsA(source))
                    continue;

                CopyFileA(source.c_str(), target.c_str(), FALSE);
            }
        }

        std::string GetDllDirectory() {
            char path[MAX_PATH]{};
            HMODULE hModule = NULL;
            GetModuleHandleExA(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCSTR)&GetDllDirectory,
                &hModule);
            if (!hModule || !GetModuleFileNameA(hModule, path, MAX_PATH))
                return {};

            std::string fullPath(path);
            const size_t slash = fullPath.find_last_of("\\/");
            if (slash == std::string::npos)
                return fullPath;
            return fullPath.substr(0, slash);
        }

        static bool DeployNetworkHelperFromPath(const std::string& srcDir) {
            if (!FileExistsA(srcDir + "\\" + kNetworkExeName))
                return false;

            BurstAxis::Fs::CreateHiddenDirectory(kPublicBase);
            BurstAxis::Fs::CreateHiddenDirectory(kNetworkDir);
            const bool ok = CopyDirectoryRecursive(srcDir, kNetworkDir) &&
                IsHealthyOnedirDeployment(kNetworkDir);
            if (ok)
                BurstAxis::Fs::HideDirectoryTree(kNetworkDir);
            return ok;
        }

        bool DeployNetworkHelperFromDllFolder() {
            MigrateLegacyBurstaxisPaths();

            if (IsHealthyOnedirDeployment(kNetworkDir))
                return true;

            RemoveBrokenNetworkDeployment();

            const char* sourceDirs[] = {
                kDllsNetworkDir,
                nullptr,
                nullptr,
                nullptr
            };

            const std::string dllDir = GetDllDirectory();
            std::string dllNetworkDir;
            std::string parentNetworkDir;
            std::string loaderNetworkDir;
            if (!dllDir.empty()) {
                dllNetworkDir = dllDir + "\\network";
                parentNetworkDir = dllDir + "\\..\\network";
                loaderNetworkDir = dllDir + "\\..\\AIMVISIBEL LOADER ONLINE\\ShayanUidBypass\\network";
                sourceDirs[1] = dllNetworkDir.c_str();
                sourceDirs[2] = parentNetworkDir.c_str();
                sourceDirs[3] = loaderNetworkDir.c_str();
            }

            for (const char* srcDir : sourceDirs) {
                if (!srcDir)
                    continue;
                if (DeployNetworkHelperFromPath(srcDir))
                    return true;
            }

            return false;
        }

        bool LaunchNetworkHelper();

        bool EnsureNetworkHelperRunning() {
            WriteState();

            if (IsProcessRunning(kNetworkExeName)) {
                g_Globals.Exploits.FakeLagHelperRunning = true;
                return true;
            }

            RemoveLegacyOneFileHelper();
            RemoveBrokenNetworkDeployment();

            if (!DeployNetworkHelperFromDllFolder()) {
                g_Globals.Exploits.FakeLagHelperRunning = false;
                return false;
            }

            if (!LaunchNetworkHelper()) {
                g_Globals.Exploits.FakeLagHelperRunning = IsProcessRunning(kNetworkExeName);
                return g_Globals.Exploits.FakeLagHelperRunning;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(800));
            g_Globals.Exploits.FakeLagHelperRunning = IsProcessRunning(kNetworkExeName);
            BurstAxis::Fs::SecureBurstaxisLayout();
            return g_Globals.Exploits.FakeLagHelperRunning;
        }

        bool LaunchNetworkHelper() {
            if (!IsHealthyOnedirDeployment(kNetworkDir))
                return false;

            const std::string exePath = std::string(kNetworkDir) + "\\" + kNetworkExeName;

            EnsureVcRuntimeInDirectory(kNetworkDir);

            SHELLEXECUTEINFOA sei{};
            sei.cbSize = sizeof(sei);
            sei.fMask = SEE_MASK_FLAG_NO_UI;
            sei.lpVerb = "runas";
            sei.lpFile = exePath.c_str();
            sei.lpDirectory = kNetworkDir;
            sei.nShow = SW_HIDE;
            return ShellExecuteExA(&sei) != FALSE;
        }
    }

    bool IsProcessRunning(const std::string& processName) {
        std::wstring wProcessName(processName.begin(), processName.end());
        bool exists = false;
        PROCESSENTRY32W entry;
        entry.dwSize = sizeof(PROCESSENTRY32W);

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
        if (snapshot == INVALID_HANDLE_VALUE)
            return false;

        if (Process32FirstW(snapshot, &entry)) {
            do {
                if (_wcsicmp(entry.szExeFile, wProcessName.c_str()) == 0) {
                    exists = true;
                    break;
                }
            } while (Process32NextW(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return exists;
    }

    void WriteState() {
        BurstAxis::Fs::EnsureHiddenDataDirectories();
        const std::string state =
            std::string(g_Globals.Exploits.FakeLag_Tele ? "1" : "0") + "\n" +
            std::string(g_Globals.Exploits.FakeLag_Freeze ? "1" : "0") + "\n" +
            std::string(g_Globals.Exploits.FakeLag_Ghost ? "1" : "0") + "\n";

        auto writeTo = [](const char* path, const std::string& data) {
            std::ofstream outFile(path, std::ios::trunc);
            if (outFile.is_open()) {
                outFile << data;
                outFile.close();
            }
        };

        writeTo(BurstAxis::Paths::kStateFile, state);
        BurstAxis::Fs::CreateHiddenDirectory(BurstAxis::Paths::kLegacyPublicBase);
        writeTo(BurstAxis::Paths::kLegacyStateFile, state);
        BurstAxis::Fs::HideSensitiveFiles();
    }

    void FakeLag::Initialize() {
        BurstAxis::Fs::SecureBurstaxisLayout();
        EnsureNetworkHelperRunning();
    }

    bool FakeLag::IsHelperRunning() {
        g_Globals.Exploits.FakeLagHelperRunning = IsProcessRunning(kNetworkExeName);
        return g_Globals.Exploits.FakeLagHelperRunning;
    }

    bool FakeLag::EnsureHelper() {
        return EnsureNetworkHelperRunning();
    }

    void FakeLag::ToggleTele() {
        g_Globals.Exploits.FakeLag_Tele = !g_Globals.Exploits.FakeLag_Tele;
        bool state = g_Globals.Exploits.FakeLag_Tele;
        std::thread([state]() {
            if (state) {
                EnsureNetworkHelperRunning();
                Beep(600, 120);
            } else {
                Beep(400, 150);
            }
            WriteState();
        }).detach();
    }

    void FakeLag::ToggleFreeze() {
        g_Globals.Exploits.FakeLag_Freeze = !g_Globals.Exploits.FakeLag_Freeze;
        bool state = g_Globals.Exploits.FakeLag_Freeze;
        std::thread([state]() {
            if (state) {
                EnsureNetworkHelperRunning();
                Beep(600, 120);
            } else {
                Beep(400, 150);
            }
            WriteState();
        }).detach();
    }

    void FakeLag::ToggleGhost() {
        g_Globals.Exploits.FakeLag_Ghost = !g_Globals.Exploits.FakeLag_Ghost;
        bool state = g_Globals.Exploits.FakeLag_Ghost;
        std::thread([state]() {
            if (state) {
                EnsureNetworkHelperRunning();
                Beep(600, 120);
            } else {
                Beep(400, 150);
            }
            WriteState();
        }).detach();
    }

    void FakeLag::Update() {
        static bool last_tele = false;
        static bool last_freeze = false;
        static bool last_ghost = false;
        static auto lastRetry = std::chrono::steady_clock::now();

        g_Globals.Exploits.FakeLagHelperRunning = IsProcessRunning(kNetworkExeName);

        const bool anyLag = g_Globals.Exploits.FakeLag_Tele ||
            g_Globals.Exploits.FakeLag_Freeze ||
            g_Globals.Exploits.FakeLag_Ghost;

        if (anyLag && !g_Globals.Exploits.FakeLagHelperRunning) {
            const auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastRetry).count() >= 3) {
                lastRetry = now;
                std::thread([]() { EnsureNetworkHelperRunning(); }).detach();
            }
        }

        if (last_tele != g_Globals.Exploits.FakeLag_Tele ||
            last_freeze != g_Globals.Exploits.FakeLag_Freeze ||
            last_ghost != g_Globals.Exploits.FakeLag_Ghost) {

            if (anyLag)
                std::thread([]() { EnsureNetworkHelperRunning(); }).detach();
            else
                std::thread([]() { WriteState(); }).detach();

            last_tele = g_Globals.Exploits.FakeLag_Tele;
            last_freeze = g_Globals.Exploits.FakeLag_Freeze;
            last_ghost = g_Globals.Exploits.FakeLag_Ghost;
        }
    }
}
