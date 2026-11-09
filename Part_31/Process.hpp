#include <Windows.h>
#include <iostream>
#include <tlhelp32.h>

DWORD GetProcZ(const char* processName)
{
    DWORD processID = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnapshot, &pe32))
        {
            do
            {
                if (strcmp(pe32.szExeFile, processName) == 0)
                {
                    processID = pe32.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }
    return processID;
}



bool CheckProcessInstancesZ(const char* processName, int& count) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (pe32.szExeFile == processName) {
                count++;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return true;
}

bool KillProcZ(DWORD processID)
{
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processID);
    if (hProcess == NULL)
    {
        return false;
    }
    bool result = TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);
    return result;
}

bool IsProcRunZ(const char* processName, DWORD& processID)
{
    bool isRunning = false;
    processID = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (strcmp(pe32.szExeFile, processName) == 0) {
                isRunning = true;
                processID = pe32.th32ProcessID;
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);

    return isRunning;
}

bool ForgeKillProcZ(DWORD processID)
{
    HWND hwnd = NULL;
    DWORD dwPID = 0;
    do {
        hwnd = FindWindowEx(NULL, hwnd, NULL, NULL);
        GetWindowThreadProcessId(hwnd, &dwPID);
    } while (dwPID != processID && hwnd != NULL);

    if (hwnd != NULL)
    {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        return true;
    }
    return false;
}