#pragma once
#include <cstdint>

enum class GameVariant {
    Normal = 0,
    Max = 1
};

inline GameVariant g_ActiveGameVariant = GameVariant::Normal;

class Offsets {
public:
    static inline uintptr_t Il2Cpp = 0x0;
    static inline uintptr_t UnityCpp = 0x0;

    static inline uintptr_t InitBase = 0xA342EFC;

    static inline uintptr_t StaticClass = 0x5C;
    static inline uintptr_t CurrentMatch = 0x50;
    static inline uintptr_t CurrentMatchGame = 0x4;
    static inline uintptr_t MatchStatus = 0x8C;
    static inline uintptr_t LocalPlayer = 0x94;
    static inline uintptr_t DictionaryEntities = 0x68;
    static inline uintptr_t GameTimer = 0x10;
    static inline uintptr_t FixedDeltaTime = 0x24;

    static inline uintptr_t Player_IsDead = 0x50;
    static inline uintptr_t Player_Name = 0x31C;
    static inline uintptr_t Player_Data = 0x48;
    static inline uintptr_t IsClientBot = 0x324;
    static inline uintptr_t TeamModeID = 0x294;
    static inline uintptr_t Player_ShadowBase = 0x1A60;
    static inline uintptr_t XPose = 0x78;
    static inline uintptr_t PlayerID = 0x268;

    static inline uintptr_t Player_Profile = 0x18CC;
    static inline uintptr_t Player_LevelPtr = 0x18CC;
    static inline uintptr_t Profile_Level = 0x14;
    static inline uintptr_t Profile_Rank = 0x58;
    static inline uintptr_t Profile_RankPoint = 0x5C;

    static inline uintptr_t AvatarManager = 0x504;
    static inline uintptr_t Avatar = 0xA8;
    static inline uintptr_t Avatar_IsVisible = 0x95;
    static inline uintptr_t Avatar_Data = 0x14;
    static inline uintptr_t Avatar_Data_IsTeam = 0x59;

    static inline uintptr_t MainCameraTransform = 0x28C;
    static inline uintptr_t FollowCamera = 0x494;
    static inline uintptr_t Camera_IntPtr = 0x8;


    static inline uintptr_t LockedAimingCollider = 0x54;
    static inline uintptr_t Collider = 0x4E8;
    static inline uintptr_t AimbotVisible = Collider;

    static inline uintptr_t Camera = 0x18;
    static inline uintptr_t AimRotation = 0x440;
    static inline uintptr_t FollowCamera_m_RightOffset = 0x64;
    static inline uintptr_t FollowCamera_m_UpOffset = 0x68;
    static inline uintptr_t FollowCamera_m_VisionSpeed = 0x48;

    static inline uintptr_t CurrentObserver = 0xB4;
    static inline uintptr_t ObserverPlayer = 0x28;

    static inline uintptr_t Weapon = 0x434;
    static inline uintptr_t WeaponData = 0x58;
    static inline uintptr_t Weapon_ItemId = 0x494;
    static inline uintptr_t WeaponSkinMap = 0x18D8;
    static inline uintptr_t WeaponRecoil = 0xC;
    static inline uintptr_t UnkPlayerWeaponInfoClass = 0x4A8;
    static inline uintptr_t IsCombineWeapon = 0xD8;
    static inline uintptr_t WeaponOnHand = 0x54;
    static inline uintptr_t CombineWeaponOnHand = 0x58;
    static inline uintptr_t Weapon_Info = 0x64;
    static inline uintptr_t WeaponID = 0x14;
    static inline uintptr_t Weapon_FireDelay = 0x10;
    static inline uintptr_t Weapon_IsSighting = 0x5CC;

    static inline uintptr_t sAim1 = 0x58C;
    static inline uintptr_t sAim2 = 0x9D0;
    static inline uintptr_t sAim3 = 0x38;
    static inline uintptr_t sAim4 = 0x2C;

    static inline uintptr_t LocalPlayerIsFiring = sAim1;
    static inline uintptr_t LastAimingInfoFromWeapon = sAim2;
    static inline uintptr_t WeaponInfo = sAim2;
    static inline uintptr_t GunTipPosition = sAim3;
    static inline uintptr_t BulletHit = sAim4;

    static inline uintptr_t ViewMatrix = 0xE8;

    static inline uintptr_t Weapon_FireComponent = 0x58;
    static inline uintptr_t Weapon_ChangeWeaponTime = 0x44;
    static inline uintptr_t Weapon_ChangeWeaponAnimSpeed = 0x48;
    static inline uintptr_t Weapon_LastFireTickCount = 0x444;
    static inline uintptr_t Weapon_LastFiringTime = 0x43C;
    static inline uintptr_t Weapon_LastPullBoltTime = 0x440;
    static inline uintptr_t Weapon_NotReadyTimeFromPreWeapon = 0x44;
    static inline uintptr_t Weapon_LastStartFireReqTickCount = 0x448;
    static inline uintptr_t Weapon_LastSwapSuccessTime = 0x3E8;

    static inline uintptr_t WeaponFireComponent_FireInterval = 0x120;

    static inline uintptr_t LocalPlayerAttributes = 0x500;
    static inline uintptr_t PlayerAttributes = 0x500;
    static inline uintptr_t NoReload = 0xC1;
    static inline uintptr_t UnlimitedAmmo = 0xE0;
    static inline uintptr_t RapidfirePtr = 0x1C8;
    static inline uintptr_t FireInterval = RapidfirePtr;
    static inline uintptr_t FireIntervalScaleTwo = RapidfirePtr;
    static inline uintptr_t FallingSpeedUpScale = 0x1D4;
    static inline uintptr_t RunSpeedUpScale = 0x1D8;
    static inline uintptr_t WalkDownSpeedScale = 0x1DC;

    static inline uintptr_t Player_SecondaryWeapon = 0x3F8;

    static inline uintptr_t Player_WeaponFireComponent = 0x8FC;

    static inline uintptr_t LevelObjectManager = 0x60;
    static inline uintptr_t LevelObjectManagerAlt = 0x64;
    static inline uintptr_t LootTransformComponent = 0x3C;
    static inline uintptr_t LootPickupList = 0xDC;
    static inline uintptr_t LootPickupListAlt = 0xC4;

    static inline uintptr_t InSnowSlideWayDashing = 0x15E8;

    static inline uintptr_t isBotOffs = 0x1B0;
    static inline uintptr_t HeadCollider = 0x4E8; // try kro ek baar
    static inline uintptr_t ReplaceCollider = LockedAimingCollider;
    static inline uintptr_t SilentFiring = 0x6C0;
    static inline uintptr_t SilentHit = 0x7D0;
    static inline uintptr_t Silents = sAim3;
    static inline uintptr_t Silentr = sAim4;

    static inline uintptr_t BaseProfileInfo = Player_Profile;
    static inline uintptr_t isFiring = LocalPlayerIsFiring;
    static inline uintptr_t StartPosition = GunTipPosition;
    static inline uintptr_t RayDir = BulletHit;
    static inline uintptr_t WeaponInfoOnWeapon = Weapon_Info;
    static inline uintptr_t ShadowState = Player_ShadowBase;
    static inline uintptr_t m_PlayerID = PlayerID;
    static inline uintptr_t LevelUp = Player_Profile;
    static inline uintptr_t Player_IsFemale = 0x7D8;

    class Bones {
    public:
        static inline uintptr_t Head = 0x49C;
        static inline uintptr_t Root = 0x4B0;
        static inline uintptr_t RootBone = 0x4B0;
        static inline uintptr_t Hip = 0x4A0;
        static inline uintptr_t Spine = 0x4A4;
        static inline uintptr_t Neck = 0x4A4;
        static inline uintptr_t Groin = 0x4AC;
        static inline uintptr_t LeftAnkle = 0x4B8;
        static inline uintptr_t RightAnkle = 0x4BC;
        static inline uintptr_t LeftFoot = 0x4C0;
        static inline uintptr_t RightFoot = 0x4C4;
        static inline uintptr_t LeftShoulder = 0x4D0;
        static inline uintptr_t RightShoulder = 0x4D4;
        static inline uintptr_t LeftHand = 0x4C8;
        static inline uintptr_t RightHand = 0x498;
        static inline uintptr_t LeftElbow = 0x4E4;
        static inline uintptr_t RightElbow = 0x4E0;
        static inline uintptr_t LeftWrist = 0x4DC;
        static inline uintptr_t RightWrist = 0x4D8;
        static inline uintptr_t LeftWristJoint = LeftWrist;
        static inline uintptr_t RightEye = 0x4A8;
        static inline uintptr_t HeadCollider = 0x4E8;
    };

    // ==================== FF MAX VARIANT SWITCH ====================
    // Call this once at startup depending on which game is running
    static void ApplyVariant(GameVariant variant)
    {
        if (variant == GameVariant::Max)
        {
            g_ActiveGameVariant = GameVariant::Max;

            // --- Core Offsets (FF Max) ---
            InitBase            = 0xA4632F0;
            Player_Name         = 0x324;
            Player_ShadowBase   = 0x1A70;
            ShadowState         = 0x1A70;
            AvatarManager       = 0x50C;
            FollowCamera        = 0x49C;
            AimRotation         = 0x448;
            MainCameraTransform = 0x290;
            Weapon              = 0x43C;
            LocalPlayerAttributes = 0x508;
            PlayerAttributes    = 0x508;
            IsClientBot         = 0x32C;
            isFiring            = 0x594;
            LocalPlayerIsFiring = 0x594;
            sAim1               = 0x594;
            sAim2               = 0x9D8;
            LastAimingInfoFromWeapon = 0x9D8;
            WeaponInfo          = 0x9D8;
            Collider            = 0x4F0;
            HeadCollider        = 0x4F0;
            AimbotVisible       = 0x4F0;

            // --- Bones (FF Max) ---
            Bones::Head          = 0x4A4;
            Bones::Neck          = 0x4AC;
            Bones::Spine         = 0x4AC;
            Bones::RightEye      = 0x4B0;
            Bones::RightShoulder = 0x4DC;
            Bones::LeftShoulder  = 0x4D8;
            Bones::RightElbow    = 0x4E8;
            Bones::LeftElbow     = 0x4EC;
            Bones::RightWrist    = 0x4E0;
            Bones::LeftWrist     = 0x4E4;
            Bones::LeftWristJoint= 0x4E4;
            Bones::RightHand     = 0x4A0;
            Bones::LeftHand      = 0x4D0;
            Bones::Hip           = 0x4A8;
            Bones::Groin         = 0x4B4;
            Bones::RightAnkle    = 0x4C4;
            Bones::LeftAnkle     = 0x4C0;
            Bones::RightFoot     = 0x4CC;
            Bones::LeftFoot      = 0x4C8;
            Bones::Root          = 0x4B8;
            Bones::RootBone      = 0x4BC;
            Bones::HeadCollider  = 0x4F0;
        }
        else
        {
            g_ActiveGameVariant = GameVariant::Normal;

            // --- Core Offsets (FF Normal) ---
            InitBase            = 0xA342EFC;
            Player_Name         = 0x31C;
            Player_ShadowBase   = 0x1A60;
            ShadowState         = 0x1A60;
            AvatarManager       = 0x504;
            FollowCamera        = 0x494;
            AimRotation         = 0x440;
            MainCameraTransform = 0x28C;
            Weapon              = 0x434;
            LocalPlayerAttributes = 0x500;
            PlayerAttributes    = 0x500;
            IsClientBot         = 0x324;
            isFiring            = 0x58C;
            LocalPlayerIsFiring = 0x58C;
            sAim1               = 0x58C;
            sAim2               = 0x9D0;
            LastAimingInfoFromWeapon = 0x9D0;
            WeaponInfo          = 0x9D0;
            Collider            = 0x4E8;
            HeadCollider        = 0x4E8;
            AimbotVisible       = 0x4E8;

            // --- Bones (FF Normal) ---
            Bones::Head          = 0x49C;
            Bones::Neck          = 0x4A4;
            Bones::Spine         = 0x4A4;
            Bones::RightEye      = 0x4A8;
            Bones::RightShoulder = 0x4D4;
            Bones::LeftShoulder  = 0x4D0;
            Bones::RightElbow    = 0x4E0;
            Bones::LeftElbow     = 0x4E4;
            Bones::RightWrist    = 0x4D8;
            Bones::LeftWrist     = 0x4DC;
            Bones::LeftWristJoint= 0x4DC;
            Bones::RightHand     = 0x498;
            Bones::LeftHand      = 0x4C8;
            Bones::Hip           = 0x4A0;
            Bones::Groin         = 0x4AC;
            Bones::RightAnkle    = 0x4BC;
            Bones::LeftAnkle     = 0x4B8;
            Bones::RightFoot     = 0x4C4;
            Bones::LeftFoot      = 0x4C0;
            Bones::Root          = 0x4B0;
            Bones::RootBone      = 0x4B0;
            Bones::HeadCollider  = 0x4E8;
        }
    }
};