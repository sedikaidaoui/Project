class Offsets {
public:
    static inline uintptr_t Il2Cpp;
    static inline uintptr_t InitBase;
    static inline uintptr_t StaticClass;
    static inline uintptr_t CurrentMatch;
    static inline uintptr_t MatchStatus;
    static inline uintptr_t LocalPlayer;
    static inline uintptr_t DictionaryEntities;
    static inline uintptr_t Player_IsDead;
    static inline uintptr_t Player_Name;
    static inline uintptr_t Player_Data;
    static inline uintptr_t Player_ShadowBase;
    static inline uintptr_t XPose;
    static inline uintptr_t AvatarManager;
    static inline uintptr_t Avatar;
    static inline uintptr_t Avatar_IsVisible;
    static inline uintptr_t Avatar_Data;
    static inline uintptr_t Avatar_Data_IsTeam;
    static inline uintptr_t HeadCollider;
    static inline uintptr_t FollowCamera;
    static inline uintptr_t Camera;
    static inline uintptr_t AimRotation;
    static inline uintptr_t MainCameraTransform;
    static inline uintptr_t Weapon;
    static inline uintptr_t WeaponData;
    static inline uintptr_t WeaponRecoil;
    static inline uintptr_t ViewMatrix;
    static inline uintptr_t isBotOffs;
    static inline uintptr_t Silent1;
    static inline uintptr_t Silent2;
    static inline uintptr_t sAim1;
    static inline uintptr_t sAim2;
    static inline uintptr_t sAim3;
    static inline uintptr_t sAim4;
    static inline uintptr_t IS_FIRING;
    static inline uintptr_t PlayerAttributes;
    static inline uintptr_t NoReload2;
    static inline uintptr_t m_FireIntervalScale;
    static inline uintptr_t m_FireIntervalScaleSkill;
    static inline uintptr_t m_FireIntervalScaleTwo;
    static inline uintptr_t Vida;
    static inline uintptr_t LevelUp;
    static inline uintptr_t pomba;
    static inline uintptr_t bisteca;
    static inline uintptr_t arma;
    static inline uintptr_t tiro;
    static inline uintptr_t Lock2X;
    static inline uintptr_t ColliderINICDNFOFJB;
    static inline uintptr_t GhostMode;
    static inline uintptr_t MovementComponent;
    static inline uintptr_t Position;
    static inline uintptr_t VSpeed;
    static inline uintptr_t IsGrounded;

    class Bones {
    public:
        static inline uintptr_t Head;
        static inline uintptr_t Root;
        static inline uintptr_t LeftWrist;
        static inline uintptr_t Spine;
        static inline uintptr_t Hip;
        static inline uintptr_t RightCalf;
        static inline uintptr_t LeftCalf;
        static inline uintptr_t RightFoot;
        static inline uintptr_t LeftFoot;
        static inline uintptr_t RightWrist;
        static inline uintptr_t LeftHand;
        static inline uintptr_t LeftShoulder;
        static inline uintptr_t RightShoulder;
        static inline uintptr_t RightWristJoint;
        static inline uintptr_t LeftWristJoint;
        static inline uintptr_t LeftElbow;
        static inline uintptr_t RightElbow;
        static inline uintptr_t Neck;
        static inline uintptr_t RightHand;
        static inline uintptr_t Hand;
        static inline uintptr_t RootBone;
        static inline uintptr_t Groin;
    };
};

void LoadFFMax()
{
    Offsets::Il2Cpp;
    Offsets::InitBase = 0x99B7E90;
    Offsets::StaticClass = 0x5C;
    Offsets::MatchStatus = 0x3C;
    Offsets::LocalPlayer = 0x44;
    Offsets::DictionaryEntities = 0x68;

    Offsets::Player_IsDead = 0x4C;
    Offsets::Player_Name = 0x254;
    Offsets::Player_Data = 0x44;
    Offsets::Player_ShadowBase = 0x14AC;
    Offsets::XPose = 0x78;
    Offsets::AvatarManager = 0x428;
    Offsets::Avatar = 0x94;
    Offsets::Avatar_IsVisible = 0x7C;
    Offsets::Avatar_Data = 0x10;
    Offsets::Avatar_Data_IsTeam = 0x51;
    Offsets::CurrentMatch = 0x50;
    Offsets::HeadCollider = 0x40C;

    Offsets::FollowCamera = 0x3B8;
    Offsets::Camera = 0x14;
    Offsets::AimRotation = 0x370;
    Offsets::MainCameraTransform = 0x1C0;

    Offsets::Weapon = 0x364;
    Offsets::WeaponData = 0x54;
    Offsets::WeaponRecoil = 0xC;
    Offsets::ViewMatrix = 0x98 + 0x24;
    Offsets::Silent1 = 0x4A8;
    Offsets::Silent2 = 0x87C;
    Offsets::PlayerAttributes = 0x424;
    Offsets::NoReload2 = 0x89;
    Offsets::isBotOffs = 0x25C;

    Offsets::Bones::Head = 0x3C0;
    Offsets::Bones::Root = 0x3D4;
    Offsets::Bones::LeftWrist = 0x3BC;
    Offsets::Bones::Spine = 0x3C8;
    Offsets::Bones::Hip = 0x3D0;
    Offsets::Bones::RightCalf = 0x3D8;
    Offsets::Bones::LeftCalf = 0x3DC;
    Offsets::Bones::RightFoot = 0x3E0;
    Offsets::Bones::LeftFoot = 0x3E4;
    Offsets::Bones::RightWrist = 0x3E8;
    Offsets::Bones::LeftHand = 0x3EC;
    Offsets::Bones::LeftShoulder = 0x3F4;
    Offsets::Bones::RightShoulder = 0x3F8;
    Offsets::Bones::LeftElbow = 0x408;
    Offsets::Bones::RightElbow = 0x404;
}

void LoadNormalFF()
{
    Offsets::Il2Cpp = 0x0;
    Offsets::InitBase = 0xA342EFC;

    Offsets::StaticClass = 0x5C;
    Offsets::CurrentMatch = 0x50;
    Offsets::MatchStatus = 0x8C;
    Offsets::LocalPlayer = 0x94;
    Offsets::DictionaryEntities = 0x68;

    Offsets::Player_IsDead = 0x50;
    Offsets::Player_Name = 0x31C;
    Offsets::Player_Data = 0x48;
    Offsets::Player_ShadowBase = 0x1A60;
    Offsets::XPose = 0x78;

    Offsets::AvatarManager = 0x504;
    Offsets::Avatar = 0xA8;
    Offsets::Avatar_IsVisible = 0x95;
    Offsets::Avatar_Data = 0x14;
    Offsets::Avatar_Data_IsTeam = 0x59;

    Offsets::FollowCamera = 0x494;
    Offsets::Camera = 0x18;
    Offsets::AimRotation = 0x440;
    Offsets::MainCameraTransform = 0x28C;

    Offsets::Weapon = 0x434;
    Offsets::WeaponData = 0x58;
    Offsets::WeaponRecoil = 0xC;

    Offsets::ViewMatrix = 0xE8;
    Offsets::PlayerAttributes = 0x500;
    Offsets::NoReload2 = 0xC1;
    Offsets::isBotOffs = 0x1B0;


    Offsets::sAim1 = 0x58C;
    Offsets::sAim2 = 0x9D0;
    Offsets::sAim3 = 0x38;
    Offsets::sAim4 = 0x2C;
    Offsets::IS_FIRING = 0x58C;
    Offsets::Silent1 = 0x6C0;
    Offsets::Silent2 = 0x7D0;

    // Collider / exploits
    Offsets::HeadCollider = 0x4E8;
    Offsets::ColliderINICDNFOFJB = 0x54;
    Offsets::GhostMode = 0x38;
    Offsets::Vida = 0x10;
    Offsets::LevelUp = 0x18CC;
    Offsets::pomba = 0x38;
    Offsets::bisteca = 0x2C;
    Offsets::arma = 0x38;
    Offsets::tiro = 0x2C;
    Offsets::Lock2X = 0x54;
    Offsets::MovementComponent = 0x14E8;
    Offsets::Position = 0x20;
    Offsets::VSpeed = 0x2C;
    Offsets::IsGrounded = 0x1554;

    Offsets::Bones::Head = 0x49C;
    Offsets::Bones::Root = 0x4B0;
    Offsets::Bones::RootBone = 0x4B0;
    Offsets::Bones::Hip = 0x4A0;
    Offsets::Bones::Spine = 0x4A4;
    Offsets::Bones::Neck = 0x4A4;
    Offsets::Bones::Groin = 0x4AC;
    Offsets::Bones::RightCalf = 0x4BC;
    Offsets::Bones::LeftCalf = 0x4B8;
    Offsets::Bones::RightFoot = 0x4C4;
    Offsets::Bones::LeftFoot = 0x4C0;
    Offsets::Bones::RightWrist = 0x4D8;
    Offsets::Bones::LeftWrist = 0x4DC;
    Offsets::Bones::LeftHand = 0x4C8;
    Offsets::Bones::RightHand = 0x498;
    Offsets::Bones::LeftShoulder = 0x4D0;
    Offsets::Bones::RightShoulder = 0x4D4;
    Offsets::Bones::LeftElbow = 0x4E4;
    Offsets::Bones::RightElbow = 0x4E0;
    Offsets::Bones::RightWristJoint = 0x4D8;
    Offsets::Bones::LeftWristJoint = 0x4DC;
    Offsets::Bones::Hand = 0x498;
}
void LoadFFPlaystore()
{
    Offsets::Il2Cpp;
    Offsets::InitBase = 0xA57115C;
    Offsets::StaticClass = 0x5C;
    Offsets::MatchStatus = 0x74;
    Offsets::LocalPlayer = 0x7C;
    Offsets::DictionaryEntities = 0x68;

    Offsets::Player_IsDead = 0x4C;
    Offsets::Player_Name = 0x294;
    Offsets::Player_Data = 0x44;
    Offsets::Player_ShadowBase = 0x15F0;
    Offsets::XPose = 0x78;
    Offsets::AvatarManager = 0x468;
    Offsets::Avatar = 0x94;
    Offsets::Avatar_IsVisible = 0x7C;
    Offsets::Avatar_Data = 0x10;
    Offsets::Avatar_Data_IsTeam = 0x51;
    Offsets::CurrentMatch = 0x50;
    Offsets::HeadCollider = 0x40C;

    Offsets::FollowCamera = 0x3F8;
    Offsets::Camera = 0x14;
    Offsets::AimRotation = 0x3B0;
    Offsets::MainCameraTransform = 0x200;

    Offsets::Weapon = 0x3A4;
    Offsets::WeaponData = 0x58;
    Offsets::WeaponRecoil = 0xC;
    Offsets::ViewMatrix = 0xBC;
    Offsets::Silent1 = 0x4E8;
    Offsets::Silent2 = 0x8F8;
    Offsets::PlayerAttributes = 0x464;
    Offsets::NoReload2 = 0x91;
    Offsets::isBotOffs = 0x29C;
    Offsets::m_FireIntervalScale = 0x17C;
    Offsets::m_FireIntervalScaleSkill = 0x180;
    Offsets::m_FireIntervalScaleTwo = 0x184;

    Offsets::Bones::Head = 0x400;
    Offsets::Bones::Root = 0x418;
    Offsets::Bones::Spine = 0x3B0;
    Offsets::Bones::Hip = 0x410;
    Offsets::Bones::LeftShoulder = 0x434;
    Offsets::Bones::LeftElbow = 0x448;
    Offsets::Bones::LeftWrist = 0x440;
    Offsets::Bones::LeftHand = 0x42C;
    Offsets::Bones::LeftWristJoint = 0x3E8;
    Offsets::Bones::LeftCalf = 0x3C4;
    Offsets::Bones::LeftFoot = 0x424;
    Offsets::Bones::RightShoulder = 0x438;
    Offsets::Bones::RightElbow = 0x444;
    Offsets::Bones::RightWrist = 0x43C;
    Offsets::Bones::RightWristJoint = 0x3E4;
    Offsets::Bones::RightCalf = 0x3C0;
    Offsets::Bones::RightFoot = 0x428;
}

void LoadFFmaxX86()
{
    Offsets::Il2Cpp;
    Offsets::InitBase = 0xB29057C;
    Offsets::StaticClass = 0x5C;

    Offsets::MatchStatus = 0x74;
    Offsets::LocalPlayer = 0x7C;
    Offsets::DictionaryEntities = 0x68;
    Offsets::CurrentMatch = 0x50;

    Offsets::Player_IsDead = 0x4C;
    Offsets::Player_Name = 0x27C;
    Offsets::Player_Data = 0x3C;
    Offsets::Player_ShadowBase = 0x15BC;
    Offsets::XPose = 0x78;
    Offsets::AvatarManager = 0x450;
    Offsets::Avatar = 0x94;
    Offsets::Avatar_IsVisible = 0x7C;
    Offsets::Avatar_Data = 0x10;
    Offsets::Avatar_Data_IsTeam = 0x4D;
    Offsets::PlayerAttributes = 0x44C;
    Offsets::NoReload2 = 0x91;
    Offsets::isBotOffs = 0xC0;

    Offsets::FollowCamera = 0x3E0;
    Offsets::Camera = 0x14;
    Offsets::AimRotation = 0x398;
    Offsets::MainCameraTransform = 0x200;

    Offsets::Weapon = 0x38C;
    Offsets::WeaponData = 0x58;
    Offsets::WeaponRecoil = 0xC;
    Offsets::ViewMatrix = 0xBC;

    Offsets::Silent1 = 0x4D0;
    Offsets::Silent2 = 0x8dc;
    Offsets::sAim1 = 0x4D0;
    Offsets::sAim2 = 0x8dc;
    Offsets::sAim3 = 0x38;
    Offsets::sAim4 = 0x2c;
    Offsets::IS_FIRING = 0x4d0;

    Offsets::Bones::Head = 0x3E8;
    Offsets::Bones::Root = 0x3FC;
    Offsets::Bones::LeftWrist = 0x3E4;
    Offsets::Bones::Spine = 0x3F0;
    Offsets::Bones::Hip = 0x3F8;
    Offsets::Bones::RightCalf = 0x400;
    Offsets::Bones::LeftCalf = 0x404;
    Offsets::Bones::RightFoot = 0x408;
    Offsets::Bones::LeftFoot = 0x40C;
    Offsets::Bones::RightWrist = 0x410;
    Offsets::Bones::LeftHand = 0x414;
    Offsets::Bones::LeftShoulder = 0x41C;
    Offsets::Bones::RightShoulder = 0x420;
    Offsets::Bones::RightWristJoint = 0x424;
    Offsets::Bones::LeftWristJoint = 0x428;
    Offsets::Bones::LeftElbow = 0x42C;
    Offsets::Bones::RightElbow = 0x430;
}
