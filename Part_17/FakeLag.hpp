#pragma once
#include <windows.h>
#include <string>
#include <thread>
#include <chrono>

namespace Features {
    class FakeLag {
    public:
        static void Initialize();
        static void Update();
        static bool IsHelperRunning();
        static bool EnsureHelper();

        static void ToggleTele();
        static void ToggleFreeze();
        static void ToggleGhost();
    };
}
