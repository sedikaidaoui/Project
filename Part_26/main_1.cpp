// x64 injector for cshellvn.dll -> HD-Player.exe (BlueStacks 5)
// Extreme Injector v3 is often 32-bit and fails on 64-bit HD-Player with "kernel32.dll not found".

#include <Windows.h>
#include <TlHelp32.h>

#include <cstdio>
#include <string>

static DWORD FindProcessId(const wchar_t* processName) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    DWORD pid = 0;

    if (Process32FirstW(snap, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, processName) == 0) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &entry));
    }

    CloseHandle(snap);
    return pid;
}

static std::wstring GetDllPathNextToExe() {
    wchar_t exePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring path(exePath);
    const size_t slash = path.find_last_of(L"\\/");
    if (slash != std::wstring::npos)
        path.resize(slash + 1);
    path += L"cshellvn.dll";
    return path;
}

static bool InjectDll(DWORD pid, const std::wstring& dllFullPath) {
    HANDLE process = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
            PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);
    if (!process) {
        wprintf(L"[ERROR] OpenProcess failed (%lu). Run as Administrator.\n", GetLastError());
        return false;
    }

    const size_t bytes = (dllFullPath.size() + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote) {
        wprintf(L"[ERROR] VirtualAllocEx failed (%lu)\n", GetLastError());
        CloseHandle(process);
        return false;
    }

    if (!WriteProcessMemory(process, remote, dllFullPath.c_str(), bytes, nullptr)) {
        wprintf(L"[ERROR] WriteProcessMemory failed (%lu)\n", GetLastError());
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    auto loadLibraryW = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(kernel32, "LoadLibraryW"));
    if (!loadLibraryW) {
        wprintf(L"[ERROR] GetProcAddress(LoadLibraryW) failed\n");
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    HANDLE thread = CreateRemoteThread(process, nullptr, 0, loadLibraryW, remote, 0, nullptr);
    if (!thread) {
        wprintf(L"[ERROR] CreateRemoteThread failed (%lu)\n", GetLastError());
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    WaitForSingleObject(thread, 20000);
    DWORD exitCode = 0;
    GetExitCodeThread(thread, &exitCode);
    CloseHandle(thread);
    VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    CloseHandle(process);

    if (exitCode == 0) {
        wprintf(L"[ERROR] LoadLibraryW returned NULL (DLL not loaded)\n");
        return false;
    }

    wprintf(L"[OK] Injected. Remote module base: 0x%lX\n", exitCode);
    return true;
}

int wmain(int argc, wchar_t** argv) {
    wprintf(L"BurstAxis Injector (x64) — target: HD-Player.exe\n\n");

    std::wstring dllPath;
    if (argc >= 2) {
        dllPath = argv[1];
    } else {
        dllPath = GetDllPathNextToExe();
    }

    if (GetFileAttributesW(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        wprintf(L"[ERROR] DLL not found:\n  %s\n", dllPath.c_str());
        wprintf(L"Copy cshellvn.dll next to this exe or pass full path as argument.\n");
        system("pause");
        return 1;
    }

    DWORD pid = FindProcessId(L"HD-Player.exe");
    if (!pid) {
        wprintf(L"[ERROR] HD-Player.exe is not running. Start BlueStacks first.\n");
        system("pause");
        return 1;
    }

    wprintf(L"Process: HD-Player.exe (PID %lu)\n", pid);
    wprintf(L"DLL: %s\n\n", dllPath.c_str());

    if (!InjectDll(pid, dllPath)) {
        system("pause");
        return 1;
    }

    wprintf(L"\nDone. Press any key to exit.\n");
    system("pause");
    return 0;
}
