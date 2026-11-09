#pragma once
#include <windows.h>
#include <string>

namespace FWork {
    namespace ADB {
        bool InitializeADB();
        bool LaunchFreeFire(int timeoutMs = 60000);
        const std::string& GetLastError();
        void KillAdb();
        void DisconnectAll();
        void ResetTargetDevice();
        void ShutdownADB();
    }
}