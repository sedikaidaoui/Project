#pragma once

class Offsets {
public:
    // Base e Inicialização
    static inline uintptr_t Il2Cpp = 0x0; //OK FIXO PONTO
    static inline uintptr_t InitBase = 0xA342EFC; //OK
    static inline uintptr_t GameFacade_Script = 0xA5BC344; //OK (OB54)
    static inline uintptr_t StaticClass = 0x5C; //OK (Access_StaticClass)
    static inline uintptr_t Access_StaticClass = 0x5C; //alias
    static inline uintptr_t CurrentMatch = 0x50; //OK (m_Match)
    static inline uintptr_t CurrentMatchGame = 0x4; //OK (OB54)
    static inline uintptr_t MatchStatus = 0x8C; //OK (MatchState)
    static inline uintptr_t Level = 0x14; //OK (OB54)
    static inline uintptr_t LocalPlayer = 0x94;  //OK (OB54)
    static inline uintptr_t DictionaryEntities = 0x68;  //OK
    static inline uintptr_t Player_IsDead = 0x50; //OK (OB54)
    static inline uintptr_t Player_Name = 0x31C; //OK (OB54)
    static inline uintptr_t Player_Data = 0x48; //OK (Player_Data_Pool OB54)
    static inline uintptr_t Player_ShadowBase = 0x1A60; //OK (OB54)
    static inline uintptr_t ShadowState = 0x1A60; //alias V7A
    static inline uintptr_t XPose = 0x78; //OK
    static inline uintptr_t LocalPlayerAttributes = 0x500;  //OK (LocalPlayerAttrib OB54)
    static inline uintptr_t AimbotVisible = 0x4E8;  //OK (OB54)
    static inline uintptr_t isBotOffs = 0x1B0; //OK (OB55)
    static inline uintptr_t IsClientBot = 0x324; //OK (OB54)
    static inline uintptr_t UnlimitedAmmo = 0xE0; //OK (OB55)
    static inline uintptr_t FixedDeltaTime = 0x24; //OK (OB54)
    static inline uintptr_t InSnowSlideWayDashing = 0x15E8; //OK (OB55)
    static inline uintptr_t AvatarManager = 0x504; //OK (OB54)
    static inline uintptr_t Avatar = 0xA8; //OK (OB54)
    static inline uintptr_t Avatar_IsVisible = 0x95; //OK (OB54)
    static inline uintptr_t Avatar_Data = 0x14; //OK (OB54)
    static inline uintptr_t Avatar_Data_IsTeam = 0x59; //OK (OB54)
    static inline uintptr_t FollowCamera = 0x494; //OK (OB54)
    static inline uintptr_t Camera = 0x18;  //OK (OB54)
    static inline uintptr_t Camera_Intptr = 0x8; //OK (OB54)
    static inline uintptr_t AimRotation = 0x440; //OK (OB54)
    static inline uintptr_t AuxAimRotation = 0x400;//UPTED
    static inline uintptr_t MainCameraTransform = 0x28C; //OK (OB54)
    static inline uintptr_t PanelToWorld = 0xE8; //OK (Matrix4x4 panelToWorld - matriz W2S)
    static inline uintptr_t ViewMatrix = 0xE8;  //alias PanelToWorld
    static inline uintptr_t Vida = 0x10;

    // ===== CLASSE DE OSSOS (OB55 / kira leak) =====
    class Bones {
    public:
        // Ossos principais (OB55 kira)
        static inline uintptr_t Head = 0x49C;
        static inline uintptr_t Hip = 0x4A0;
        static inline uintptr_t Spine = 0x4A4;
        static inline uintptr_t Neck = 0x4A4;             // kira nao tem Neck -> alias Spine
        static inline uintptr_t Groin = 0x4AC;
        static inline uintptr_t Root = 0x4B0;             // Root_Pes
        static inline uintptr_t Root_Pes = 0x4B0;
        static inline uintptr_t RootBone = 0x4B0;         // alias Root

        // Pernas / Tornozelos
        static inline uintptr_t LeftAnkle = 0x4B8;
        static inline uintptr_t RightAnkle = 0x4BC;
        static inline uintptr_t LeftToe = 0x4C0;          // alias LeftFoot
        static inline uintptr_t RightToe = 0x4C4;         // alias RightFoot
        static inline uintptr_t LeftCalf = 0x4B8;         // alias LeftAnkle
        static inline uintptr_t RightCalf = 0x4BC;        // alias RightAnkle
        static inline uintptr_t LeftFoot = 0x4C0;
        static inline uintptr_t RightFoot = 0x4C4;

        // Ombro / Mãos / Cotovelos / Pulsos
        static inline uintptr_t LeftShoulder = 0x4D0;
        static inline uintptr_t RightShoulder = 0x4D4;
        static inline uintptr_t LeftHand = 0x4C8;
        static inline uintptr_t RightHand = 0x498;        // OB55 kira
        static inline uintptr_t LeftElbow = 0x4E4;
        static inline uintptr_t RightElbow = 0x4E0;
        static inline uintptr_t LeftWrist = 0x4DC;
        static inline uintptr_t RightWrist = 0x4D8;

        // Alias adicionais
        static inline uintptr_t Pelvis = 0x4A0;           // alias Hip
        static inline uintptr_t HipCenter = 0x4A0;        // alias Hip
        static inline uintptr_t Collider = 0x4B0;
    };

    // ===== OBSERVER =====
    static inline uintptr_t CurrentObserver = 0xB4;
    static inline uintptr_t ObserverPlayer = 0x28;
    static inline uintptr_t m_LocalObserver = CurrentObserver; // when dead
    static inline uintptr_t m_TargetPlayer = ObserverPlayer;

    // ===== WEAPON =====
    static inline uintptr_t Weapon = 0x434;
    static inline uintptr_t WeaponData = 0x58;
    static inline uintptr_t WeaponRecoil = 0xC;      // WeaponRecoil_Scatter
    static inline uintptr_t WeaponOnHand = 0x54;
    static inline uintptr_t WeaponReload = 0x99;    // UPDATED
    static inline uintptr_t AmmoOff = 0x48C;
    static inline uintptr_t NoRecoilFlag = 0x548;   // flag de no recoil
    static inline uintptr_t IsFiring = 0x9D0;
    static inline uintptr_t Weapon_IsSighting = 0x5CC;
    static inline uintptr_t Weapon_LastPullBoltTime = 0x440;
    static inline uintptr_t Player_SecondaryWeapon = 0x3F8;
    static inline uintptr_t Weapon_ItemId = 0x494;
    static inline uintptr_t weaponinfo = 0x950; //V7A
    static inline uintptr_t WeaponInfo = 0x64;      // WeaponInfo_Type
    static inline uintptr_t WeaponID = 0x14;
    static inline uintptr_t guntipposition = 0x38;
    static inline uintptr_t bullet_hit = 0x2C;
    static inline uintptr_t NoReload = 0xC1;        // V3 UPDATE
    static inline uintptr_t NoReload2 = 0x89;       // UPDATED
    static inline uintptr_t IS_FIRING = 0x58C;      // V7A firing
    static inline uintptr_t saim1 = 0x58C;          // alias IS_FIRING
    static inline uintptr_t saim2 = 0x9D0;          // weapon info
    static inline uintptr_t sAim1 = 0x58C;          // alias IS_FIRING
    static inline uintptr_t sAim2 = 0x9D0;          // weapon info (alias saim2)
    static inline uintptr_t sAim3 = 0x38;           // guntip
    static inline uintptr_t sAim4 = 0x2C;           // bullet hit
    // Silent Aim V2 (port BrutalSilent / BR MODE BY DEATH ENGINE)
    static inline uintptr_t BrutalSilentWeaponInfo = 0x9D0;
    static inline uintptr_t BrutalSilentGunTipPosition = 0x38;
    static inline uintptr_t BrutalSilentBulletHit = 0x2C;
    static inline uintptr_t tiro = 0x58C;           // alias IS_FIRING (DownPlayer)
    static inline uintptr_t InfiniteIceWall = 0x78; // V7A
    static inline uintptr_t ReloadNoConsumeAmmoclip = 0xC0; // V7A
    static inline uintptr_t BuffAmmo = 0x104;       // V7A

    // ===== AIMBOT VISIBLE =====
    static inline uintptr_t LockedAimingCollider = 0x54;
    static inline uintptr_t Collider = 0x4A4;
    static inline uintptr_t HeadCollider = 0x4E8;   // V7A
    static inline uintptr_t ReplaceCollider = 0x54; // OB55
    // Colliders (OB54 / strixgg) - usados no Aimbot, RageAimbot e Data.cpp
    static inline uintptr_t ColliderHECFNHJKOMN = 0x4A4;      // Head Collider (ptr)
    static inline uintptr_t ColliderINICDNFOFJB = 0x54;       // LockedAimingCollider
    static inline uintptr_t RubixHeadCollider = 0x4A4;        // alias Head Collider (Aimbot.cpp)
    static inline uintptr_t kanishkcheat = 0x54;              // alias LockedAimingCollider (OB54 C#)
    // Aimbot Lite (AimBotVisibleSafe)
    static inline uintptr_t AimBotLiteHeadCollider = 0x360;   // lectura collider de cabeza
    static inline uintptr_t AimBotLiteCollider = 0x4E8;       // lectura alternativa

    // ===== MOVEMENT / PLAYER COMPONENTS =====
    static inline uintptr_t MovementComponent = 0x139C;
    static inline uintptr_t MoveCompPosition = 0x20;
    static inline uintptr_t MoveCompVSpeed = 0x2C;
    static inline uintptr_t MoveCompIsGrounded = 0x13F0;

    // ===== TIMERS =====
    static inline uintptr_t PlayerAttributes = 0x500;   // LocalPlayerAttrib (OB54)
    static inline uintptr_t GameTimer = 0x10;            // GameTimer (OB54)
    // ===== PROFIL (BaseProfileInfo) =====
    //  (1) ESP RANK - COPIA (le projet de reference) : 0x18CC   <-- utilise en 1er
    //  (2) note "FF India Max / OB55" de ce meme projet : 0x18DC
    //  (3) dump OB55 V7A (1.130.1), class PlayerNetwork  : 0x1A74
    //  L'ordre de lecture est dans RankEsp::GetProfile().
    static inline uintptr_t ProfileInfo = 0x18CC;        // reference (PROFIL)
    static inline uintptr_t ProfileInfoOB55 = 0x18DC;    // note OB55 de la reference
    static inline uintptr_t Player_ProfileDump = 0x1A74; // dump OB55 V7A
    static inline uintptr_t LevelUp = 0x18CC;            // alias ProfileInfo
    static inline uintptr_t Player_Profile = 0x18CC;     // alias ProfileInfo
    // Champs DANS BaseProfileInfo (reference + dump) :
    //   AccountID 0x8 | Level 0x10(ref) / 0x14(dump) | Rank 0x58 | RankPoint 0x5C
    static inline uintptr_t Profile_Rank = 0x58;         // Rank (tier id)
    static inline uintptr_t Profile_RankPoint = 0x5C;    // RankPoint (score de rang)
    static inline uintptr_t ProfileRankId = 0x58;        // alias Profile_Rank
    static inline uintptr_t ProfileScore = 0x5C;         // alias Profile_RankPoint
    static inline uintptr_t OreonWu = 0xC1C;             // SAME
    static inline uintptr_t HPose = 0x78;                // SAME

    // ===== KCMap_Teleport Y Mapa =====
    static inline uintptr_t UIInGameScene = 0x8;
    static inline uintptr_t m_UIScene = 0x8;            // alias
    static inline uintptr_t BigMapCtrl = 0x240;         // V7A
    static inline uintptr_t m_BigMapCtrl = 0x240;       // alias V7A
    static inline uintptr_t MapContentCtrl = 0x5C;      // V7A
    static inline uintptr_t m_MapContentCtrl = 0x5C;    // alias V7A
    static inline uintptr_t LocalMapMarkController = 0x94; // V7A
    static inline uintptr_t m_LocalMapMarkController = 0x94; // alias V7A
    // MapMarkPosition : OB55 / V7A = 0x60 (0x58 = stale older build -> wrong landing spot)
    static inline uintptr_t MapMarkPosition = 0x60;
    static inline uintptr_t m_pos = 0x60;               // V7A (Vector3 pos)
    static inline uintptr_t MoveCompPhysState = 0xC;
    static inline uintptr_t PhysStateFlyFlag = 0x8;

    // Transform matrix world position (Vector3 at +0x60)
    static inline uintptr_t MatrixPosition = 0x601aa;
    static inline uintptr_t GameVariables = 0xB0;

    // Ajustes de Cámara
    static inline uintptr_t RightcameraOffset = 0x38;
    static inline uintptr_t FOVcameraoffset = 0x44;
    static inline uintptr_t Backcameraoffset = 0x40;
    static inline uintptr_t Upcameraoffset = 0x3C;
    static inline uintptr_t Phase1CameraEulerAnglesY = 0x48;

    // Speed Timer / No Gravity
    static inline uintptr_t GameFacadeTimer = 0x10;
    static inline uintptr_t TimerFixedDeltaTime = 0x24;
    static inline uintptr_t Position = 0x20;
    static inline uintptr_t VSpeed = 0x2C;
    static inline uintptr_t IsGrounded = 0x13F0;
    static inline uintptr_t FlyHack = 0x1B0;
    static inline uintptr_t GhostMode = 0x38;

    // CÁMARA FOV (Vision)
    static inline uintptr_t FollowCameraFOVOffset = 0x48;

    // Possição da Transform (shake kill / telepneu): Vector3 world pos +0x60
    static inline uintptr_t ShakeKillPosX = 0x60;
    static inline uintptr_t ShakeKillPosY = 0x64;
    static inline uintptr_t ShakeKillPosZ = 0x68;
    static inline uintptr_t telepneu = 0x100;       // AVT9 legacy

    // BackJump (GameVarDef) - detectado en runtime // (FF MAX V7A)
    static inline uintptr_t EnableAccelerationOnFalling = 0x28F3;   // bool
    static inline uintptr_t EnableLowFallingSwapWeapon = 0x2C3D;    // bool

    // ===== SEMI TELA PARADA / MOVEMENT 360 (UserControlHandler) =====
    static inline uintptr_t UserControl = 0x344;          // Player -> protected UserControlHandler m_UserControl
    static inline uintptr_t AxisData = 0x34;              // private UserControlAxisData[] m_AxisData
    static inline uintptr_t FingerInDashArea = 0x4C;      // private int m_FingerInDashArea
    static inline uintptr_t IsTouched = 0x37;             // class UserControlAxisData -> bool m_IsTouched
    static inline uintptr_t LockFingerInDashArea = 0x50;  // private bool m_LockFingerInDashArea
    static inline uintptr_t DashByMovingJoystick = 0x58;  // private bool <DashByMovingJoystick>k__BackingField

    // ===== SILENT AIM LEGIT (OB54 / LYX PAPA) =====
    static inline uintptr_t SilentLegitIsFiring = 0x58C;    // sAim1 (localPlayer bool)
    static inline uintptr_t SilentLegitWeaponInfo = 0x9D0;  // sAim2 (localPlayer -> weaponData)
    static inline uintptr_t SilentLegitGunTip = 0x38;       // sAim3 (weaponData Vector3)
    static inline uintptr_t SilentLegitBulletHit = 0x2C;    // sAim4 (weaponData Vector3)

    // ===== EXPLOITS ATTRIBUTES (PlayerAttributes) =====
    static inline uintptr_t FireInterval = 0x1C8;           // Rapid Fire (OB54 RapidfirePtr)
    static inline uintptr_t fastmed = 0x8C;                 // FSModeUseMedikitFasterRate (verificar)
    static inline uintptr_t BuffDamage = 0xF8;              // BuffWeaponDamageScale (set 32-bit ; set 64-bit = 0x128)
    static inline uintptr_t m_DriveSpeedScale = 0x170;      // Speed Car (verificar)
    static inline uintptr_t InfinitySkylerOffsets = 0x120;  // Cooldown / Skiler (verificar)
    static inline uintptr_t InfinitySkillOffset = 0x124;    // PetSkillCDReduction (verificar)

    // ===== FAST SWITCH (WeaponParams.SwitchWeaponTime) =====
    static inline uintptr_t WeaponParams = 0x0;             // AVT9
    static inline uintptr_t QuickSwap = 0x528;              // OB54
    static inline uintptr_t PreQuickSwap = 0x4BC;           // OFF_SWAP_WEAPON_TIME (verificar)
    static inline uintptr_t PostQuickSwap = 0x52C;          // adjacente a QuickSwap (verificar)

    // ===== CHEST COLLIDER (FireColliders List) =====
    static inline uintptr_t fireColliders = 0x4A8;          // (VERIFICAR - sem referência)
};