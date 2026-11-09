#pragma once
#include <Windows.h>
#include <EspLines/Player.h>
#include <EspLines/Math/Matrix4v4.hpp>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>

namespace Config {
    static float CornerHeight = 20.0f;

    enum class HitBox {
        Head,
        Neck,
        Chest
    };

    enum class SilentTargetingMode {
        ClosestToCrosshair = 0,
        Target360 = 1,
        LowestHealth = 2,
        ClosestToPlayer = 3
    };

    enum class Priority {
        High,
        Normal,
        Low
    };
}

class SpinLockGuard {
    std::atomic_flag& flag;
public:
    SpinLockGuard(std::atomic_flag& f) : flag(f) {
        while (flag.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }
    ~SpinLockGuard() {
        flag.clear(std::memory_order_release);
    }
};

class Globals {
public:
    struct AimBotConfig {
        bool Enabled = false;
        bool AimbotChest = false;
        bool AimLockActive = false;
        bool AimLock;
        bool AimLock2x;
        bool AimLock2xActive = false;
        bool FastFireActive = false;
        bool AimShoulder;
        bool ExtremeWhenShooting = true;
        bool ExtremeLMouse = true;
        int ExtremeAimKey = 0;
        int ExtremeDistance = 100;
        int ExtremeHitbox = 0;
        bool ExtremeUseFov = true;
        bool ExtremeIgnoreTrainingBots = false;
        bool ExtremePriorityCrosshair = true;
        bool ExtremePriorityLowHealth = false;
        bool ExtremePriorityNearest = false;
        int ExtremeToggleKeybind = 0;
        bool NoRecoil;
        bool FastReload = false;
        int FastReloadToggleKeybind = 0;
        bool FastFire;
        bool RapidFire = false;
        float RapidFireSpeed = 0.5f;
        int RapidFireToggleKeybind = 0;
        bool AimbotFFMAX;
        bool IgnoreKnocked = true;
        bool IgnoreBots;
        int DistanceAim = 50;
        int Fov = 150;
        float Fillcolor[4] = { 0.f, 0.f, 0.f, 0.2f };
        int AimbotBind = VK_LBUTTON;
        int AimbotType = 0; // 0 = Legit, 1 = Rubix, 2 = Extreme, 3 = AimLock
        int AimbotToggleKeybind = 0;
        int AimbotTypeKeybind = 0;
        bool MouseAim;
        bool MouseAimFiring = true;
        int MouseAimBind = VK_LBUTTON;
        int MouseAimToggleKeybind = 0;
        int UpdateInterval = 100;
        int ShotDelay = 100;
        int FirstShotDelay = 20;
        float Smoothing = 2.0f;
        Config::HitBox HitBox = Config::HitBox::Head;
        Vector3 TargetPos = { 0, 0, 0 };
        bool RubixEnabled = false;
        int RubixBind = 0;
        int RubixToggleKeybind = 0;
        bool SilentAimEnabled = false;
        int SilentAimBind = VK_LBUTTON;
        int SilentLegitToggleKeybind = 0;
        int SilentAimMode = 0;       // 0 = Standard, 1 = Silent Legit
        int SilentTargetingMode = 0;
        float SilentJitter = 0.05f;
        float HeadShotRate = 0.6f;
        bool AutoFire = false;
        float AutoFireRadius = 180.0f;

        // Silent mode fields used by SilentAim.cpp
        bool SilentMode = false;        // master toggle for silent mode thread
        int SilentMethod = 0;           // 0 = Body, 1 = Head, 2 = Both
        float SilentSmoothing = 1.0f;   // smoothing multiplier for silent jitter

        // Silent mode target: 0 = Standard, 1 = Silent Legit, 2 = Body (combo)
        int SilentBodyBone = 0;       // 0 = Hip, 1 = Neck (used when SilentAimMode == 2)

        // AIMLOCK
        bool AimLockToggle = false;        // master ON/OFF
        float AimLockSpeed = 5.0f;         // smooth speed
        bool AimLockFovEnable = false;     // FOV check
        int AimLockFov = 180;              // FOV
        int AimLockBone = 0;               // 0 = Head, 1 = Body, 2 = Neck
        float AimLockDelay = 50.0f;        // delay in ms
        int AimLockMaxDistance = 300;      // max distance
        bool AimLockOnlyVisible = false;   // only visible targets

        // ===== RAGE AIMBOT (EliteXzp pull method) =====
        // Master switch for the quaternion aim-rotation pull thread.
        bool Rage = false;
        // Track through walls: skips the aim-assist visibility gate (kill cover).
        bool RageWallTrack = true;
        // Slerp speed used by the pull thread (1 = instant, 20 = very smooth).
        float RageTrackSpeed = 5.0f;
        // Screen radius (pixels from the crosshair) the rage scan accepts.
        float RageFov = 600.0f;
        // 0 = Head, 1 = Neck.
        int RageBone = 0;
        // Target scoring: 0 = crosshair, 1 = lowest health, 2 = nearest.
        int RealRagePriority = 0;
    }
    AimBot;

    struct Exploits {
        bool SpeedHack;
        float Delay = 1.0f;
        float Distance = 1.0f;
        bool FastSwitch;
        bool GodMode;
        bool WallHack;
        bool HighJump;
        bool CameraHack = false;
        bool CameraHack1 = false;
        bool CameraHack2 = false;
        bool CameraHack3 = false;
        bool UnderCam;
        bool SpeedHack1;
        bool SpeedHack2;
        bool InfiniteAmmo;
        bool FastMedkit;
        bool Parachute;

        bool InfinitySkyler = false;
        bool InfinitySkill = false;
        int InfinitySkillToggleKeybind = 0;
        bool FakeDame = false;
        bool SpeedCar = false;
        bool Scale = false;

        bool GhostActive = false;
        bool SpeedHackActive = false;
        bool InfiniteAmmoActive = false;
        bool FastMedkitActive = false;
        bool CameraHackActive = false;
        bool UnderCamActive = false;
        bool HighJumpActive = false;
        bool ParachuteActive = false;
        bool FlyHackActive = false;
        float FlyHackMaxAltitude = 50.0f;
        int FlyHackIntToggleKeybind = 0;

        bool KcNoGravityFly = false;
        int KcNoGravityFlyToggleKeybind = 0;
        float KcFlyHorizontalSpeed = 6.0f;
        float KcFlyMaxHeight = 100.0f;
        bool FastSwitchActive = false;
        bool FakeLag_Tele = false;
        int FakeLag_TeleBind = 0;
        int FakeLag_TeleToggleKeybind = 0;
        bool FakeLag_Freeze = false;
        int FakeLag_FreezeBind = 0;
        int FakeLag_FreezeToggleKeybind = 0;
        bool FakeLag_Ghost = false;
        int FakeLag_GhostBind = 0;
        int FakeLag_GhostToggleKeybind = 0;

        bool SpringHack = false;
        float SpinSpeed = 5.0f;
        bool ShakeKill = false;
        int ShakeKillToggleKeybind = 0;
        float ShakeAmplitude = 0.09f;
        float ShakeSpeed = 1.4f;
        float ShakeMultiplier = 1.7f;
        float ShakeKillMaxDistance = 500.0f;
        bool UpPlayer = false;
        float UpPlayerAltitude = 0.5f;
        bool TeleportBase = false;
        float TeleportBaseDistance = 80.0f;
        bool TeleKill = false;
        float TeleKillDistance = 50.0f;
        bool MapTeleport = false;
        int SelectedTeleport = 0;

        // methodsall
        bool SpeedTimerHack = false;
        bool TeleportMarkEnabled = false;
        bool TeleportMarkFlyPrep = false;
        bool EnemyPullEnabled = false;
        float EnemyPullMaxDistance = 500.0f;
        float PullHeightOffset = 0.08f;
        int EnemyPullFov = 150;
        int EnemyPullTickMs = 2;

        bool MagnetEnemyEnabled = false;
        float MagnetEnemyFov = 200.0f;
        float MagnetEnemyMaxDistance = 250.0f;
        float MagnetEnemySpeed = 0.5f;
        int MagnetEnemyToggleKeybind = 0;

        // Pull Enemy / magnet gate used by the rage aimbot: while a grab is
        // running the pull thread waits (kMagnetV2AimbotDelayMs) before aiming.
        bool MagnetLite = false;

        bool FakeLagHelperRunning = false;
        bool SpeedTimerHackActive = false;
        int SpeedTimerHackToggleKeybind = 0;
        int TeleportMarkToggleKeybind = 0;
        int EnemyPullToggleKeybind = 0;
    }
    Exploits;

    struct Visuals
    {
        float x, y;

        float LeftKneeOffset = -0.25f;
        float RightKneeOffset = -0.35f;

        float TextSize = 12.0f;
        float LogoScale = 0.10f;
        float Thickness = 1.0f;
        int DistanceEsp = 250;
        bool Enable = true;
        bool Observe = false;
        bool Watermark = false;
        bool Enemy = false;
        bool ESPWukong = false;
        bool Lines = true;
        int EspLines = 1;
        float LinesColor[4] = { 1.f, 1.f, 1.f, 1.f };
        float KnockedColor[4] = { 180.f / 255.f, 0.f, 255.f / 255.f, 1.0f };
        bool FilledBox = false;
        float Filledboxcolor[4] = { 0.f, 0.f, 0.f, 0.4f };

        bool Box = true;
        int players_box = 2;
        float BoxColor[4] = { 1.f, 1.f, 1.f, 1.f };

        bool ESPHealthTEXT = true;
        bool HealthBar = true;
        int players_healthbar = 1;
        float texthColor[4] = { 1.f, 1.f, 1.f, 1.f };

        bool ESPWeapon = false;
        bool ESPWeaponIcon = false;
        float ESPWeaponColor[4] = { 1.f, 1.f, 1.f, 1.f };

        bool Skeleton = true;
        float SkeletonColor[4] = { 1.f, 1.f, 1.f, 1.f };

        bool Alvo = false;
        float AlvoColor[4] = { 1.f, 1.f, 1.f, 1.f };

        bool Name = false;
        float NameColor[4] = { 1.f, 1.f, 1.f, 1.f };

        bool Distance = false;
        float DistColor[4] = { 0.7f, 0.7f, 0.7f, 1.f };

        bool ESPGranada = false;
        float ESPGranadaColor[4] = { 180.f / 255.f, 0.f, 255.f / 255.f, 1.0f };

        bool RadarHack = false;

        float WatermarkColor[4] = { 180.f / 255.f, 0.f, 255.f / 255.f, 1.0f };
        float EnemyColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Trắng mặc định

        bool Debug = false;
        float HipHeightOffset = 0.7f;
        float HipWidthOffset = 0.33f;
        float HipWidthScale = 0.19f;
        float LeftHipHeightOffset = 0.85f;
        float RightHipHeightOffset = 0.98f;

        float LeftKneeHeightOffset = 0.5f;
        float RightKneeHeightOffset = 0.5f;
        float KneeWidthScale = 0.5f;
        float LeftKneeWidthOffset = 0.1f;
        float RightKneeWidthOffset = 0.1f;
        float LeftKneeFlexionOffset = 0.1f;
        float RightKneeFlexionOffset = 0.1f;

        bool ChamsTransparente = false;
        bool Celeste = false;
        bool Glow = false;
        bool Blanco = false;
        bool Frame = false;
        bool Negro = false;
        float TextureColor[4] = { 0.1176f, 0.3921f, 0.5882f, 0.6f };

        bool RefreshEsp = false;
        int RefreshEspToggleKeybind = 0;
        bool FixESP = false;
        int FixESPToggleKeybind = 0;
        bool ShowGameTimer = false;

        // Upgraded ESP variables
        bool GlowLines = true;
        float GlowRadius = 12.0f;
        float GlowFeather = 2.0f;
        float LineThickness = 1.5f;
        bool RainbowLines = false;
        bool FillColorBox = false;
        float FillColor[4] = { 1.f, 1.f, 1.f, 0.2f };
        bool FillColorBoxState = false; // back-compat placeholder
        bool ShieldBar = false;
        int players_shieldbar = 0;
        bool esparmas = true;
        float GunColor[4] = { 1.f, 1.f, 1.f, 1.f };
        bool showIcons = false;
        float SkeletonThickness = 0.5f;
        bool AimPointESP = false;
        bool Wukong = false;
        bool moco = false;
        bool Prevision = false;
        bool SnakeText = true;
        bool Radar = false;
        bool minimap = false;
        float RadarPosX = 100.0f;
        float RadarPosY = 100.0f;
        bool ShowArrow = false;
        float ArrowOffsetY = 7.0f;
        bool ShowTopLineAndArrow = false;
        bool ShowAvatar = false;
        bool Level = false;
        bool ID = false;
        bool Pose = false;
        bool HeadDot = false;
        float HeadDotSize = 2.0f;
        float HeadDotColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
        bool RGB = false;
    } Visuals;

    struct Misc
    {
        bool ShowAimbotFov;
        float AimbotFovColor[4] = { 180.f / 255.f, 0.f, 255.f / 255.f, 1.0f };
        float AccentColor[4] = { 239.f / 255.f, 68.f / 255.f, 68.f / 255.f, 1.0f };
        float SecondaryColor[4] = { 0.075f, 0.078f, 0.098f, 1.0f };
        int ThemeIndex = 0;          // 0 = Obsidian+Cyan, 1 = Emerald, 2 = Gold, 3 = Rose
        bool StatusPanelVisible = false;
        int StatusPanelAnchor = 1;   // 0=Top, 1=Right, 2=Left, 3=Bottom
        float StatusPanelX = 10.0f;
        float StatusPanelY = 10.0f;
    } Misc;

    enum class InitState {
        None,
        Login,
        InitializingMemory,
        FindingWindow,
        ReadyToScan, // Set Process screen
        ScanningModule,
        Completed,
        Failed
    };

    struct General
    {
        char Username[255] = "";
        char Password[255] = "";
        char Key[255] = "";
        char License[255] = "";
        bool ShutDown = false;
        bool Capture = false;
        Config::Priority Priority = Config::Priority::Normal;
        int Delay = 0;
        int MenuKey = VK_INSERT;

        bool ForceonAim = false;
        float ForceAimMaxDistance = 200.0f;
        float ForceAimMaxPull = 8.0f;
        float ForceAimMaxPullVertical = 1.0f;
        bool divekill = false;
        int FORCEPART = 0;
        int forcePartkeybind = 0;
        int ForceAimToggleKeybind = 0;
        int divekillBind = 0;
        int divekillToggleKeybind = 0;

        std::atomic<InitState> State{ InitState::Login };
        std::string StatusMessage = "";
        std::atomic<int> ActiveThreads{0};

        std::string AdbStatus = "Please wait for game...";
        float AdbStatusColor[4] = { 1.0f, 165.0f / 255.0f, 0.0f, 1.0f }; // Màu cam mặc định
    } General;

    struct Esp {
        std::unordered_map<long, Player> Entities;
        std::atomic_flag EntitiesLock = ATOMIC_FLAG_INIT;
        Matrix4x4 ViewMatrix{};
        Vector3 MainCamera{};
        uint32_t LocalPlayer = 0;
        uint32_t previousCount = 0;
        bool Matrix = false;
        int Width = 0;
        int Height = 0;
        bool InMatch = false;
        float screenPullRange = 10000.0f;
    } EspConfig;

};

inline Globals g_Globals;

inline bool IsFeatureKeyActive(int bind) {
    if (bind <= 0) return true;
    return (GetAsyncKeyState(bind) & 0x8000) != 0;
}

#include <fstream>
#include <Auth/BurstAxisPaths.hpp>
#include <Auth/BurstAxisFs.hpp>

inline bool SaveConfig() {
    BurstAxis::Fs::EnsureHiddenDataDirectories();
    std::ofstream out(BurstAxis::Paths::kConfigFile, std::ios::binary);
    if (!out) return false;
    out.write((char*)&g_Globals.AimBot, sizeof(g_Globals.AimBot));
    out.write((char*)&g_Globals.Exploits, sizeof(g_Globals.Exploits));
    out.write((char*)&g_Globals.Visuals, sizeof(g_Globals.Visuals));
    out.write((char*)&g_Globals.Misc, sizeof(g_Globals.Misc));
    out.close();
    BurstAxis::Fs::SetHidden(BurstAxis::Paths::kConfigFile, false);
    return true;
}

inline bool LoadConfig() {
    std::ifstream in(BurstAxis::Paths::kConfigFile, std::ios::binary);
    if (!in.is_open())
        in.open("Cshell_Config.bin", std::ios::binary);
    if (!in) return false;
    in.read((char*)&g_Globals.AimBot, sizeof(g_Globals.AimBot));
    in.read((char*)&g_Globals.Exploits, sizeof(g_Globals.Exploits));
    in.read((char*)&g_Globals.Visuals, sizeof(g_Globals.Visuals));
    in.read((char*)&g_Globals.Misc, sizeof(g_Globals.Misc));
    return true;
}
