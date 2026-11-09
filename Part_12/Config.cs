using System.Drawing;
using System.Numerics;
using System.Runtime.CompilerServices;
using System.Windows.Forms;

namespace AotForms
{
    internal static class Config
    {
        internal static bool speed = false;
        internal static bool rgb = false;
        internal static float iconsize = 1.0f;
        public static Vector4 ICONCOLOR = new Vector4(1f, 1f, 1f, 1f);
        internal static bool espweapon = false;  
        internal static float AimBotSmooth = 0f;
        internal static AimBotType AimBotType;
        internal static bool enableAimBot = false;
        internal static bool AimLock = false;
        internal static bool TriggerBot = false;
        internal static bool DrawClosest = false;
        internal static bool AutoLoadConfig = false;
        internal static bool AimBotRage = false;
        internal static bool SilentAim = false;
        internal static bool AimbotVisible = false;
        internal static bool kcsafeSilentAim = false;
        internal static bool kcbrutasilnetaim = false;
        internal static bool RapidFire = false;
        internal static float RapidFireSpeed = 0.5f;
        internal static bool FastReload = false;
        internal static bool UpPlayer = false;
        internal static bool tele = false;
        internal static bool ShakeKill = false;
        internal static bool flyhack = false;
        internal static bool UnlimitedAmmo = false;
        internal static bool teliportwall = false;
        internal static bool alignment = true;


        internal static Keys AimbotKey = Keys.LButton; 

        internal static int AimBotMaxDistance = 200;

        internal static bool IgnoreKnocked = false;
        internal static bool UpdateEntities = false;
        internal static bool NoRecoil = false;
        internal static bool NoCache = false;
        internal static int AimbotSmoothness = 0;
        internal static int AimBotFov = 100000;
        internal static bool speedint = false;

        internal static TargetingMode TargetingMode  = TargetingMode.ClosestToCrosshair;

        internal static bool ESPLine = false;
        internal static Color ESPLineColor = Color.White;
        internal static float AimFov = 200f;
        internal static float Esprender = 80f;
        internal static bool StreamMode = false;
        
        internal static Color NameCheat = Color.Red;
        internal static bool ESPBox = false;
        internal static float speedVal = 5.0f;
        internal static bool Speed = false;
        internal static bool speedext = false;
        internal static bool wallhack = false;

        internal static Color ESPBoxColor = Color.White;
        internal static bool teliport = false;
        internal static bool Kcspawnkill = false;
        internal static bool ClimbUpEnabled = false;
        internal static bool DownPlayer = false;
        internal static bool EnemyPullEnabled = false;
        internal static bool EnemyPullEnabledv2 = false;
        internal static bool forceaim = false;
        internal static bool undergroundkill = false;
        internal static bool MAGNETPULL = false;
        internal static bool FlyHack = false;
        internal static bool spinbot = false;
        internal static float SpinBotSpeed = 4.2f;
        internal static bool highjumppp = false;
        internal static bool cameraleftt = false;
        internal static bool visionhackkk = false;
        internal static bool flyhacks = false;
        internal static bool teleportmap = false;
        internal static bool brsutfiree = false;

        internal static int EnemyPullTickMs = 6;
        internal static float EnemyPullMaxDistance = 150f;
        internal static bool ESPInformation = false;
        internal static bool espdistance = false;
        internal static bool ESPName = false;
        internal static Color ESPNameColor = Color.White;
        internal static bool ESPHealth = false;
        internal static bool ESPHealthText = false;
        internal static Color ESPHealthColor = Color.Green;
        internal static bool ESPSkeleton = false;  
        internal static Color ESPSkeletonColor = Color.White;
        internal static bool ESPRGB = false;
        internal static bool AimTrackLine = false;
        internal static Color AimTrackLineColor = Color.Red;
        internal static bool HackerTag = false;
        internal static Color HackerTagColor = Color.Red;
        internal static bool EspTimer = false;
        internal static bool ESPEditor = false;
        internal static bool EnemyTimerActive = false;
        internal static System.DateTime EnemyTimerStart = System.DateTime.MinValue;
        internal static System.DateTime LastEnemySeen = System.DateTime.MinValue;
        internal static int EnemyTimerDuration = 60;
        internal static int ResetAfterNoEnemy = 10;
        internal static bool FOVEnabled = false;
        
        internal static Color FOVColor = Color.White;
        internal static LinePosition ESPLinePosition = LinePosition.Top;

        internal static Vector2 EspWeaponIconOffsetPx = Vector2.Zero;
        internal static Vector2 EspNameOffsetPx = Vector2.Zero;
        internal static Vector2 EspHackerOffsetPx = Vector2.Zero;
        internal static Vector2 EspBoxOffsetPx = Vector2.Zero;
        internal static Vector2 EspHealthBarOffsetPx = Vector2.Zero;
        internal static Vector2 EspSnapLineStartOffsetPx = Vector2.Zero;
        internal static Vector2 EspSnapLineEndOffsetPx = Vector2.Zero;
        /// <summary>Fine offset for distance text when it is drawn outside the nameplate (editor + in-game).</summary>
        internal static Vector2 EspDistanceOffsetPx = Vector2.Zero;

        internal static EspNameBoxAnchor EspNameAnchor = EspNameBoxAnchor.BoxTopCenter;
        internal static EspDistanceBoxAnchor EspDistanceAnchor = EspDistanceBoxAnchor.InsideNameplate;
        internal static EspHealthBoxAnchor EspHealthAnchor = EspHealthBoxAnchor.BoxRight;
        internal static EspWeaponBoxAnchor EspWeaponAnchor = EspWeaponBoxAnchor.BoxTopCenter;

        internal static float cameraVal = 1.0f;
        internal static float visionVal = 3.141592741f;

        // Local UI state for the wardrobe preview. This intentionally does not
        // read from or write to any external process.
        internal static bool WardrobeEnabled = true;
        internal static string SelectedWardrobeCategory = "Top";
        internal static uint SelectedWardrobeItemId;
        internal static string SelectedWardrobeItemName = "No item selected";
    }
    public enum TargetingMode
    {
        ClosestToCrosshair,
        Target360,
        ClosestToPlayer,
        LowestHealth,
    }
    public enum AimBotType
    {
        Silent,
        Rage
    }
    public enum LinePosition
    {
        Top,
        Center,
        Bottom
    }

    /// <summary>Where ESP name / nameplate sits relative to the corner ESP box.</summary>
    public enum EspNameBoxAnchor
    {
        BoxTopCenter,
        BoxBottomCenter,
        BoxTopLeft,
        BoxTopRight,
        BoxBottomLeft,
        BoxBottomRight,
        BoxMiddleLeft,
        BoxMiddleRight,
    }

    /// <summary>Where distance text is drawn (ESP Information). InsideNameplate = original bar layout.</summary>
    public enum EspDistanceBoxAnchor
    {
        InsideNameplate,
        AboveNameWhenNameAtTop,
        BoxTopCenter,
        BoxBottomCenter,
        BoxTopLeft,
        BoxTopRight,
        BoxBottomLeft,
        BoxBottomRight,
        BoxMiddleLeft,
        BoxMiddleRight,
    }

    public enum EspHealthBoxAnchor
    {
        BoxRight,
        BoxLeft,
        BoxTop,
        BoxBottom,
    }

    public enum EspWeaponBoxAnchor
    {
        BoxTopCenter,
        BoxBottomCenter,
        BoxTopLeft,
        BoxTopRight,
        BoxBottomLeft,
        BoxBottomRight,
        BoxMiddleLeft,
        BoxMiddleRight,
    }
   
}
