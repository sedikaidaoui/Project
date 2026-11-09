#pragma once
#include <Windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <Psapi.h>
#include "Process.hpp"

std::string GetExeDirectoryZ() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);

    std::string fullPath(path);
    size_t lastSlashIndex = fullPath.find_last_of("\\/");
    if (lastSlashIndex != std::string::npos) {
        return fullPath.substr(0, lastSlashIndex);
    }
    return "";
}

std::string GetExeDirectoryPIDZ(DWORD processID) {
    char path[MAX_PATH];
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);

    if (hProcess) {
        if (GetModuleFileNameExA(hProcess, NULL, path, MAX_PATH)) {
            std::string fullPath(path);
            size_t lastSlashIndex = fullPath.find_last_of("\\/");
            CloseHandle(hProcess);
            if (lastSlashIndex != std::string::npos) {
                return fullPath.substr(0, lastSlashIndex);
            }
        }
        CloseHandle(hProcess);
    }

    return "";
}

bool CambiarDirectoryZ(const std::string& path) {
    return SetCurrentDirectoryA(path.c_str());
}

void KillAdbZ() {
    DWORD ProcIdAdb = GetProcZ("adb.exe");
    DWORD ProcIdAdb2 = 0;
    KillProcZ(ProcIdAdb);
    if (IsProcRunZ("adb.exe", ProcIdAdb2)) {
        ForgeKillProcZ(ProcIdAdb2);
    }
    DWORD ProcIdAdbHD = GetProcZ("HD-Adb.exe");
    DWORD ProcIdAdbHD2 = 0;
    KillProcZ(ProcIdAdbHD);
    if (IsProcRunZ("HD-Adb.exe", ProcIdAdbHD2)) {
        ForgeKillProcZ(ProcIdAdbHD2);
    }
}

std::string ExtraerLibAddressZ(const std::string& input) {
    std::size_t pos = input.find('-');
    if (pos != std::string::npos && pos >= 8) {
        return input.substr(pos - 8, 8);
    }
    return "";
}

inline uintptr_t uIntExtrZ(std::string c, int base = 16) {
    static_assert(sizeof(uintptr_t) == sizeof(unsigned long)
        || sizeof(uintptr_t) == sizeof(unsigned long long),
        "Please add string to handle conversion for this architecture.");
    if (sizeof(uintptr_t) == sizeof(unsigned long)) {
        return strtoul(c.c_str(), nullptr, base);
    }
    return strtoull(c.c_str(), nullptr, base);
}

std::string ShellGetAddressNoSuZ(const std::string& comand) {
    HANDLE hStdOutRead, hStdOutWrite;
    HANDLE hStdInRead, hStdInWrite;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
        std::cerr << "CreatePipe failed: " << GetLastError() << std::endl;
        return 0;
    }
    if (!SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0)) {
        std::cerr << "SetHandleInformation failed: " << GetLastError() << std::endl;
        return 0;
    }

    if (!CreatePipe(&hStdInRead, &hStdInWrite, &sa, 0)) {
        std::cerr << "CreatePipe failed: " << GetLastError() << std::endl;
        return 0;
    }
    if (!SetHandleInformation(hStdInWrite, HANDLE_FLAG_INHERIT, 0)) {
        std::cerr << "SetHandleInformation failed: " << GetLastError() << std::endl;
        return 0;
    }

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = hStdOutWrite;
    si.hStdOutput = hStdOutWrite;
    si.hStdInput = hStdInRead;
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessA(NULL, (LPSTR)".\\HD-Adb shell \"getprop ro.secure ; /boot/android/android/system/xbin/bstk/su\"", NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        std::cerr << "CreateProcess failed: " << GetLastError() << std::endl;
        return 0;
    }

    CloseHandle(hStdOutWrite);
    CloseHandle(hStdInRead);

    DWORD written;
    std::string commands = comand + "\n";
    if (!WriteFile(hStdInWrite, commands.c_str(), commands.length(), &written, NULL)) {
        std::cerr << "WriteFile failed: " << GetLastError() << std::endl;
        return 0;
    }
    CloseHandle(hStdInWrite);

    CHAR buffer[128];
    DWORD read;
    std::string output;
    while (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &read, NULL) && read > 0) {
        buffer[read] = '\0';
        output += buffer;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hStdOutRead);

    return ExtraerLibAddressZ(output);
}

void ComdADBZ(const std::string& command) {
    std::string fullCommand = ".\\HD-Adb shell " + command;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));
    if (!CreateProcessA(NULL, const_cast<LPSTR>(fullCommand.c_str()), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        std::cerr << "Failed to start process. Error: " << GetLastError() << std::endl;
        return;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}
