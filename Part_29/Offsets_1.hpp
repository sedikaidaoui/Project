#pragma once

class Offsets {
public:
    // Base e Inicializa??o
    static inline uintptr_t Il2Cpp = 0x0;
    static inline uintptr_t InitBase = 0xA342EFC; // V3 UPDATE
    static inline uintptr_t StaticClass = 0x5C; // V3 UPDATE

    // Match e LocalPlayer
    static inline uintptr_t CurrentMatch = 0x50; // V3 UPDATE
    static inline uintptr_t MatchStatus = 0x8C; // V3 UPDATE
    static inline uintptr_t LocalPlayer = 0x94; // V3 UPDATE
    static inline uintptr_t DictionaryEntities = 0x68; // V3 UPDATE

    // Dados do Jogador
    static inline uintptr_t Player_IsDead = 0x50; // V3 UPDATE
    static inline uintptr_t Player_Name = 0x31C; // V3 UPDATE
    static inline uintptr_t Player_Data = 0x48; // V3 UPDATE
    static inline uintptr_t OreonWu = 0xC1C;//SAME
    static inline uintptr_t Player_ShadowBase = 0x1A60; // V3 UPDATE
    static inline uintptr_t XPose = 0x78;//SAME

    // Avatar e Visibilidade
    static inline uintptr_t AvatarManager = 0x504; // V3 UPDATE
    static inline uintptr_t Avatar = 0xA8; // V3 UPDATE
    static inline uintptr_t Avatar_IsVisible = 0x95; // V3 UPDATE
    static inline uintptr_t Avatar_Data = 0x14; // V3 UPDATE
    static inline uintptr_t Avatar_Data_IsTeam = 0x59; // V3 UPDATE

    // C?mera e Mira
    static inline uintptr_t FollowCamera = 0x494; // V3 UPDATE
    static inline uintptr_t Camera = 0x18; // V3 UPDATE
    static inline uintptr_t AimRotation = 0x440; // V3 UPDATE
    static inline uintptr_t MainCameraTransform = 0x28C; // V3 UPDATE
    static inline uintptr_t ViewMatrix = 0xE8;//IDK

    // Armas e Combate
    static inline uintptr_t Weapon = 0x434; // V3 UPDATE
    static inline uintptr_t WeaponData = 0x58; // V3 UPDATE
    static inline uintptr_t WeaponParams = 0x6C;   // Weapon.m_WeaponParams (embedded)
    static inline uintptr_t QuickSwap = 0x14C;     // SwitchWeaponTime
    static inline uintptr_t PreQuickSwap = 0x150;  // PreSwitchWeaponTime
    static inline uintptr_t PostQuickSwap = 0x154; // PostSwitchWeaponTime
    static inline uintptr_t WeaponRecoil = 0xC; // V3 UPDATE
    static inline uintptr_t AmmoOff = 0x48C;
    static inline uintptr_t IsClientBot = 0x324; // V3 UPDATE

    // Variveis Diversas
    static inline uintptr_t ColliderHECFNHJKOMN = 0x4E8; // V4 UPDATE
    static inline uintptr_t RubixHeadCollider = 0x4E8; // V4 UPDATE
    static inline uintptr_t fireColliders = 0x754; // m_fireColliders List
    static inline uintptr_t ColliderINICDNFOFJB = 0x54;
    static inline uintptr_t kanishkcheat = 0x54;
    static inline uintptr_t Vida = 0x10;
    static inline uintptr_t tiro = 0x2C;
    static inline uintptr_t telepneu = 0x0;
    static inline uintptr_t GhostMode = 0x38;
    static inline uintptr_t InSnowSlideWayDashing = 0x15E8;

    // KC No Gravity Fly (movement component on local player)
    static inline uintptr_t MovementComponent = 0x139C;
    static inline uintptr_t MoveCompPosition = 0x20;
    static inline uintptr_t MoveCompVSpeed = 0x2C;
    static inline uintptr_t MoveCompIsGrounded = 0x13F0; // on player directly
    static inline uintptr_t IsFiring = 0x53D;

    // Silent Legit offsets (FORCEAIM CPP)
    static inline uintptr_t SilentLegitIsFiring = 0x58C;    // LocalPlayer + offset -> bool//UPTED
    static inline uintptr_t SilentLegitWeaponInfo = 0x9D0;  // LocalPlayer + offset -> uint weapon ptr//UPTED
    static inline uintptr_t SilentLegitGunTip = 0x38;       // weaponData + offset -> Vector3 gun tip
    static inline uintptr_t SilentLegitBulletHit = 0x2C;    // weaponData + offset -> Vector3 bullet delta

    // Atributos e Timer
    static inline uintptr_t PlayerAttributes = 0x500; // V3 UPDATE
    static inline uintptr_t InfinitySkylerOffsets = 0x158;
    static inline uintptr_t InfinitySkillOffset = 0x158;
    static inline uintptr_t BuffDamage = 0xC4;
    static inline uintptr_t fastmed = 0x84; // PlayerAttributes.m_FSModeUseMedikitFasterRate
    static inline uintptr_t m_DriveSpeedScale = 0x51;
    static inline uintptr_t FireInterval = 0x188;
    static inline uintptr_t NoReload = 0xC1; // V3 UPDATE
    static inline uintptr_t GameTimer = 0x10; // V3 UPDATE
    static inline uintptr_t FixedDeltaTime = 0x24; // V3 UPDATE

    // KCMap_Teleport mark chain
    static inline uintptr_t UIInGameScene = 0x8;
    static inline uintptr_t BigMapCtrl = 0x218;
    static inline uintptr_t MapContentCtrl = 0x54;
    static inline uintptr_t LocalMapMarkController = 0x90;
    static inline uintptr_t MapMarkPosition = 0x58;
    static inline uintptr_t MoveCompPhysState = 0xC;
    static inline uintptr_t PhysStateFlyFlag = 0x8;

    // Transform matrix world position (Vector3 at +0x60: X, Y+4, Z+8)
    static inline uintptr_t MatrixPosition = 0x60;

    // UpPlayer122 Shake Kill � matrix reads (X write only)
    static inline uintptr_t ShakeKillPosX = 0x60;
    static inline uintptr_t ShakeKillPosY = 0x64;
    static inline uintptr_t ShakeKillPosZ = 0x68;
    static inline uintptr_t GameVariables = 0xB0;

    // Observador
    static inline uintptr_t CurrentObserver = 0xb4; // V3 UPDATE
    static inline uintptr_t ObserverPlayer = 0x28; // V3 UPDATE
    static inline uintptr_t m_LocalObserver = CurrentObserver; // on currentMatch when dead
    static inline uintptr_t m_TargetPlayer = ObserverPlayer;

    // Ajustes de Cmera
    static inline uintptr_t RightcameraOffset = 0x38;
    static inline uintptr_t FOVcameraoffset = 0x44;

    static inline uintptr_t LevelUp = 0x14A8;

    // ===== RAGE AIMBOT (EliteXzp AimbotPullThreadFunc) =====
    // The reference snippet listed 0x12C4 / 0x12B4 literals, which do not map
    // to the player object on this build; the validated rotation field (0x440)
    // is used for current/prev/backup so we never write into unrelated memory.
    static inline uintptr_t AimRotationPrev = AimRotation;
    static inline uintptr_t AimRotationBackup = AimRotation;
    static inline uintptr_t m_SwapWeaponTime = 0x570;
    static inline uintptr_t PlayerNetwork_HHCBNAPCKHF = Player_ShadowBase; // 0x1A60
    static inline uintptr_t ShadowState_TargetPos = 0x28;
    static inline uintptr_t ShadowState_StartRot = 0x34;
    static inline uintptr_t ShadowState_TargetRot = 0x44;
    static inline uintptr_t MainCamTransform = MainCameraTransform;
    static inline uintptr_t Player_IsFemale = 0x7D8;
    static inline uintptr_t isfemale = Player_IsFemale;

    class Bones {
    public:
        static inline uintptr_t Head = 0x49C; // V4 UPDATE
        static inline uintptr_t Neck = 0x4A4;//UPTED
        static inline uintptr_t Hip = 0x4A0; // V4 UPDATE
        static inline uintptr_t Root = 0x4B0; // V4 UPDATE
        static inline uintptr_t LeftShoulder = 0x4D0; // V4 UPDATE
        static inline uintptr_t RightShoulder = 0x4D4; // V4 UPDATE
        static inline uintptr_t LeftElbow = 0x4E4; // V4 UPDATE
        static inline uintptr_t RightElbow = 0x4E0; // V4 UPDATE
        static inline uintptr_t LeftHand = 0x4DC; // V4 UPDATE
        static inline uintptr_t RightHand = 0x4D8; // V4 UPDATE
        static inline uintptr_t RightWrist = 0x4D8; // V3 UPDATE
        static inline uintptr_t LeftWrist = 0x4DC; // V3 UPDATE
        static inline uintptr_t LeftFoot = 0x4C0; // V4 UPDATE
        static inline uintptr_t RightFoot = 0x4C4; // V4 UPDATE
        static inline uintptr_t LeftAnkle = 0x4B8; // V4 UPDATE
        static inline uintptr_t RightAnkle = 0x4BC; // V4 UPDATE
        static inline uintptr_t Groin = 0x4AC; // V3 UPDATE
    };

};
