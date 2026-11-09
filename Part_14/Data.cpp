#include "Data.hpp"
#include <src/Globals.hpp>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/TMatrix.hpp>
#include <map>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <EspLines/Math/AimB.hpp>
#include <EspLines\Aimbot\Aimbot.hpp>
#include <EspLines\Aimbot\RageAimbot.hpp>
#include <EspLines\Aimbot\SilentAim.hpp>
#include <EspLines\Aimbot\SilentAimV2.hpp>
#include <EspLines\Aimbot\AimbotLogic.h>
#include <EspLines/Data/SilentAimLegit.hpp>
#include <EspLines/Data/ForceAimFeatures.hpp>
#include <EspLines/RankEsp.hpp>
#include <EspLines/Exploits/Exploit/DownPlayerInternal.hpp>
#include <EspLines/Exploits/Exploit/SilentLite.hpp>
#include <EspLines/Exploits/Exploit/Invisible.hpp>
#include <EspLines/Exploits/Exploit/MagnetLite.hpp>
#include <Psapi.h>
#include <src/adb/adb_utils.hpp>
#include <chrono>
#include <unordered_map>

static std::unordered_map<uint32_t, int> entityFailures;

// --- Fake Damage (Shuriken multi) : BuffWeaponDamageScale -------------------
static uint32_t g_fakeDameAttr    = 0;   // dernier PlayerAttributes ecrit
static bool     g_fakeDameApplied = false;

void FWork::Data::RestoreFakeDamage() {
    if (g_fakeDameApplied && g_fakeDameAttr)
        Mem.Write<float>((uintptr_t)g_fakeDameAttr + Offsets::BuffDamage, 1.0f);
    g_fakeDameAttr    = 0;
    g_fakeDameApplied = false;
}

bool FWork::Data::IsFakeDamageActive() {
    return g_fakeDameApplied;
}

namespace {

static bool ReadViewMatrix(uint32_t followCamera, Matrix4x4& outMatrix) {
    if (!followCamera) return false;

    const uint32_t camera = Mem.Read<uint32_t>(followCamera + Offsets::Camera);
    if (!camera) return false;

    const uint32_t cameraBase = Mem.Read<uint32_t>(camera + 0x8);
    if (!cameraBase) return false;

    outMatrix = Mem.Read<Matrix4x4>(cameraBase + Offsets::ViewMatrix);
    return true;
}

static bool FindTeamMateCamera(uint32_t currentGame, uint32_t activePlayer,
    uint32_t& outMainTransform, uint32_t& outFollowCamera)
{
    if (!currentGame) return false;

    const uint32_t entityDictionary = Mem.Read<uint32_t>(currentGame + Offsets::DictionaryEntities);
    if (!entityDictionary) return false;

    const uint32_t entities = Mem.Read<uint32_t>(entityDictionary + 0xC);
    if (!entities) return false;

    const uint32_t entitiesCount = Mem.Read<uint32_t>(entityDictionary + 0x10);
    if (entitiesCount < 1) return false;

    const uint32_t entityList = entities + 0x1C;

    for (uint32_t i = 0; i < entitiesCount; ++i) {
        const uint32_t entity = Mem.Read<uint32_t>(entityList + i * 0x10);
        if (!entity || entity == activePlayer) continue;

        const uint32_t avatarManager = Mem.Read<uint32_t>(entity + Offsets::AvatarManager);
        if (!avatarManager) continue;

        const uint32_t avatar = Mem.Read<uint32_t>(avatarManager + Offsets::Avatar);
        if (!avatar) continue;

        const uint32_t avatarData = Mem.Read<uint32_t>(avatar + Offsets::Avatar_Data);
        if (!avatarData) continue;

        const bool isTeam = Mem.Read<bool>(avatarData + Offsets::Avatar_Data_IsTeam);
        const bool isDead = Mem.Read<bool>(entity + Offsets::Player_IsDead);
        if (!isTeam || isDead) continue;

        const uint32_t tMain = Mem.Read<uint32_t>(entity + Offsets::MainCameraTransform);
        const uint32_t tFollow = Mem.Read<uint32_t>(entity + Offsets::FollowCamera);
        if (tMain && tFollow) {
            outMainTransform = tMain;
            outFollowCamera = tFollow;
            return true;
        }
    }

    return false;
}

static uint32_t ResolveActivePlayer(uint32_t currentMatch) {
    if (!currentMatch) return 0;

    const uint32_t matchLocal = Mem.Read<uint32_t>(currentMatch + Offsets::LocalPlayer);
    if (matchLocal) {
        uint32_t activePlayer = matchLocal;

        const uint32_t playerObserver = Mem.Read<uint32_t>(matchLocal + Offsets::CurrentObserver);
        if (playerObserver) {
            const uint32_t observed = Mem.Read<uint32_t>(playerObserver + Offsets::ObserverPlayer);
            if (observed) {
                activePlayer = observed;
            }
        }
        return activePlayer;
    }

    const uint32_t matchObserver = Mem.Read<uint32_t>(currentMatch + Offsets::m_LocalObserver);
    if (!matchObserver) return 0;

    return Mem.Read<uint32_t>(matchObserver + Offsets::m_TargetPlayer);
}

} // namespace

void FWork::Data::Refresh() {
    SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
    g_Globals.EspConfig.Entities.clear();
    entityFailures.clear();
    {
        SRWLockGuard cacheLock(&::Mem::CacheLock, true);
        ::Mem::Cache.clear();
    }
    g_Globals.EspConfig.Matrix = false;
    g_Globals.EspConfig.LocalPlayer = 0;
    g_Globals.EspConfig.previousCount = 0;
}

void FWork::Data::Work() {
        // Silent Aim V2 : demarre le worker (idempotent)
        SilentAimV2Function::SilentAimV2::Start();

        GameContext ctx;
        static int matchFailureCount = 0;

        if (Offsets::Il2Cpp == 0) {
            g_Globals.EspConfig.InMatch = false;
            return;
        }

        if (g_Globals.Visuals.RefreshEsp) {
            static auto lastRefresh = std::chrono::steady_clock::now();
            const auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRefresh).count() >= 2000) {
                Refresh();
                lastRefresh = now;
            }
        }

        ctx.currentGame = GetCurrentGame();
        ctx.currentMatch = ctx.currentGame ? GetCurrentMatch(ctx.currentGame) : 0;
        ctx.localPlayer = ctx.currentMatch ? Mem.Read<uint32_t>(ctx.currentMatch + Offsets::LocalPlayer) : 0;

        if (!SetupLocalPlayerAndCamera(ctx)) {
            if (++matchFailureCount > 30) { // Require ~240ms of failure (30 frames * 8ms delay)
                SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
                if (!g_Globals.EspConfig.Entities.empty()) {
                    g_Globals.EspConfig.Entities.clear();
                }
                entityFailures.clear();
                {
                    SRWLockGuard lock(&::Mem::CacheLock, true);
                    ::Mem::Cache.clear(); // Clear translation cache instantly to allow fresh resolving
                }
                g_Globals.EspConfig.InMatch = false;
            }
            return;
        }

        // Reset failure count on success
        matchFailureCount = 0;
        g_Globals.EspConfig.InMatch = true;
        ProcessEntities(ctx);

        // --- Silent - Lite ---
        if (g_Globals.AimBot.SilentLiteEnabled)
        {
            const uint32_t liteDict = Mem.Read<uint32_t>(ctx.currentGame + Offsets::DictionaryEntities);
            uint32_t liteEntities = liteDict ? Mem.Read<uint32_t>(liteDict + 0xC) : 0;
            const uint32_t liteCount = liteDict ? Mem.Read<uint32_t>(liteDict + 0x10) : 0;
            if (liteEntities) liteEntities += 0x1C;

            const Vector3   liteMainPos = g_Globals.EspConfig.MainCamera;
            const Matrix4x4 liteView    = g_Globals.EspConfig.ViewMatrix;
            const int       liteW       = g_Globals.EspConfig.Width;
            const int       liteH       = g_Globals.EspConfig.Height;

            Lite::SilentLiteRun(ctx.localPlayer, liteEntities, liteCount, liteMainPos, liteView, liteW, liteH);
        }

        // --- INVISIBLE / ANTI INVISIBLE (toujours appele : restauration a l'OFF) ---
        {
            const uint32_t invDict = Mem.Read<uint32_t>(ctx.currentGame + Offsets::DictionaryEntities);
            uint32_t invEntities = invDict ? Mem.Read<uint32_t>(invDict + 0xC) : 0;
            const uint32_t invCount = invDict ? Mem.Read<uint32_t>(invDict + 0x10) : 0;
            if (invEntities) invEntities += 0x1C;

            Inv::InvisibleRun(ctx.localPlayer, invEntities, invCount);
        }

        // --- MAGNET - LITE (toujours appele : restauration auto des positions a l'OFF) ---
        {
            const uint32_t mltDict = Mem.Read<uint32_t>(ctx.currentGame + Offsets::DictionaryEntities);
            uint32_t mltEntities = mltDict ? Mem.Read<uint32_t>(mltDict + 0xC) : 0;
            const uint32_t mltCount = mltDict ? Mem.Read<uint32_t>(mltDict + 0x10) : 0;
            if (mltEntities) mltEntities += 0x1C;

            const Vector3   mltMainPos = g_Globals.EspConfig.MainCamera;
            const Matrix4x4 mltView    = g_Globals.EspConfig.ViewMatrix;
            const int       mltW       = g_Globals.EspConfig.Width;
            const int       mltH       = g_Globals.EspConfig.Height;

            Lite::MagnetLiteRun(ctx.localPlayer, mltEntities, mltCount, mltMainPos, mltView, mltW, mltH);
        }

        //Aimbot Legit
        Aim::Aimbot::StartAimbot();

        //Silent Aim
        Aim::SilentAim::Start();

        if (g_Globals.AimBot.SilentAimEnabled && g_Globals.AimBot.SilentAimMode == 1) {
            SilentAimLegit::Start();
        } else {
            SilentAimLegit::Stop();
        }

        //Aimbot Rage
        Aim::RageAimbot::Aimbot();

        //Aimbot Lite
        Aim::Aimbot::AimBotVisibleSafe();

        //Down Player (threads idempotentes)
        DownPlayerInternal::Start();

        g_Globals.EspConfig.LocalPlayer = ctx.activePlayer;
        FWork::ForceAim::Update();

        // --- Memory Hacks Logic ---
        uint32_t localPlayer = ctx.localPlayer;
        if (localPlayer) {
            uint32_t weapon = Mem.Read<uint32_t>(localPlayer + Offsets::Weapon);
            
            // Weapon Hacks (No Recoil)
            if (weapon) {
                uint32_t weaponData = Mem.Read<uint32_t>(weapon + Offsets::WeaponData);
                if (weaponData && g_Globals.AimBot.NoRecoil) {
                    Mem.Write<float>(weaponData + Offsets::WeaponRecoil, 0.0f); 
                }

                // Infinite Ammo
                static int ammoTick = 0;
                if (++ammoTick % 5 == 0) {
                    if (g_Globals.Exploits.InfiniteAmmo) {
                        int currentAmmo = Mem.Read<int>(weapon + Offsets::AmmoOff);
                        if (currentAmmo < 900) Mem.Write<int>(weapon + Offsets::AmmoOff, 999);
                    }
                }
            }

            // Fast Reload + Rapid Fire (AotForms — every tick)
            uint32_t attributes = Mem.Read<uint32_t>(localPlayer + Offsets::PlayerAttributes);
            if (attributes) {
                Mem.Write<bool>(attributes + Offsets::NoReload, g_Globals.AimBot.FastReload);

                static std::unordered_map<uint32_t, float> s_originalFireIntervals;
                static bool s_lastRapidFireState = false;

                if (g_Globals.AimBot.RapidFire) {
                    float currentInterval = 0.0f;
                    if (Mem.Read<float>(attributes + Offsets::FireInterval, currentInterval)) {
                        if (s_originalFireIntervals.find(attributes) == s_originalFireIntervals.end())
                            s_originalFireIntervals[attributes] = currentInterval;

                        const float rapidFireValue = 1.1f - g_Globals.AimBot.RapidFireSpeed;
                        if (fabsf(currentInterval - rapidFireValue) > 0.001f)
                            Mem.Write<float>(attributes + Offsets::FireInterval, rapidFireValue);
                    }
                    s_lastRapidFireState = true;
                }
                else if (s_lastRapidFireState) {
                    auto saved = s_originalFireIntervals.find(attributes);
                    if (saved != s_originalFireIntervals.end())
                        Mem.Write<float>(attributes + Offsets::FireInterval, saved->second);
                    else
                        Mem.Write<float>(attributes + Offsets::FireInterval, 1.0f);
                    s_lastRapidFireState = false;
                }
            }

            // Attribute Hacks (Fast Medkit, etc.) - Update every 10 frames to save FPS
            static int attrTick = 0;
            if (++attrTick % 10 == 0) {
                attributes = Mem.Read<uint32_t>(localPlayer + Offsets::PlayerAttributes);
                if (attributes) {
                    // Fast Medkit (0x8C = FSModeUseMedikitFasterRate)
                    if (g_Globals.Exploits.FastMedkit) {
                        Mem.Write<float>(attributes + Offsets::fastmed, 0.78f);
                        g_Globals.Exploits.FastMedkitActive = true;
                    } else if (g_Globals.Exploits.FastMedkitActive) {
                        Mem.Write<float>(attributes + Offsets::fastmed, 1.0f);
                        g_Globals.Exploits.FastMedkitActive = false;
                    }

                    // Speed Car
                    if (g_Globals.Exploits.SpeedCar) {
                        Mem.Write<float>(attributes + Offsets::m_DriveSpeedScale, 5.0f);
                    }

                    // Infinity Skyler
                    static bool InfinitySkylerActive = false;
                    if (g_Globals.Exploits.InfinitySkyler) {
                        Mem.Write<float>(attributes + Offsets::InfinitySkylerOffsets, 1.0f);
                        InfinitySkylerActive = true;
                    }
                    else if (InfinitySkylerActive) {
                        Mem.Write<float>(attributes + Offsets::InfinitySkylerOffsets, 0.0f);
                        InfinitySkylerActive = false;
                    }

                    // Infinite Skill (character delay)
                    if (g_Globals.Exploits.InfinitySkill) {
                        Mem.Write<float>(attributes + Offsets::InfinitySkillOffset, 1.0f);
                    }

                }
            }
        }
    }

    bool FWork::IsInsideFOV(const ImVec2& pos) {
        int centerX = g_Globals.EspConfig.Width / 2;
        int centerY = g_Globals.EspConfig.Height / 2;
        int radius = g_Globals.AimBot.Fov;

        int dx = (int)pos.x - centerX;
        int dy = (int)pos.y - centerY;

        return (dx * dx + dy * dy) <= (radius * radius);
    }

    uint32_t FWork::Data::GetCurrentGame() {
        if (Offsets::Il2Cpp == 0) return 0;

#ifdef FFMax
        // Implementa��o para quando FFMax est� definido
        uint32_t staticGameFacade = Mem.Read<uint32_t>(Mem.Read<uint32_t>(Offsets::Il2Cpp + Offsets::InitBase) + Offsets::StaticClass);
        if (!staticGameFacade) return 0;
        return Mem.Read<uint32_t>(staticGameFacade);
#else
        // Implementa��o padr�o quando FFMax n�o est� definido
        uint32_t baseGameFacade = Mem.Read<uint32_t>(Offsets::Il2Cpp + Offsets::InitBase);
        if (!baseGameFacade) return 0;

        uint32_t gameFacade = Mem.Read<uint32_t>(baseGameFacade);
        if (!gameFacade) return 0;

        uint32_t staticGameFacade = Mem.Read<uint32_t>(gameFacade + Offsets::StaticClass);
        if (!staticGameFacade) return 0;

        return Mem.Read<uint32_t>(staticGameFacade);
#endif
    }
    uint32_t FWork::Data::GetCurrentMatch(uint32_t currentGame) {
        if (!currentGame) return 0;

        uint32_t currentMatch = Mem.Read<uint32_t>(currentGame + Offsets::CurrentMatch);
        if (!currentMatch) return 0;

        uint32_t matchStatus = Mem.Read<uint32_t>(currentMatch + Offsets::MatchStatus);
        return (matchStatus == 1) ? currentMatch : 0;
    }

    bool FWork::Data::SetupLocalPlayerAndCamera(GameContext& ctx) {
        const uint32_t currentMatch = ctx.currentMatch;
        const uint32_t matchLocal = ctx.localPlayer;

        ctx.activePlayer = ResolveActivePlayer(currentMatch);
        if (!ctx.activePlayer) {
            return false;
        }

        g_Globals.EspConfig.LocalPlayer = ctx.activePlayer;

        uint32_t activeMainTransform = Mem.Read<uint32_t>(ctx.activePlayer + Offsets::MainCameraTransform);
        uint32_t activeFollowCamera = Mem.Read<uint32_t>(ctx.activePlayer + Offsets::FollowCamera);

        if (!activeMainTransform || !activeFollowCamera) {
            uint32_t teamMain = 0;
            uint32_t teamFollow = 0;
            if (FindTeamMateCamera(ctx.currentGame, ctx.activePlayer, teamMain, teamFollow)) {
                if (!activeMainTransform) activeMainTransform = teamMain;
                if (!activeFollowCamera) activeFollowCamera = teamFollow;
            }
        }

        Vector3 mainPos = Vector3::Zero();
        if (activeMainTransform) {
            TransformUtils::GetPosition(activeMainTransform, mainPos);
        }

        Matrix4x4 viewMatrix{};
        bool matrixFound = false;
        if (activeFollowCamera) {
            matrixFound = ReadViewMatrix(activeFollowCamera, viewMatrix);
        }

        if (!matrixFound && matchLocal && matchLocal != ctx.activePlayer) {
            const uint32_t localFollow = Mem.Read<uint32_t>(matchLocal + Offsets::FollowCamera);
            matrixFound = ReadViewMatrix(localFollow, viewMatrix);
        }

        if (!matrixFound) {
            return false;
        }

        {
            SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
            if (mainPos != Vector3::Zero()) {
                g_Globals.EspConfig.MainCamera = mainPos;
            }
            g_Globals.EspConfig.Matrix = true;
            g_Globals.EspConfig.ViewMatrix = viewMatrix;
        }

        // SpeedHack & InfiniteAmmo & FastMedkit (Optimize by skipping frames)
        // Walk Speed force le passage a chaque tick : a x60 le jeu re-ecriait le
        // multiplicateur entre deux ecritures espacees de 10 ticks.
        static int setupTick = 0;
        if ((++setupTick % 10 == 0 || g_Globals.Exploits.WalkSpeed || g_Globals.Exploits.FakeDame) && matchLocal) {
            uint32_t playerAttr = 0;
            if (Mem.ReadFast2<uint32_t>(matchLocal + Offsets::PlayerAttributes, &playerAttr) && playerAttr != 0) {
                // SpeedHack / Walk Speed
                // Multiplicateur de vitesse de DEPLACEMENT du joueur local :
                //   PlayerAttributes + 0x1BC et + 0x1B8  (float, 1.0 = normal)
                // Walk Speed a la priorite (facteur reglable jusqu'a x60),
                // sinon le Speed Timer du menu garde son 1.7 historique.
                float desiredMult = 1.0f;
                if (g_Globals.Exploits.WalkSpeed) {
                    desiredMult = g_Globals.Exploits.WalkSpeedMult;
                    if (desiredMult < 1.0f)  desiredMult = 1.0f;
                    if (desiredMult > 60.0f) desiredMult = 60.0f;
                }
                else if (g_Globals.Exploits.SpeedHack) {
                    desiredMult = 1.7f;
                }

                if (desiredMult != 1.0f) {
                    Mem.Write<float>((uintptr_t)playerAttr + 0x1BC, desiredMult);
                    Mem.Write<float>((uintptr_t)playerAttr + 0x1B8, desiredMult);
                    g_Globals.Exploits.SpeedHackActive = true;
                    g_Globals.Exploits.WalkSpeedActive = g_Globals.Exploits.WalkSpeed;
                    g_Globals.Exploits.WalkSpeedApplied =
                        g_Globals.Exploits.WalkSpeed ? desiredMult : 0.0f;
                    g_Globals.Exploits.WalkSpeedAttrPtr = playerAttr;
                }
                else if (g_Globals.Exploits.SpeedHackActive) {
                    Mem.Write<float>((uintptr_t)playerAttr + 0x1BC, 1.0f);
                    Mem.Write<float>((uintptr_t)playerAttr + 0x1B8, 1.0f);
                    g_Globals.Exploits.SpeedHackActive = false;
                    g_Globals.Exploits.WalkSpeedActive = false;
                    g_Globals.Exploits.WalkSpeedApplied = 0.0f;
                    g_Globals.Exploits.WalkSpeedAttrPtr = 0;
                }

                // InfiniteAmmo
                if (g_Globals.Exploits.InfiniteAmmo) {
                    Mem.Write<bool>((uintptr_t)playerAttr + 0x88, true);
                    g_Globals.Exploits.InfiniteAmmoActive = true;
                }
                else if (g_Globals.Exploits.InfiniteAmmoActive) {
                    Mem.Write<bool>((uintptr_t)playerAttr + 0x88, false);
                    g_Globals.Exploits.InfiniteAmmoActive = false;
                }

                // Fake Damage (Shuriken multi) : BuffWeaponDamageScale
                //   PlayerAttributes + BuffDamage (0xF8, float ; 1.0 = normal)
                // ecrit a CHAQUE tick tant que coche (le jeu re-evalue le buff).
                if (g_Globals.Exploits.FakeDame) {
                    Mem.Write<float>((uintptr_t)playerAttr + Offsets::BuffDamage, 9014280192.0f);
                    g_fakeDameAttr    = playerAttr;
                    g_fakeDameApplied = true;
                }
                else if (g_fakeDameApplied) {
                    Mem.Write<float>((uintptr_t)g_fakeDameAttr + Offsets::BuffDamage, 1.0f);
                    g_fakeDameAttr    = 0;
                    g_fakeDameApplied = false;
                }
            }
        }
        else if (!matchLocal &&
                 (g_Globals.Exploits.SpeedHackActive || g_Globals.Exploits.WalkSpeedActive ||
                  g_fakeDameApplied)) {
            // Plus de joueur local (lobby / fin de match) : plus d'adresse a
            // ecrire -> on relache les etats pour que le menu ne mente pas.
            g_Globals.Exploits.SpeedHackActive = false;
            g_Globals.Exploits.WalkSpeedActive = false;
            g_Globals.Exploits.WalkSpeedApplied = 0.0f;
            g_Globals.Exploits.WalkSpeedAttrPtr = 0;
            g_fakeDameAttr    = 0;
            g_fakeDameApplied = false;
        }

        // Camera Hack
        if (setupTick % 5 == 0 && matchLocal) {
            uint32_t camHackPointer = Mem.Read<uint32_t>(matchLocal + Offsets::FollowCamera);
            if (camHackPointer != 0) {
                if (g_Globals.Exploits.CameraHack) {
                    Mem.Write<float>((uintptr_t)camHackPointer + Offsets::RightcameraOffset, 20.0f);
                    g_Globals.Exploits.CameraHackActive = true;
                }
                else if (g_Globals.Exploits.CameraHackActive) {
                    Mem.Write<float>((uintptr_t)camHackPointer + Offsets::RightcameraOffset, 1.0f);
                    g_Globals.Exploits.CameraHackActive = false;
                }

                if (g_Globals.Exploits.UnderCam) {
                    Mem.Write<float>((uintptr_t)camHackPointer + Offsets::FOVcameraoffset, g_Globals.Exploits.Distance);
                    g_Globals.Exploits.UnderCamActive = true;
                }
                else if (g_Globals.Exploits.UnderCamActive) {
                    Mem.Write<float>((uintptr_t)camHackPointer + Offsets::FOVcameraoffset, 1.0f);
                    g_Globals.Exploits.UnderCamActive = false;
                }
            }
        }

        if (setupTick % 5 == 0 && matchLocal) {
            // Aimlock Mira 2x
            uint32_t weapon = Mem.Read<uint32_t>(matchLocal + Offsets::Weapon);
            if (weapon != 0)
            {
                uint32_t attachmentData = Mem.Read<uint32_t>(weapon + 0x4FC);
                if (attachmentData != 0)
                {
                    uint32_t sightParams = 0;
                    if (Mem.ReadFast2<uint32_t>(attachmentData + 0x84, &sightParams) && sightParams != 0)
                    {
                        const float lockedValue = 1.90277084e17f;
                        const float defaultValue = 1.0f;

                        if (g_Globals.AimBot.AimLock2x)
                        {
                            if (!g_Globals.AimBot.AimLock2xActive)
                            {
                                Mem.Write<float>(sightParams + 0x5C, lockedValue);
                                g_Globals.AimBot.AimLock2xActive = true;
                            }
                        }
                        else if (g_Globals.AimBot.AimLock2xActive)
                        {
                            Mem.Write<float>(sightParams + 0x5C, defaultValue);
                            g_Globals.AimBot.AimLock2xActive = false;
                        }
                    }
                }
            }
        }

        if (setupTick % 5 == 0 && matchLocal) {
            // Aimlock
            uint32_t weapon = 0;
            if (Mem.ReadFast2<uint32_t>(matchLocal + 0x348, &weapon) && weapon != 0) {
                if (g_Globals.AimBot.AimLock) {
                    Mem.Write<float>(weapon + 0x3F0, -1.0f);
                    g_Globals.AimBot.AimLockActive = true;
                }
                else if (g_Globals.AimBot.AimLockActive) {
                    Mem.Write<float>(weapon + 0x3F0, 1.0f);
                    g_Globals.AimBot.AimLockActive = false;
                }
            }

            // Fast Switch (WeaponParams.SwitchWeaponTime)
            if (g_Globals.Exploits.FastSwitch) {
                uint32_t weapon = 0;
                if (Mem.ReadFast2<uint32_t>(matchLocal + Offsets::Weapon, &weapon) && weapon != 0) {
                    Mem.Write<float>(weapon + Offsets::WeaponParams + Offsets::QuickSwap, 0.1f);
                    Mem.Write<float>(weapon + Offsets::WeaponParams + Offsets::PreQuickSwap, 0.1f);
                    Mem.Write<float>(weapon + Offsets::WeaponParams + Offsets::PostQuickSwap, 0.1f);
                }
                g_Globals.Exploits.FastSwitchActive = true;
            }
            else if (g_Globals.Exploits.FastSwitchActive) {
                uint32_t weapon = 0;
                if (Mem.ReadFast2<uint32_t>(matchLocal + Offsets::Weapon, &weapon) && weapon != 0) {
                    Mem.Write<float>(weapon + Offsets::WeaponParams + Offsets::QuickSwap, 1.0f);
                    Mem.Write<float>(weapon + Offsets::WeaponParams + Offsets::PreQuickSwap, 1.0f);
                    Mem.Write<float>(weapon + Offsets::WeaponParams + Offsets::PostQuickSwap, 1.0f);
                }
                g_Globals.Exploits.FastSwitchActive = false;
            }

            // Parachute
            uint32_t gameVars = 0;
            if (Mem.ReadFast2<uint32_t>(matchLocal + Offsets::GameVariables, &gameVars) && gameVars != 0) {
                if (g_Globals.Exploits.Parachute) {
                    Mem.Write<float>(gameVars + 0x1a8, 0.0f);
                    Mem.Write<float>(gameVars + 0x1f4, 0.0f);
                    g_Globals.Exploits.ParachuteActive = true;
                }
                else if (g_Globals.Exploits.ParachuteActive) {
                    Mem.Write<float>(gameVars + 0x1a8, 3.0f);
                    Mem.Write<float>(gameVars + 0x1f4, 0.5f);
                    g_Globals.Exploits.ParachuteActive = false;
                }
            }
        }

        return true;
    }

    void FWork::Data::ProcessEntities(const GameContext& ctx) {
        uint32_t entityDictionary = Mem.Read<uint32_t>(ctx.currentGame + Offsets::DictionaryEntities);
        if (!entityDictionary) return;

        uint32_t entities = Mem.Read<uint32_t>(entityDictionary + 0xC);
        if (!entities) return;

        entities += 0x1C;

        uint32_t entitiesCount = Mem.Read<uint32_t>(entityDictionary + 0x10);
        if (entitiesCount != g_Globals.EspConfig.previousCount) {
            g_Globals.EspConfig.previousCount = entitiesCount;
        }

        if (entitiesCount < 1) return;

        Vector3 mainPos = g_Globals.EspConfig.MainCamera;

        const bool isInObserveMode = (ctx.activePlayer != 0) &&
            (ctx.localPlayer == 0 || ctx.activePlayer != ctx.localPlayer);
        (void)isInObserveMode;

        for (uint32_t i = 0; i < entitiesCount; ++i) {
            uint32_t entity = Mem.Read<uint32_t>(entities + i * 0x10);

            if (entity == 0 || entity == ctx.localPlayer) continue;

            try {
                Player localPlayer;

                auto ProcessEntity = [&](Player& p) -> bool {

                    uint32_t avatarManager = Mem.Read<uint32_t>(entity + Offsets::AvatarManager);
                    uint32_t avatar = avatarManager ? Mem.Read<uint32_t>(avatarManager + Offsets::Avatar) : 0;
                    uint32_t avatarData = avatar ? Mem.Read<uint32_t>(avatar + Offsets::Avatar_Data) : 0;

                    if (!avatarData) return false;

                    p.IsVisible = Mem.Read<bool>(avatar + Offsets::Avatar_IsVisible);
                    bool isWukong = Mem.Read<bool>(entity + Offsets::OreonWu);
                    p.IsWukong = isWukong;
                    bool isTeam = Mem.Read<bool>(avatarData + Offsets::Avatar_Data_IsTeam);

                    p.IsTeam = isTeam ? Player::Bool3::True : Player::Bool3::False;
                    p.IsKnown = !isTeam;

                    if (p.IsTeam == Player::Bool3::True || !p.IsKnown) {
                        return false;
                    }

                    // ESP Wukong: Bypass visibility check if isWukong is true
                    if (!p.IsVisible && !g_Globals.Visuals.ESPWukong && !isWukong && !isInObserveMode) {
                        return false;
                    }

                    uint32_t shadowBase = Mem.Read<uint32_t>(entity + Offsets::Player_ShadowBase);
                    p.IsKnocked = shadowBase ? (Mem.Read<int>(shadowBase + Offsets::XPose) == 8) : false;

                    p.IsBot = Mem.Read<bool>(entity + Offsets::IsClientBot);

                    if (uint32_t nameAddr = 0; Mem.Read(entity + Offsets::Player_Name, nameAddr) && nameAddr) {
                        p.Name = Mem.String(nameAddr + 0xC, 128);
                    }

                    if (g_Globals.Visuals.RankIcon) {
                        std::string rankName;   // le texte du rang n'est plus affiche : on ne garde que le score (icone)
                        RankEsp::ReadRankFromEntity(entity, rankName, &p.RankScore);
                    }

                    // In Training mode/some versions, empty name = BOT
                    if (p.Name.empty()) {
                        p.Name = "BOT";
                    }

                    bool nameIsBot = (p.Name.find("BOT") != std::string::npos || 
                                      p.Name.find("Bot") != std::string::npos || 
                                      p.Name.find("bot") != std::string::npos);

                    // Additional bot flag checks (some versions use 0x2ed or 0x2ee)
                    bool altBotFlag = Mem.Read<bool>(entity + 0x2ed) || Mem.Read<bool>(entity + 0x2ee);

                    if (g_Globals.AimBot.IgnoreBots && (p.IsBot || altBotFlag || nameIsBot)) {
                        return false;
                    }

                    p.IsDead = Mem.Read<bool>(entity + Offsets::Player_IsDead);

                    uint32_t dataPool = Mem.Read<uint32_t>(entity + Offsets::Player_Data);
                    if (!dataPool) return false;
                    uint32_t poolObj = Mem.Read<uint32_t>(dataPool + 0x8);
                    if (!poolObj) return false;
                    uint32_t pool = Mem.Read<uint32_t>(poolObj + 0x10);
                    if (!pool) return false;
                    uint32_t weaponptr = Mem.Read<uint32_t>(poolObj + 0x20);
                    if (!weaponptr) return false;

                    p.Health = Mem.Read<short>(pool + Offsets::Vida);
                    p.WeaponID = Mem.Read<short>(weaponptr + Offsets::Vida);

                    static const std::pair<uint32_t, Vector3 Player::*> boneOffsets[] = {
                        {Offsets::Bones::Head, &Player::Head},
                        {Offsets::Bones::Neck, &Player::Neck},
                        {Offsets::Bones::LeftShoulder, &Player::LeftShoulder},
                        {Offsets::Bones::RightShoulder, &Player::RightShoulder},
                        {Offsets::Bones::LeftElbow, &Player::LeftElbow},
                        {Offsets::Bones::RightElbow, &Player::RightElbow},
                        {Offsets::Bones::LeftWrist, &Player::LeftWrist},
                        {Offsets::Bones::RightWrist, &Player::RightWrist},
                        {Offsets::Bones::LeftHand, &Player::LeftHand},
                        {Offsets::Bones::RightHand, &Player::RightHand},
                        {Offsets::Bones::Hip, &Player::Hip},
                        {Offsets::Bones::Groin, &Player::Groin},
                        {Offsets::Bones::Root, &Player::Root},
                        {Offsets::Bones::LeftAnkle, &Player::LeftAnkle},
                        {Offsets::Bones::RightAnkle, &Player::RightAnkle},
                        {Offsets::Bones::LeftFoot, &Player::LeftFoot},
                        {Offsets::Bones::RightFoot, &Player::RightFoot}
                    };

                    for (const auto& [offset, memberPtr] : boneOffsets) {
                        if (uint32_t boneAddress = 0; Mem.Read(entity + offset, boneAddress)) {
                            TransformUtils::GetNodePosition(boneAddress, p.*memberPtr);
                        }
                    }

                    // --- FIX ESP LOGIC: Correct missing/wrong ESP positions ---
                    if (g_Globals.Visuals.FixESP || p.IsKnocked)
                    {
                        p.IsKnown = true;

                        if (p.IsTeam == Player::Bool3::Unknown)
                            p.IsTeam = Player::Bool3::False;

                        if (p.IsTeam == Player::Bool3::True)
                            return false;

                        if (p.Root != Vector3::Zero())
                        {
                            if (p.Head == Vector3::Zero() || Math::Abs(p.Head.Y - p.Root.Y) < 0.2f)
                            {
                                float hMod = p.IsKnocked ? 0.5f : 1.65f;
                                p.Head = Vector3(p.Root.X, p.Root.Y + hMod, p.Root.Z);
                            }

                            if (p.Spine == Vector3::Zero() || Math::Abs(p.Spine.Y - p.Root.Y) < 0.2f)
                            {
                                float sMod = p.IsKnocked ? 0.25f : 0.9f;
                                p.Spine = Vector3(p.Root.X, p.Root.Y + sMod, p.Root.Z);
                            }

                            if (p.Neck == Vector3::Zero() || Math::Abs(p.Neck.Y - p.Root.Y) < 0.2f)
                            {
                                if (p.Head != Vector3::Zero() && p.Spine != Vector3::Zero())
                                {
                                    p.Neck = Vector3::Lerp(p.Spine, p.Head, 0.75f);
                                }
                                else
                                {
                                    float nMod = p.IsKnocked ? 0.35f : 1.45f;
                                    p.Neck = Vector3(p.Root.X, p.Root.Y + nMod, p.Root.Z);
                                }
                            }

                            if (p.Hip == Vector3::Zero() || Math::Abs(p.Hip.Y - p.Root.Y) < 0.2f)
                            {
                                float pMod = p.IsKnocked ? 0.15f : 0.7f;
                                p.Hip = Vector3(p.Root.X, p.Root.Y + pMod, p.Root.Z);
                            }
                        }
                        else if (p.Head != Vector3::Zero())
                        {
                            float rMod = p.IsKnocked ? 0.5f : 1.65f;
                            p.Root = Vector3(p.Head.X, p.Head.Y - rMod, p.Head.Z);
                        }
                    }

                    if (p.Head != Vector3::Zero()) {
                        p.Distance = Vector3::Distance(mainPos, p.Head);
                    }

                    p.Address = entity;

                    // --- CACHE COLLIDERS FOR AIMBOT (Legit Decoupling) ---
                    p.CachedColliderAddress = entity + Offsets::ColliderINICDNFOFJB;
                    p.HeadColliderValue = 0;
                    p.ChestColliderValue = 0;

                    // 1. Get Head Collider
                    Mem.ReadFast2<uint32_t>(entity + Offsets::ColliderHECFNHJKOMN, &p.HeadColliderValue);

                    // 2. Get Chest Collider (Gender-based)
                    uint32_t fireCollidersList = 0;
                    if (Mem.ReadFast2<uint32_t>(entity + Offsets::fireColliders, &fireCollidersList) && fireCollidersList != 0) {
                        uint32_t itemsArray = 0;
                        if (Mem.ReadFast2<uint32_t>(fireCollidersList + 0x8, &itemsArray) && itemsArray != 0) {
                            bool isFemale = Mem.Read<bool>(entity + 0x7BC); // Gender offset
                            if (isFemale) {
                                Mem.ReadFast2<uint32_t>(itemsArray + 0x18, &p.ChestColliderValue); // Female Chest Index
                                if (p.ChestColliderValue == 0) Mem.ReadFast2<uint32_t>(itemsArray + 0x14, &p.ChestColliderValue);
                            } else {
                                Mem.ReadFast2<uint32_t>(itemsArray + 0x14, &p.ChestColliderValue); // Male Chest Index
                                if (p.ChestColliderValue == 0) Mem.ReadFast2<uint32_t>(itemsArray + 0x10, &p.ChestColliderValue);
                            }
                        }
                    }

                    // Fallback Strategy: Neck -> Head
                    if (p.ChestColliderValue == 0) {
                        // Use Neck offset if available, otherwise Head
                        if (!Mem.ReadFast2<uint32_t>(entity + Offsets::Bones::Neck, &p.ChestColliderValue)) {
                             p.ChestColliderValue = p.HeadColliderValue;
                        }
                    }

                    return true;
                };

                bool isValid = ProcessEntity(localPlayer);
                
                {
                    SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
                    if (isValid) {
                        entityFailures[entity] = 0;
                        auto it = g_Globals.EspConfig.Entities.find(entity);
                        if (it != g_Globals.EspConfig.Entities.end()) {
                            localPlayer.ShotsFiredAtTarget = it->second.ShotsFiredAtTarget;
                        }
                        g_Globals.EspConfig.Entities[entity] = localPlayer;
                    }
                    else {
                        entityFailures[entity]++;
                        bool shouldErase = (entityFailures[entity] >= 15);
                        
                        auto it = g_Globals.EspConfig.Entities.find(entity);
                        if (it != g_Globals.EspConfig.Entities.end() && it->second.IsDead) {
                            shouldErase = true;
                        }
                        
                        if (shouldErase) {
                            g_Globals.EspConfig.Entities.erase(entity);
                            entityFailures.erase(entity);
                        }
                    }
                }
            }
            catch (...) {
                SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
                entityFailures[entity]++;
                if (entityFailures[entity] >= 15) {
                    g_Globals.EspConfig.Entities.erase(entity);
                    entityFailures.erase(entity);
                }
                continue;
            }
    }
}
