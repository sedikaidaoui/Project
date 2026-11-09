#pragma once
#include <Windows.h>
#include <EspLines/Player.h>
#include <EspLines/Math/Matrix4v4.hpp>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>
#include <string>
#include <vector>

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
        bool Enabled;
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

        // Aimbot Lite (AimBotVisibleSafe)
        bool AimBotVisibleSafe = false;         // master toggle
        int HeadRate = 70;                      // peso de cabeza (%)
        int ChestRate = 30;                     // peso de pecho (%)
        float AimBotVisibleSafeStrength = 0.8f; // probabilidad 0..1 de aplicar tiro

        // Silent - Lite (code source "Silent - Lite", trigger = clic gauche)
        bool SilentLiteEnabled = false;

        // Silent Aim V2 (port "BrutalSilent" / BR MODE BY DEATH ENGINE)
        bool SilentAimV2 = false;
        int  SilentAimV2Bind = VK_LBUTTON;
        int  SilentAimV2ToggleKeybind = 0;
        bool SilentAimV2IgnoreKnocked = false;
        bool SilentAimV2IgnoreBots = false;
        bool SilentAimV2FullHeadshot = false;
        bool SilentAimV2KillInChest = false;
        bool SilentAimV2ShowFov = true;
        int  SilentAimV2MaxDistance = 250;
        int  SilentAimV2Fov = 400;
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
        int SpeedHackPercent = 100;    // Speed Hack timer (%)
        int SpeedHackToggleKeybind = 0; // touche on/off du Speed Hack
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

        bool BackJump = false;
        bool UndergroundEnabled = false;
        bool TelaParada = false;
        bool DownPlayer = false;
        bool EnemyDown = false;

        // Silent Aim Mem (port de SilentAimMem/Aimbot.cs)
        bool SilentMem = false;
        int SilentMemKey = VK_LBUTTON;
        int SilentMemTargetingMode = 0;   // 0 = ClosestToCrosshair, 1 = Target360, 2 = LowestHealth, 3 = ClosestToPlayer
        int SilentMemAimStyle = 0;        // 0 = Silent (bullet write), 1 = Rage (AimRotation)
        float SilentMemMaxDistance = 400.0f;
        int SilentMemFov = 150;
        int SilentMemSmooth = 5;
        bool SilentMemIgnoreKnocked = true;

        bool RapidFly = false;
        float RapidFlyHeight = 1.5f;

        // Magnet Pull / Pull Enemy (port BLAZE / PullEnemy360Cpp)
        bool PullEnemy = false;
        bool PullSilent = false;
        int PullEnemyBind = 0;
        int PullEnemyFov = 150;
        float PullEnemyDistance = 500.0f;
        int PullEnemyTickMs = 6;
        int PullEnemyPercent = 100;   // porcentaje del tirón (10-200)

        bool SpeedTimerHackActive = false;
        int EnemyPullToggleKeybind = 0;

        // INVISIBLE / ANTI INVISIBLE (port APK -> ecriture memoire)
        bool Invisible = false;
        int  InvisibleToggleKeybind = 0;
        bool AntiInvisible = false;
        int  AntiInvisibleToggleKeybind = 0;

        // Magnet - Lite (racine tiree sur le rayon de tir + restauration auto)
        bool  MagnetLite = false;
        int   MagnetLiteToggleKeybind = 0;
        float MagnetLiteFov = 150.0f;
        float MagnetLiteCooldownMs = 100.0f;

        // Push Forward : propulsion avant DOUCE (incrementale, ease-in/out)
        bool  PushForward = false;
        int   PushForwardToggleKeybind = 0;
        float PushForwardSpeed = 5.00f;    // unites/seconde (KcFly utilise 6.0)
        float PushForwardRampMs = 300.0f;  // acceleration / freinage (ms)

        // Walk Speed : multiplicateur de vitesse de DEPLACEMENT du joueur local.
        // Meme chemin memoire que le Speed Timer du menu, mais sans le facteur
        // 1.7 fige : ici le multiplicateur est reglable jusqu'a x60.
        // (PlayerAttributes + 0x1B8 / + 0x1BC, float, 1.0 = normal)
        bool  WalkSpeed = false;
        int   WalkSpeedToggleKeybind = 0;
        float WalkSpeedMult = 60.0f;       // xN : 1.0 = normal, 60.0 = x60

        // etat observe (affiche dans le menu : confirme l'ecriture reelle)
        bool  WalkSpeedActive = false;     // multiplicateur non-normal en memoire
        float WalkSpeedApplied = 0.0f;     // derniere valeur ecrite
        uint32_t WalkSpeedAttrPtr = 0;     // derniere adresse PlayerAttributes ecrite

        // Ghost Lag : plafond de ping (0 = desactive). Sert si le "ping safe"
        // ne suffit pas : au-dela de la duree, les paquets sont relaches seuls.
        int   GhostPingCapMs = 0;          // ms (750 = maximum demande)

        // Ghost Lag "PING SAFE" : les paquets plus petits ou egaux que cette
        // taille (octets IP+UDP+payload) sont envoyes IMMEDIATEMENT et jamais
        // retenus — ce sont les keep-alives / mesures de RTT, ceux qui font
        // monter l'affichage du ping. Seuls les gros paquets (mouvement) sont
        // retenus -> le ping affiche reste NORMAL pendant l'effet ghost.
        int   GhostPingPassBytes = 64;     // octets (0 = tout retenu, ping monte)

        // Fake Damage (Shuriken multi) : BuffWeaponDamageScale = x9e9 tant que
        // coche ; restaure a 1.0f a la decochee / fin de match / unload.
        int   FakeDameToggleKeybind = 0;
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
        float KnockedColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };   // KO -> ROUGE (etait mauve)
        bool FilledBox = false;
        float Filledboxcolor[4] = { 0.f, 0.f, 0.f, 0.4f };

        bool Box = true;
        int players_box = 1;
        float BoxColor[4] = { 1.f, 1.f, 1.f, 1.f };

        bool ESPHealthTEXT = true;
        bool HealthBar = true;
        int players_healthbar = 1;
        float texthColor[4] = { 1.f, 1.f, 1.f, 1.f };

        bool ESPWeapon = false;
        bool ESPWeaponIcon = false;
        float ESPWeaponColor[4] = { 1.f, 1.f, 1.f, 1.f };

        bool Skeleton = false;
        float SkeletonColor[4] = { 1.f, 1.f, 1.f, 1.f };

        bool Alvo = true;
        float AlvoColor[4] = { 0.f, 1.f, 0.f, 1.f };

        bool Name = true;
        float NameColor[4] = { 1.f, 1.f, 1.f, 1.f };

        bool Distance = false;
        float DistColor[4] = { 0.7f, 0.7f, 0.7f, 1.f };

        bool ESPGranada = false;
        float ESPGranadaColor[4] = { 180.f / 255.f, 0.f, 255.f / 255.f, 1.0f };

        bool RadarHack = false;

        float WatermarkColor[4] = { 0.953f, 0.957f, 0.965f, 1.0f };
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
        bool Wukong = true;
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
        bool RankIcon = false;              // badge de rang (PNG embarque dans la DLL)
        bool Pose = false;
        bool HeadDot = false;
        float HeadDotSize = 2.0f;
        float HeadDotColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
        bool RGB = false;
    } Visuals;

    struct Misc
    {
        bool ShowAimbotFov;
        float AimbotFovColor[4] = { 59.f / 255.f, 130.f / 255.f, 246.f / 255.f, 1.0f };
        float AccentColor[4] = { 0.953f, 0.957f, 0.965f, 1.0f };
        float SecondaryColor[4] = { 0.075f, 0.078f, 0.098f, 1.0f };
        int ThemeIndex = 0;          // 0 = Obsidian+Cyan, 1 = Emerald, 2 = Gold, 3 = Rose
        bool StatusPanelVisible = true;
        int StatusPanelAnchor = 1;   // 0=Top, 1=Right, 2=Left, 3=Bottom
        float StatusPanelX = 10.0f;
        float StatusPanelY = 10.0f;
        bool KeybindsEnabled = true; // KEYBINDS ENABLE (tab AIMKILL) : gate des raccourcis
    } Misc;

    // -------------------------------------------------------------------
    // AIMKILL : options reprises du projet APK UPDATEDOB55 (BLACK CORPS).
    // Chaque option est envoyee au lib injectee (libTERMINALX999.so) via
    // le canal binaire UDS (voir src/aimkill/AimKillLink).
    // Les options ESP / keybinds du menu APK sont liees aux champs
    // natifs 0xVoid (Visuals / Misc) plutot que dupliquees ici.
    // -------------------------------------------------------------------
    struct AimKill
    {
        // --- Groupe AIM (tab "AIM" du menu APK) ---
        bool ActivateAll = false;       // 102   -> Mode 3   (ESP master du lib)
        bool AimkillSend = false;       // 105   -> Mode 1044
        bool SilentKill = false;        // 889   -> Mode 889
        bool SendAllFov = false;        // 1050  -> Mode 1050
        bool SafeAimkill = false;       // 5661  -> Mode 5661
        bool RealAimkillV2 = false;     // 5662  -> Mode 5662
        bool AutoSwitchSend = false;    // 106   -> Mode 1047
        bool AutoSwitchNormal = false;  // 508   -> Mode 508
        bool CoverElimination = false;  // 107   -> Mode 1048
        bool AimkillNoSwitch = false;   // 110   -> Mode 1049
        bool HideDamage = false;        // 527   -> Mode 527
        bool CoverPull = false;         // 509   -> Mode 509
        bool AimLock = false;           // 34834 -> Mode 34834
        bool TargetListOverlay = false; // 526   -> overlay menu APK (sans effet 0xVoid)
        bool TargetLock = false;        // 523   -> Mode 523
        int  AimFov = 1000;             // 104   -> Mode 104 (0..1000, defaut 1000)

        // --- Groupe BRTL (tab "BRTL" du menu APK) ---
        // ESP LINE/BOX/HEALTH/NAME/WEAPON/DISTANCE/DRAW FOV -> champs Visuals/VisBot natifs
        bool InvinsHack = false;        // 532   -> Mode 532
        bool AutofireAllGuns = false;   // 521   -> Mode 521
        bool DownKill = false;          // 5658  -> Mode 504
        bool DiveKillNew = false;       // 5660  -> Mode 5660
        bool BrustFire = false;         // 515   -> Mode 515
        bool RapidFire = false;         // 518   -> Mode 518
        bool FastReload = false;        // 519   -> Mode 519
        bool FlyX40 = false;            // 5659  -> Mode 5659
        bool StableFly = false;         // 5002  -> Mode 5002
        bool TeleportMark = false;      // 56578 -> Mode 56578
        bool SendPosition = false;      // 5663  -> Mode 5663

        // --- Groupe KEYBIND (tab "KEYBIND" du menu APK) ---
        // KEYBINDS ENABLE   -> g_Globals.Misc.KeybindsEnabled
        // SHOW KEYBIND STAT -> g_Globals.Misc.StatusPanelVisible
        // CLEAR ALL KEYBIND -> bouton (reset des raccourcis)
        // SHOW INFO HUD     -> g_Globals.Visuals.Watermark
        bool FlyhackMini = false;       // 111   -> Mode 5657
        bool HighFps = false;           // 529   -> Mode 529
    }
    AimKill;

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

// Config is stored next to the DLL. No hidden directories under C:\Users\Public.
inline std::string GetConfigPath() {
    char path[MAX_PATH]{};
    HMODULE hModule = nullptr;
    if (GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&GetConfigPath, &hModule) && GetModuleFileNameA(hModule, path, MAX_PATH)) {
        std::string fullPath(path);
        const size_t slash = fullPath.find_last_of("\\/");
        if (slash != std::string::npos)
            return fullPath.substr(0, slash + 1) + "Cshell_Config.bin";
    }
    return "Cshell_Config.bin";
}

inline bool SaveConfig() {
    std::ofstream out(GetConfigPath(), std::ios::binary);
    if (!out) return false;
    out.write((char*)&g_Globals.AimBot, sizeof(g_Globals.AimBot));
    out.write((char*)&g_Globals.Exploits, sizeof(g_Globals.Exploits));
    out.write((char*)&g_Globals.Visuals, sizeof(g_Globals.Visuals));
    out.write((char*)&g_Globals.Misc, sizeof(g_Globals.Misc));
    out.write((char*)&g_Globals.AimKill, sizeof(g_Globals.AimKill));
    out.close();
    return true;
}

inline bool LoadConfig() {
    const std::streamoff expected =
        (std::streamoff)sizeof(g_Globals.AimBot) + (std::streamoff)sizeof(g_Globals.Exploits) +
        (std::streamoff)sizeof(g_Globals.Visuals) + (std::streamoff)sizeof(g_Globals.Misc) +
        (std::streamoff)sizeof(g_Globals.AimKill);

    std::ifstream in(GetConfigPath(), std::ios::binary | std::ios::ate);
    if (!in) return false;

    // Garde : un fichier ecrit par une autre version du projet (taille differente)
    // decalerait tous les blocs suivants -> on refuse plutot que de charger n'importe quoi.
    if (in.tellg() != expected) { in.close(); return false; }

    in.seekg(0, std::ios::beg);
    in.read((char*)&g_Globals.AimBot, sizeof(g_Globals.AimBot));
    in.read((char*)&g_Globals.Exploits, sizeof(g_Globals.Exploits));
    in.read((char*)&g_Globals.Visuals, sizeof(g_Globals.Visuals));
    in.read((char*)&g_Globals.Misc, sizeof(g_Globals.Misc));
    in.read((char*)&g_Globals.AimKill, sizeof(g_Globals.AimKill));
    in.close();

    // --- Features supprimees du menu : on les force OFF apres chargement ---
    // (un ancien config pourrait les reactiver alors qu'il n'y a plus de checkbox)
    g_Globals.AimBot.Enabled              = false;
    g_Globals.AimBot.AimbotToggleKeybind  = 0;
    g_Globals.AimBot.AimbotTypeKeybind    = 0;
    g_Globals.AimBot.RapidFire            = false;
    g_Globals.AimBot.RapidFireToggleKeybind = 0;
    g_Globals.AimBot.SilentAimEnabled     = false;
    g_Globals.AimBot.SilentLegitToggleKeybind = 0;
    g_Globals.AimBot.SilentLiteEnabled    = false;

    g_Globals.Exploits.FlyHackActive              = false;
    g_Globals.Exploits.FlyHackIntToggleKeybind    = 0;
    g_Globals.Exploits.KcNoGravityFly             = false;
    g_Globals.Exploits.KcNoGravityFlyToggleKeybind = 0;
    g_Globals.Exploits.ShakeKill                  = false;
    g_Globals.Exploits.ShakeKillToggleKeybind     = 0;
    g_Globals.Exploits.SpringHack                 = false;
    g_Globals.Exploits.UpPlayer                   = false;
    g_Globals.Exploits.TeleKill                   = false;
    g_Globals.Exploits.UndergroundEnabled         = false;
    g_Globals.Exploits.PullEnemy                  = false;
    g_Globals.Exploits.PullEnemyBind              = 0;
    g_Globals.Exploits.TelaParada                 = false;
    g_Globals.Exploits.DownPlayer                 = false;
    g_Globals.Exploits.EnemyDown                  = false;
    g_Globals.Exploits.RapidFly                   = false;
    g_Globals.Exploits.InfinitySkyler             = false;
    g_Globals.Exploits.MagnetEnemyEnabled         = false;
    g_Globals.Exploits.MagnetEnemyToggleKeybind   = 0;
    g_Globals.Exploits.MagnetLite                 = false;
    g_Globals.Exploits.MagnetLiteToggleKeybind    = 0;
    g_Globals.Exploits.Invisible                  = false;
    g_Globals.Exploits.InvisibleToggleKeybind     = 0;
    g_Globals.Exploits.AntiInvisible              = false;
    g_Globals.Exploits.AntiInvisibleToggleKeybind = 0;
    g_Globals.Exploits.SilentMem                  = false;
    g_Globals.Exploits.SilentMemKey               = VK_LBUTTON;

    // --- Etats RUNTIME : n'arrivent jamais du fichier -----------------------
    // WalkSpeedAttrPtr est une adresse memoire d'une session precedente :
    // la recharger ecrirait 1.0 a l'adresse equivaut du joueur.
    g_Globals.Exploits.WalkSpeedActive            = false;
    g_Globals.Exploits.WalkSpeedApplied           = 0.0f;
    g_Globals.Exploits.WalkSpeedAttrPtr           = 0;
    g_Globals.Exploits.SpeedHackActive            = false;
    g_Globals.Exploits.SpeedTimerHackActive       = false;

    return true;
}
