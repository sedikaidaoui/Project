#pragma once
#include <windows.h>
#include <string>

namespace FWork {
    namespace ADB {
        bool InitializeADB();
        void KillAdb();
        // Khởi động Free Fire trên emulator (không cần game đang chạy)
        bool LaunchFreeFire();
        // RTT TCP vers l'emulator (BlueStacks) — non bloquant.
        // Retour: ms, ou -1 = ma kaynch / timeout
        int GetEmulatorPingMs();
    }
}