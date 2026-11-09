using Guna.UI2.WinForms;
using ImGuiNET;
using Memory;
using System;
using System.Diagnostics;
using System.Numerics;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Threading.Tasks;
using NAudio.Wave;
using AotForms.Components;
using static AotForms.Config;
using static AotForms.WinAPI;
using static System.Windows.Forms.VisualStyles.VisualStyleElement;
using Color = System.Drawing.Color;
using Size = System.Drawing.Size;

namespace AotForms
{
    internal class ESP : ClickableTransparentOverlay.Overlay
    {
        IntPtr hWnd;
        IntPtr HDPlayer;
        private CX memoryfast = new CX();
        private bool camera = false;
        private bool fly = false;
        private bool jump = false;
        private bool left = false;
        private bool vision = false;
        private bool brust = false;
        private bool speedext = false;
        private bool wallhackkk = false;

        private IEnumerable<long> visionResult;
        private IEnumerable<long> cameraResult;
        private float previousCameraVal = Config.cameraVal;
        private float previousVisionVal = Config.visionVal;
        private bool _styleInitialized = false;
        private const short DefaultMaxHealth = 200;
        private Vector4 lineColor = new Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        private Vector4 fovColor = new Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        private Vector4 boxColor = new Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        private Vector4 box3DColor = new Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        private Vector4 skeletonColor = new Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        private static Dictionary<string, IntPtr> _weaponIcons = new();

        private static IntPtr _espEditorPreviewPtr = IntPtr.Zero;
        private static uint _espEditorPreviewW;
        private static uint _espEditorPreviewH;
        private static bool _espEditorPreviewLoadAttempted;
        private static IntPtr _espEditorWeaponPreviewPtr = IntPtr.Zero;
        private static uint _espEditorWeaponPreviewW;
        private static uint _espEditorWeaponPreviewH;
        private static bool _espEditorWeaponPreviewLoadAttempted;
        private static Vector2 _mainPanelPos;
        private static Vector2 _mainPanelSize;

        private enum EspEditorDragKind { None, LineStart, LineEnd, Box, Name, Distance, Health, Weapon, Hacker }
        private enum EspEditorColorTarget { None, Line, Box, Name, Health, Weapon, Hacker }

        private EspEditorDragKind _espEditorDragKind = EspEditorDragKind.None;
        private EspEditorColorTarget _espEditorColorTarget = EspEditorColorTarget.None;
        private bool _espEditorColorPopupOpen;
        private Vector2 _espEditorColorPopupPos = Vector2.Zero;
        private EspEditorColorTarget _espEditorColorClickTarget = EspEditorColorTarget.None;
        private Vector2 _espEditorColorClickStart = Vector2.Zero;

        private static float EspEditorDistSq(Vector2 a, Vector2 b)
        {
            float dx = a.X - b.X, dy = a.Y - b.Y;
            return dx * dx + dy * dy;
        }

        private static float EspEditorDistToSegmentSq(Vector2 p, Vector2 a, Vector2 b)
        {
            Vector2 ab = b - a;
            float t = ab.X * ab.X + ab.Y * ab.Y;
            if (t < 1e-6f) return EspEditorDistSq(p, a);
            float u = Math.Clamp(((p.X - a.X) * ab.X + (p.Y - a.Y) * ab.Y) / t, 0f, 1f);
            float px = a.X + ab.X * u;
            float py = a.Y + ab.Y * u;
            float dx = p.X - px, dy = p.Y - py;
            return dx * dx + dy * dy;
        }

        private static bool EspEditorPointInRect(Vector2 p, Vector2 mn, Vector2 mx)
        {
            return p.X >= mn.X && p.X <= mx.X && p.Y >= mn.Y && p.Y <= mx.Y;
        }

        private static bool EspEditorPointInRectPad(Vector2 p, Vector2 mn, Vector2 mx, float pad)
        {
            return p.X >= mn.X - pad && p.X <= mx.X + pad && p.Y >= mn.Y - pad && p.Y <= mx.Y + pad;
        }

        private IntPtr _brandIcon = IntPtr.Zero;
        private static readonly Random _hackerRandom = new();
        private static uint _hackerEntityAddress = 0;
        private static DateTime _lastHackerUpdate = DateTime.MinValue;
        private const double HackerUpdateIntervalSeconds = 3.0;
        public static Vector4 ThemeColor = new Vector4(0.10f, 0.78f, 0.94f, 1f);
        public static ImFontPtr HeaderFont;
        public static ImFontPtr LebelFont;
        public static ImFontPtr TextFont;

        private static bool _uiVisible = true;
        private static float _uiAlpha = 1.0f;
        private static float _uiFadeSpeed = 15.0f;
        private static float _uiFadeOutSpeed = 48.0f;
        private static bool _insertKeyPressedLastFrame = false;
        private static bool _vkInsertDownLast = false;
        private static readonly Dictionary<int, float> _tabFade = new();
        private static readonly Dictionary<int, float> _tabAnim = new();
        private static float _uiGlowAnim = 0f;
        private static float _tabDropLocalY = -1f;
        private static float _tabDropTargetLocalY = -1f;
        private static float _tabDropLocalX = 0f;
        private static float _tabDropLocalW = 0f;
        private static float _tabDropLocalH = 0f;
        private static bool _tabDropTargetValid = false;
        private static readonly Dictionary<int, IntPtr> _tabIconPtrs = new();
        private static bool _tabIconsInitialized = false;
        private static bool _faMergedIntoTextFont;
        private static byte[]? _retainedFaSolidBytes;
        private static bool _showTabPopup = false;
        private static float _tabPopupAnim = 0f;
        private static int _previousTab = 0;
        private static int _activeTab = 0;

        private enum FlowPhase { Login, PostLoginLoad, Main }
        private static FlowPhase _flowPhase = FlowPhase.Login;
        private static string _loginLicense = "";
        private static string _loginError = "";
        private static bool _loginBusy = false;
        private static Task<string?>? _emulatorInitTask;
        private static volatile bool _loginDone;
        private static bool _loginOk;
        private static string _loginResultMsg = "";

        private static float _loaderTime = 0f;
        private static float _connectedFade = 0f;
        private static bool _connected = false;
        private static bool _soundPlayed = false;
        private static float _loaderAngle = 0f;
        private static float _pulseAlpha = 0.5f;
        private static bool _pulseIncreasing = true;

        private static bool _showHookSuccessScreen;
        private static float _hookSuccessAnimTime;
        private const float HookSuccessAnimDuration = 2.75f;
        private const float SidebarTabTopPad = 4f;

        private static readonly Vector4 PanelBg = new(0.04f, 0.05f, 0.07f, 1f);
        private static readonly Vector4 PanelBorder = new(0.08f, 0.11f, 0.14f, 1f);
        /// <summary>Font Awesome solid glyphs (merged 0xF000–0xF8FF in TextFont). Order: Aim, Esp, Misc, Cfg, Ext.</summary>
        private static readonly string[] TabFaGlyphs =
        {
            "\uF05B", // crosshairs
            "\uF06E", // eye
            "\uF06D", // fire
            "\uF7D9", // screwdriver-wrench (FA6 solid); falls back to label if missing in font
            "\uF007", // user
        };
        private static readonly Vector4 TabAccentRed = new(0.10f, 0.78f, 0.94f, 1f);
        private static readonly Vector4 HexBrandRed = new(0.10f, 0.78f, 0.94f, 1f);
        private static readonly Vector4 HexTabUnderlineRed = new(0.10f, 0.78f, 0.94f, 1f);
        private readonly string[] _comboItems2 = { "Closest To Crosshair", "Target360", "Closest To Player", "Lowest Health" };
        private readonly string[] _comboItems1 = { "Silent Aim", "Aimbot Rage" };
        private readonly string[] _comboItems = { "Top", "Center", "Bottom" };
        /// <summary>Labels match <see cref="EspNameBoxAnchor"/> order.</summary>
        private readonly string[] _espEditorNameItems =
        {
            "Top of box",
            "Bottom of box",
            "Top left",
            "Top right",
            "Bottom left",
            "Bottom right",
            "Middle left",
            "Middle right",
        };
        /// <summary>Labels match <see cref="EspDistanceBoxAnchor"/> order.</summary>
        private readonly string[] _espEditorDistanceItems =
        {
            "Inside nameplate",
            "Above name (when name at top)",
            "Top of box",
            "Bottom of box",
            "Top left",
            "Top right",
            "Bottom left",
            "Bottom right",
            "Middle left",
            "Middle right",
        };
        private readonly string[] _espEditorHealthItems =
        {
            "Right side",
            "Left side",
            "Top of box",
            "Bottom of box",
        };
        private readonly string[] _espEditorWeaponItems =
        {
            "Top of box",
            "Bottom of box",
            "Top left",
            "Top right",
            "Bottom left",
            "Bottom right",
            "Middle left",
            "Middle right",
        };
        private float _espEditorHealthSectionAnim;
        private float _visLineComboAnim;
        private float _visInfoComboAnim;
        private float _visNameComboAnim;
        private float _visWeaponComboAnim;
        private readonly string[] _headerItems = { "Aim", "Esp", "Colors", "Extras", "Misc" };
        private int _selectedHeader, _comboBox, _comboBox1, _comboBox2;
        private struct EntityRenderData
        {
            public Vector2 headScreenPos;
            public Vector2 bottomScreenPos;
            public float Distance;
            public bool IsValid;
        }
        private string CamReplacePattern => MakeAob(cameraVal);
        private string VisionReplacePattern => MakeAob(visionVal);
        private async void CameraOn()
        {
            string[] processname = { "HD-Player" };
            bool success = memoryfast.SetProcess(processname);
            if (!success)
            {
                return;
            }
            if (cameraResult == null)
            {
                cameraResult = await memoryfast.AoBScan("00 00 80 3F 00 00 00 00 00 00 00 00 00 00 80 BF 00 00 00 00 00 00 80 BF 00 00 00 00 00 00 00 00 00 00 80 3F 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 80 3F 00 00 00 00 00 00 00 00 00 00 80 BF 00 00 80 7F 00 00 80 7F 00 00 80 7F 00 00 80 FF");
                Console.Beep(2000, 600);
            }
            if (cameraResult != null && cameraResult.Any())
            {
                foreach (long id in cameraResult)
                {
                    memoryfast.AobReplace(id, CamReplacePattern);
                }
            }
        }
        private async void CameraOff()
        {
            foreach (long id in cameraResult)
            {
                memoryfast.AobReplace(id, "00 00 80 3F");
            }


        }

        private async void VisionOn()
        {
            string[] processname = { "HD-Player" };
            bool success = memoryfast.SetProcess(processname);
            if (!success)
            {
                return;
            }
            if (visionResult == null)
            {
                visionResult = await memoryfast.AoBScan("DB 0F 49 40 10 2A 00 EE 00 10 80 E5 10 3A 01 EE 14 10 80 E5");
                Console.Beep(2000, 600);
            }
            if (visionResult != null && visionResult.Any())
            {
                foreach (long id in visionResult)
                {
                    memoryfast.AobReplace(id, VisionReplacePattern);
                }
            }
        }
        private async void VisionOff()
        {
            foreach (long id in visionResult)
            {
                memoryfast.AobReplace(id, "DB 0F 49 40 10 2A 00 EE 00 10 80 E5 10 3A 01 EE 14 10 80 E5");
            }


        }

        static string MakeAob(float value)
        {
            byte[] bytes = BitConverter.GetBytes(value);
            return BitConverter.ToString(bytes).Replace("-", " ");
        }

        Vector4 HsvToRgb(float h, float s, float v)
        {
            int i = (int)(h * 6);
            float f = h * 6 - i;
            float p = v * (1 - s);
            float q = v * (1 - f * s);
            float t = v * (1 - (1 - f) * s);

            float r = 0, g = 0, b = 0;
            switch (i % 6)
            {
                case 0: r = v; g = t; b = p; break;
                case 1: r = q; g = v; b = p; break;
                case 2: r = p; g = v; b = t; break;
                case 3: r = p; g = q; b = v; break;
                case 4: r = t; g = p; b = v; break;
                case 5: r = v; g = p; b = q; break;
            }

            return new Vector4(r, g, b, 1f);
        }
        private void RenderEntities()
        {
            if (!Core.HaveMatrix)
                return;

            var drawList = ImGui.GetBackgroundDrawList();
            bool anyEnemySeenThisFrame = false;
            const float espEdgePad = 72f;

            // Update random hacker entity for tag (only one enemy marked)
            if (Config.HackerTag)
            {
                if ((DateTime.Now - _lastHackerUpdate).TotalSeconds > HackerUpdateIntervalSeconds || _hackerEntityAddress == 0)
                {
                    var valid = Core.Entities.Values
                        .Where(e => !e.IsDead && e.IsKnown)
                        .ToList();

                    if (valid.Count > 0)
                    {
                        _hackerEntityAddress = valid[_hackerRandom.Next(valid.Count)].Address;
                    }
                    else
                    {
                        _hackerEntityAddress = 0;
                    }
                    _lastHackerUpdate = DateTime.Now;
                }
            }

            foreach (var entity in Core.Entities.Values)
            {
                if (entity.Address == 0) continue;
                if (entity.IsDead || !entity.IsKnown) continue;

                float distW = Vector3.Distance(Core.LocalMainCamera, entity.Head);
                float espRenderMax = Math.Clamp(Config.Esprender, 0f, 100f);
                if (distW > espRenderMax) continue;

                Vector2 headScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.Head, Core.Width, Core.Height);
                Vector2 bottomScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.Root, Core.Width, Core.Height);
                if (headScreenPos.X < 0f && headScreenPos.Y < 0f) continue;
                if (bottomScreenPos.X < 0f && bottomScreenPos.Y < 0f) continue;

                // Shift core screen coords (0..Core.Width/Height) into overlay coords (virtual desktop origin).
                headScreenPos += _emulatorScreenOffset;
                bottomScreenPos += _emulatorScreenOffset;

                var entityData = new EntityRenderData
                {
                    headScreenPos = headScreenPos,
                    bottomScreenPos = bottomScreenPos,
                    Distance = distW,
                    IsValid = true
                };

                float CornerHeight = Math.Abs(entityData.headScreenPos.Y - entityData.bottomScreenPos.Y);
                float CornerWidth = CornerHeight * 0.65f;

                anyEnemySeenThisFrame = true;
                Config.LastEnemySeen = DateTime.Now;
                if (!Config.EnemyTimerActive)
                {
                    Config.EnemyTimerActive = true;
                    Config.EnemyTimerStart = DateTime.Now;
                }

                // Dynamic ESP colors (RGB mode)
                uint lineColor = ColorToUint32(Config.ESPLineColor);
                uint boxColor = ColorToUint32(Config.ESPBoxColor);
                if (Config.ESPRGB)
                {
                    Color rainbow = ColorFromHSV((float)(ImGui.GetTime() * 100 % 360), 1.0f, 1.0f);
                    lineColor = ColorToUint32(rainbow);
                    boxColor = lineColor;
                }

                Vector2 offLineStart = ScaleEspOffsetPx(Config.EspSnapLineStartOffsetPx, CornerHeight);
                Vector2 offLineEnd = ScaleEspOffsetPx(Config.EspSnapLineEndOffsetPx, CornerHeight);
                Vector2 offBox = ScaleEspOffsetPx(Config.EspBoxOffsetPx, CornerHeight);
                Vector2 offName = ScaleEspOffsetPx(Config.EspNameOffsetPx, CornerHeight);
                Vector2 offHacker = ScaleEspOffsetPx(Config.EspHackerOffsetPx, CornerHeight);
                Vector2 offWeapon = ScaleEspOffsetPx(Config.EspWeaponIconOffsetPx, CornerHeight);
                Vector2 offHealth = ScaleEspOffsetPx(Config.EspHealthBarOffsetPx, CornerHeight);
                Vector2 offDist = ScaleEspOffsetPx(Config.EspDistanceOffsetPx, CornerHeight);

                if (string.IsNullOrEmpty(entity.Name))
                    entity.Name = "Training Bot";

                float distM = MathF.Round(Vector3.Distance(Core.LocalMainCamera, entity.Head));
                string distStr = $"{distM}m";
                string nameDisplay = SanitizeEspDisplayName(entity.Name);

                float healthPercentage = entity.Health > 1000 ? 1f :
                    entity.Health < 0 ? 1f :
                    (float)entity.Health / (entity.Health > 230 ? 500 : 200);
                healthPercentage = Math.Clamp(healthPercentage, 0f, 1f);

                GetEspBoxBounds(entityData.headScreenPos, CornerWidth, CornerHeight, offBox,
                    out float boxL, out float boxT, out float boxR, out float boxB, out float boxMidX, out float boxMidY);
                float gapScaled = Math.Max(3f, EspAnchorGapPx * (CornerHeight / EspOffsetRefCornerHeight));

                float npW = 0f;
                float npH = 0f;
                Vector2 npTL = Vector2.Zero;
                bool showDist = Config.espdistance;
                EspDistanceBoxAnchor distAnchor = Config.EspDistanceAnchor;
                if (!Config.ESPInformation &&
                    (distAnchor == EspDistanceBoxAnchor.InsideNameplate || distAnchor == EspDistanceBoxAnchor.AboveNameWhenNameAtTop))
                {
                    distAnchor = EspDistanceBoxAnchor.BoxTopCenter;
                }
                bool distInNameplate = showDist && Config.ESPInformation && distAnchor == EspDistanceBoxAnchor.InsideNameplate;
                if (Config.ESPInformation)
                {
                    if (distInNameplate)
                        ComputeEspNameplateSize(nameDisplay, distStr, out npW, out npH);
                    else
                        ComputeEspNameplateSizeNameOnly(nameDisplay, out npW, out npH);
                    npTL = GetNameBoxAnchorTopLeft(Config.EspNameAnchor, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled, npW, npH) + offName;
                }

                if (Config.ESPLine)
                {
                    Vector2 endPos = entityData.headScreenPos + offLineEnd;
                    Vector2 lineStart = new Vector2(Core.Width / 2f, 10f);
                    switch (Config.ESPLinePosition)
                    {
                        case LinePosition.Top:
                            lineStart = new Vector2(Core.Width / 2f, 10f);
                            endPos = entityData.headScreenPos + offLineEnd;
                            break;
                        case LinePosition.Center:
                            lineStart = new Vector2(Core.Width / 2f, Core.Height / 2f);
                            endPos = entityData.headScreenPos + offLineEnd;
                            break;
                        case LinePosition.Bottom:
                            lineStart = new Vector2(Core.Width / 2f, Core.Height - 10f);
                            endPos = entityData.bottomScreenPos + offLineEnd;
                            break;
                    }
                    lineStart += offLineStart + _emulatorScreenOffset;

                    drawList.AddLine(lineStart, endPos, lineColor, 1f);
                }

                if (Config.ESPBox)
                {
                    DrawCorneredBox(entityData.headScreenPos.X - (CornerWidth / 2) + offBox.X, entityData.headScreenPos.Y + offBox.Y, CornerWidth, CornerHeight, boxColor, 1f);
                }

                Vector2 nameAnchorSize = Config.ESPName && !Config.ESPInformation
                    ? EspCalcTextSizeName(nameDisplay)
                    : new Vector2(60, 14);
                Vector2 namePos = GetNameBoxAnchorTopLeft(Config.EspNameAnchor, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled, nameAnchorSize.X, nameAnchorSize.Y) + offName;

                // Clipping check must use core coords (origin at emulator), not overlay coords.
                Vector2 headCore = entityData.headScreenPos - _emulatorScreenOffset;
                if (headCore.X >= -espEdgePad && headCore.Y >= -espEdgePad &&
                    headCore.X <= Core.Width + espEdgePad && headCore.Y <= Core.Height + espEdgePad)
                {
                    if (Config.ESPInformation)
                    {
                        var fg = ImGui.GetForegroundDrawList();
                        int teamDigit = (int)(entity.Address % 10);
                        DrawEspInfoNameplate(fg, npTL, npW, nameDisplay, distStr, teamDigit, healthPercentage, entity.IsKnocked, showDist && distInNameplate);
                        if (showDist && !distInNameplate)
                        {
                            Vector2 distSz = ImGui.CalcTextSize(distStr);
                            Vector2 distTL = GetDistanceBoxAnchorTopLeft(distAnchor, Config.EspNameAnchor, distSz, npTL, npW, npH, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled) + offDist;
                            fg.AddText(distTL, ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 0.93f, 0.2f, 1f)), distStr);
                        }
                    }
                    else if (Config.ESPName)
                    {
                        var fg = ImGui.GetForegroundDrawList();
                        Vector2 nameSz = EspCalcTextSizeName(nameDisplay);
                        Vector2 simpleNamePos = GetNameBoxAnchorTopLeft(Config.EspNameAnchor, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled, nameSz.X, nameSz.Y) + offName;
                        if (TextFont.IsLoaded()) ImGui.PushFont(TextFont);
                        fg.AddText(simpleNamePos, ColorToUint32(Config.ESPNameColor), nameDisplay);
                        if (TextFont.IsLoaded()) ImGui.PopFont();
                    }

                    // Distance can render independently of ESP Information.
                    if (showDist && !Config.ESPInformation)
                    {
                        Vector2 distSz = ImGui.CalcTextSize(distStr);
                        Vector2 distTL = GetDistanceBoxAnchorTopLeft(distAnchor, Config.EspNameAnchor, distSz, npTL, npW, npH, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled) + offDist;
                        ImGui.GetForegroundDrawList().AddText(distTL, ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 0.93f, 0.2f, 1f)), distStr);
                    }

                    if (Config.HackerTag && entity.Address == _hackerEntityAddress)
                    {
                        string hackerText = "HACKER";
                        Vector2 hackerSize = ImGui.CalcTextSize(hackerText);
                        float anchorW = Config.ESPInformation ? npW : nameAnchorSize.X;
                        float anchorX = Config.ESPInformation ? npTL.X : namePos.X;
                        float anchorTopY = Config.ESPInformation ? npTL.Y : namePos.Y;
                        Vector2 hackerPos = new Vector2(
                            anchorX + (anchorW - hackerSize.X) * 0.5f,
                            anchorTopY - hackerSize.Y - 3f);
                        hackerPos += offHacker;

                        uint hackerCol = ColorToUint32(Config.HackerTagColor);
                        var fg = ImGui.GetForegroundDrawList();
                        fg.AddText(hackerPos + new Vector2(1, 1), ColorToUint32(Color.Black), hackerText);
                        fg.AddText(hackerPos, hackerCol, hackerText);
                    }


                    if (Config.espweapon)
                    {
                        if (entityData.headScreenPos.X >= -espEdgePad && entityData.headScreenPos.Y >= -espEdgePad && entityData.headScreenPos.X <= Core.Width + espEdgePad && entityData.headScreenPos.Y <= Core.Height + espEdgePad)
                        {
                            string weaponFileName = $"{entity.WeaponName?.ToLower()}.png";
                            string resourceName = $"pixel.Resources.Icons.{weaponFileName}";

                            try
                            {
                                if (!_weaponIcons.TryGetValue(weaponFileName, out var imagehandle))
                                {
                                    using (Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream(resourceName))
                                    {
                                        if (stream != null)
                                        {
                                            using (var bitmap = new Bitmap(stream))
                                            {
                                                string tempPath = Path.Combine(Path.GetTempPath(), $"{Guid.NewGuid()}.png");
                                                bitmap.Save(tempPath);

                                                AddOrGetImagePointer(tempPath, true, out imagehandle, out var width, out var height);

                                                _weaponIcons[weaponFileName] = imagehandle;

                                                try { File.Delete(tempPath); } catch { }
                                            }
                                        }
                                    }
                                }


                                if (imagehandle != IntPtr.Zero)
                                {
                                    Vector2 iconSize = new Vector2(60 * Config.iconsize, 20 * Config.iconsize);
                                    EspNameBoxAnchor wMap = MapWeaponAnchorToNameAnchor(Config.EspWeaponAnchor);
                                    Vector2 iconPos = GetNameBoxAnchorTopLeft(wMap, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled, iconSize.X, iconSize.Y);
                                    iconPos += offWeapon;
                                    if (!Config.rgb)
                                    {
                                        ImGui.GetForegroundDrawList().AddImage(imagehandle, iconPos, iconPos + iconSize, Vector2.Zero, Vector2.One, ImGui.ColorConvertFloat4ToU32(Config.ICONCOLOR));
                                    }
                                    else
                                    {
                                        const int slices = 80;
                                        float time = (float)ImGui.GetTime();

                                        float sliceWidth = iconSize.X / slices;

                                        for (int i = 0; i < slices; i++)
                                        {
                                            float xStart = iconPos.X + i * sliceWidth;
                                            float xEnd = xStart + sliceWidth;


                                            float uvStart = (float)i / slices;
                                            float uvEnd = (float)(i + 1) / slices;


                                            float hue = ((float)i / slices + time * 0.25f) % 1.0f;

                                            Vector4 rgbColor = HsvToRgb(hue, 1f, 1f);
                                            rgbColor.W = Config.ICONCOLOR.W;

                                            ImGui.GetForegroundDrawList().AddImage(
                                                imagehandle,
                                                new Vector2(xStart, iconPos.Y),
                                                new Vector2(xEnd, iconPos.Y + iconSize.Y),
                                                new Vector2(uvStart, 0),
                                                new Vector2(uvEnd, 1),
                                                ImGui.ColorConvertFloat4ToU32(rgbColor)
                                            );
                                        }
                                    }

                                }
                            }
                            catch (Exception ex)
                            {
                                Console.WriteLine($"Error drawing weapon icon: {ex.Message}");
                            }
                        }

                    }
                    var vList = ImGui.GetForegroundDrawList();
                    if (Config.ESPHealth && !Config.ESPInformation)
                    {
                        float t = 3f;
                        uint healthBarColor;
                        if (entity.IsKnocked)
                            healthBarColor = ColorToUint32(Color.Red);
                        else if (healthPercentage > 0.8f)
                            healthBarColor = ColorToUint32(Color.Lime);
                        else if (healthPercentage > 0.4f)
                            healthBarColor = ColorToUint32(Color.Yellow);
                        else
                            healthBarColor = ColorToUint32(Color.Red);

                        switch (Config.EspHealthAnchor)
                        {
                            case EspHealthBoxAnchor.BoxTop:
                                {
                                    float y0 = boxT - gapScaled - t;
                                    Vector2 o = offHealth;
                                    vList.AddRect(new Vector2(boxL + o.X - 1, y0 + o.Y - 1), new Vector2(boxR + o.X + 1, y0 + t + o.Y + 1), 0xFF000000, 1.0f);
                                    vList.AddRectFilled(new Vector2(boxL + o.X, y0 + o.Y), new Vector2(boxR + o.X, y0 + t + o.Y), 0x90000000, 1.0f);
                                    float fillW = CornerWidth * healthPercentage;
                                    vList.AddRectFilled(new Vector2(boxL + o.X, y0 + o.Y), new Vector2(boxL + fillW + o.X, y0 + t + o.Y), healthBarColor, 1.0f);
                                    break;
                                }
                            case EspHealthBoxAnchor.BoxBottom:
                                {
                                    float y0 = boxB + gapScaled;
                                    Vector2 o = offHealth;
                                    vList.AddRect(new Vector2(boxL + o.X - 1, y0 + o.Y - 1), new Vector2(boxR + o.X + 1, y0 + t + o.Y + 1), 0xFF000000, 1.0f);
                                    vList.AddRectFilled(new Vector2(boxL + o.X, y0 + o.Y), new Vector2(boxR + o.X, y0 + t + o.Y), 0x90000000, 1.0f);
                                    float fillW = CornerWidth * healthPercentage;
                                    vList.AddRectFilled(new Vector2(boxL + o.X, y0 + o.Y), new Vector2(boxL + fillW + o.X, y0 + t + o.Y), healthBarColor, 1.0f);
                                    break;
                                }
                            case EspHealthBoxAnchor.BoxLeft:
                                {
                                    Vector2 p = new Vector2(boxL - 5f - t, boxT) + offHealth;
                                    vList.AddRect(new Vector2(p.X - 1, p.Y - 1), new Vector2(p.X + t + 1, p.Y + CornerHeight + 1), 0xFF000000, 1.0f);
                                    vList.AddRectFilled(p, new Vector2(p.X + t, p.Y + CornerHeight), 0x90000000, 1.0f);
                                    vList.AddRectFilled(new Vector2(p.X, p.Y + CornerHeight * (1 - healthPercentage)), new Vector2(p.X + t, p.Y + CornerHeight), healthBarColor, 1.0f);
                                    break;
                                }
                            default:
                                {
                                    Vector2 healthBarPosRight = new Vector2(boxR + 5f, boxT) + offHealth;
                                    float healthBarWidth = t;
                                    float healthBarFullHeight = CornerHeight;
                                    vList.AddRect(new Vector2(healthBarPosRight.X - 1, healthBarPosRight.Y - 1),
                                        new Vector2(healthBarPosRight.X + healthBarWidth + 1, healthBarPosRight.Y + healthBarFullHeight + 1),
                                        0xFF000000, 1.0f);
                                    vList.AddRectFilled(new Vector2(healthBarPosRight.X, healthBarPosRight.Y),
                                        new Vector2(healthBarPosRight.X + healthBarWidth, healthBarPosRight.Y + healthBarFullHeight),
                                        0x90000000, 1.0f);
                                    vList.AddRectFilled(new Vector2(healthBarPosRight.X, healthBarPosRight.Y + healthBarFullHeight * (1 - healthPercentage)),
                                        new Vector2(healthBarPosRight.X + healthBarWidth, healthBarPosRight.Y + healthBarFullHeight),
                                        healthBarColor, 1.0f);
                                    break;
                                }
                        }
                    }
                    if (Config.ESPHealthText)
                    {
                        if (!entity.IsKnocked)
                        {
                            DrawHealthText(entity, entity.Health);
                        }

                    }

                }



                if (Config.ESPSkeleton)
                {
                    DrawSkeleton(entity);
                }

            }

            // ===== ESP TIMER (InValid Timer / END THIS GAME) =====
            if (Config.EspTimer)
            {
                var drawListTimer = ImGui.GetForegroundDrawList();
                int windowWidth = Core.Width;

                if (!anyEnemySeenThisFrame && Config.EnemyTimerActive)
                {
                    if ((DateTime.Now - Config.LastEnemySeen).TotalSeconds > Config.ResetAfterNoEnemy)
                        Config.EnemyTimerActive = false;
                }

                if (Config.EnemyTimerActive)
                {
                    int elapsed = (int)(DateTime.Now - Config.EnemyTimerStart).TotalSeconds;
                    int remaining = Config.EnemyTimerDuration - elapsed;
                    string displayText = remaining > 0
                        ? $"InValid Timer {remaining}s"
                        : " END THIS GAME "; // timer logic

                    Vector2 textSizeTimer = ImGui.CalcTextSize(displayText);
                    float x = (windowWidth - textSizeTimer.X) / 2f;
                    float y = 100f;

                    float rainbowSpeed = 2f;
                    float hue = (float)(DateTime.Now.TimeOfDay.TotalSeconds * rainbowSpeed % 360);
                    Color col = ColorFromHSV(hue, 1f, 1f);
                    uint textColor1 = ColorToUint32(col);

                    drawListTimer.AddText(new Vector2(x + 1, y + 1),
                        ImGui.ColorConvertFloat4ToU32(new Vector4(0, 0, 0, 0.7f)),
                        displayText);

                    drawListTimer.AddText(new Vector2(x, y), textColor1, displayText);
                }
            }
        }

        private void UpdateEntities()
        {

            foreach (var entity in Core.Entities.Values)
            {
                if (entity.IsTeam != Bool3.False) continue;

                TreeNode entityNode = new TreeNode(entity.Name);

                entityNode.Nodes.Add(new TreeNode($"IsKnown: {entity.IsKnown}"));
                entityNode.Nodes.Add(new TreeNode($"IsTeam: {entity.IsTeam}"));
                entityNode.Nodes.Add(new TreeNode($"Head: {entity.Head}"));
                entityNode.Nodes.Add(new TreeNode($"Root: {entity.Root}"));
                entityNode.Nodes.Add(new TreeNode($"Health: {entity.Health}"));
                entityNode.Nodes.Add(new TreeNode($"IsDead: {entity.IsDead}"));
                entityNode.Nodes.Add(new TreeNode($"IsKnocked: {entity.IsKnocked}"));


            }
            Thread.Sleep(1000);
        }
        private void NoCache()
        {

            InternalMemory.Cache = new();
            Core.Entities = new();
            Thread.Sleep(1000);
        }

        private void HandleUIAnimation()
        {
            bool imguiInsert = ImGui.IsKeyPressed(ImGuiKey.Insert);
            bool vkDown = (GetAsyncKeyState(System.Windows.Forms.Keys.Insert) & 0x8000) != 0;
            bool vkEdge = vkDown && !_vkInsertDownLast;
            _vkInsertDownLast = vkDown;

            bool insertPressed = imguiInsert || vkEdge;
            if (_flowPhase == FlowPhase.Main && insertPressed && !_insertKeyPressedLastFrame)
            {
                _uiVisible = !_uiVisible;
                if (!_uiVisible)
                {
                    _showTabPopup = false;
                    _tabPopupAnim = 0f;
                }
            }
            _insertKeyPressedLastFrame = insertPressed;

            float targetAlpha = _uiVisible ? 1.0f : 0.0f;
            float dt = ImGui.GetIO().DeltaTime;
            float fadeRate = _uiVisible ? _uiFadeSpeed : _uiFadeOutSpeed;
            _uiAlpha += (targetAlpha - _uiAlpha) * dt * fadeRate;
            _uiAlpha = Math.Clamp(_uiAlpha, 0.0f, 1.0f);
            if (!_uiVisible && _uiAlpha < 0.02f)
                _uiAlpha = 0f;
        }

        protected override unsafe void Render()
        {
            EnsureTabIconsLoaded();
            if (Core.Handle != IntPtr.Zero)
                CreateHandle();
            HandleUIAnimation();

            if (_loginDone)
            {
                _loginDone = false;
                if (_loginOk)
                {
                    _loginError = "";
                    _flowPhase = FlowPhase.PostLoginLoad;
                    ResetLoaderVisualState();
                    _emulatorInitTask = BootstrapRuntime.InitEmulatorAsync();
                }
                else
                    _loginError = _loginResultMsg;
            }

            if (_uiVisible || _uiAlpha > 0.001f)
                RenderUI();

            if (_flowPhase == FlowPhase.Main)
                CustomNotification.RenderNotifications();

            if (!Core.HaveMatrix) return;



            var espClipDl = ImGui.GetBackgroundDrawList();
            espClipDl.PushClipRect(_emulatorClipMin, _emulatorClipMax, true);
            try
            {
                if (Config.FOVEnabled)
                {
                    DrawFOVCircle(Config.AimFov);
                }

                // Aim Track Line: draw from screen center to nearest enemy inside FOV
                if (Config.AimTrackLine)
                {
                    Vector2 screenCenter = new Vector2(Core.Width / 2f, Core.Height / 2f);
                    Entity nearestEnemy = null;
                    Vector2 nearestEnemyScreenPos = Vector2.Zero;
                    float nearestDistance = float.MaxValue;

                    foreach (var entity in Core.Entities.Values)
                    {
                        if (entity.IsDead || !entity.IsKnown) continue;

                        var headPos = W2S.WorldToScreen(Core.CameraMatrix, entity.Head, Core.Width, Core.Height);
                        if (headPos.X < 1 || headPos.Y < 1) continue;

                        float distToCrosshair = Vector2.Distance(screenCenter, headPos);
                        if (distToCrosshair < nearestDistance &&
                            (!Config.FOVEnabled || distToCrosshair <= Config.AimFov))
                        {
                            nearestDistance = distToCrosshair;
                            nearestEnemy = entity;
                            nearestEnemyScreenPos = headPos;
                        }
                    }

                    if (nearestEnemy != null)
                    {
                        uint trackLineColor = ColorToUint32(Config.AimTrackLineColor);
                        DrawGlowLine(screenCenter + _emulatorScreenOffset, nearestEnemyScreenPos + _emulatorScreenOffset, trackLineColor, 0.8f, 4.0f, 1.0f, 0.6f);
                    }
                }
            }
            finally
            {
                espClipDl.PopClipRect();
            }

            string windowName = "Overlay";
            hWnd = FindWindow(null!, windowName);
            HDPlayer = FindWindow("BlueStacksApp", null!);

            if (hWnd != IntPtr.Zero)
            {
                long extendedStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
                SetWindowLong(hWnd, GWL_EXSTYLE, (extendedStyle | WS_EX_TOOLWINDOW) & ~WS_EX_APPWINDOW);
            }
            else
            {

            }
            var espEntityClipDl = ImGui.GetBackgroundDrawList();
            espEntityClipDl.PushClipRect(_emulatorClipMin, _emulatorClipMax, true);
            try
            {
                RenderEntities();
            }
            finally
            {
                espEntityClipDl.PopClipRect();
            }

        }

        // Glowing line helper (for Aim Track Line)
        private void DrawGlowLine(Vector2 start, Vector2 end, uint color, float thickness, float glowRadius, float feather, float glowOpacityMultiplier)
        {
            var drawList = ImGui.GetBackgroundDrawList();
            Vector4 colorVec = ImGui.ColorConvertU32ToFloat4(color);

            for (float i = glowRadius; i > 0; i -= feather)
            {
                float alpha = colorVec.W * (i / glowRadius) * glowOpacityMultiplier;
                alpha = Math.Clamp(alpha, 0, 1);
                uint glowColor = ImGui.ColorConvertFloat4ToU32(new Vector4(colorVec.X, colorVec.Y, colorVec.Z, alpha));
                drawList.AddLine(start, end, glowColor, thickness + (glowRadius - i) * 0.5f);
            }

            drawList.AddLine(start, end, color, thickness);
        }
        private void DrawLine(ImDrawListPtr drawList, Vector2 startPos, Vector2 endPos, uint color)
        {
            if (startPos.X > 0 && startPos.Y > 0 && endPos.X > 0 && endPos.Y > 0)
            {
                drawList.AddLine(startPos, endPos, color, 1.5f); // Adjust thickness as needed
            }
        }
        private void DrawSkeleton(Entity entity)
        {
            var drawList = ImGui.GetBackgroundDrawList();
            uint lineColor = ColorToUint32(Config.ESPSkeletonColor);
            uint circleColor = ColorToUint32(Color.White);

            var headScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.Head, Core.Width, Core.Height);
            Vector2 headCorePos = headScreenPos;
            var leftWristScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.RightWrist, Core.Width, Core.Height);
            var spineScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.Spine, Core.Width, Core.Height);
            var hipScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.Hip, Core.Width, Core.Height);
            var rootScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.Root, Core.Width, Core.Height);
            var rightCalfScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.RightCalf, Core.Width, Core.Height);
            var leftCalfScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.LeftCalf, Core.Width, Core.Height);
            var rightFootScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.RightFoot, Core.Width, Core.Height);
            var leftFootScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.LeftFoot, Core.Width, Core.Height);
            var rightWristScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.RightWrist, Core.Width, Core.Height);
            var leftHandScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.LeftHand, Core.Width, Core.Height);
            var leftShoulderScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.LeftShoulder, Core.Width, Core.Height);
            var rightShoulderScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.RightShoulder, Core.Width, Core.Height);
            var rightWristJointScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.RightWristJoint, Core.Width, Core.Height);
            var leftWristJointScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.LeftWristJoint, Core.Width, Core.Height);
            var leftElbowScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.LeftElbow, Core.Width, Core.Height);
            var rightElbowScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.RightElbow, Core.Width, Core.Height);

            // If head is invalid, don't draw the whole skeleton.
            if (headCorePos.X <= 0f || headCorePos.Y <= 0f)
                return;

            var o = _emulatorScreenOffset;
            headScreenPos += o;
            leftWristScreenPos += o;
            spineScreenPos += o;
            hipScreenPos += o;
            rootScreenPos += o;
            rightCalfScreenPos += o;
            leftCalfScreenPos += o;
            rightFootScreenPos += o;
            leftFootScreenPos += o;
            rightWristScreenPos += o;
            leftHandScreenPos += o;
            leftShoulderScreenPos += o;
            rightShoulderScreenPos += o;
            rightWristJointScreenPos += o;
            leftWristJointScreenPos += o;
            leftElbowScreenPos += o;
            rightElbowScreenPos += o;

            DrawLine(drawList, spineScreenPos, rightShoulderScreenPos, lineColor);
            DrawLine(drawList, spineScreenPos, hipScreenPos, lineColor);
            DrawLine(drawList, spineScreenPos, leftShoulderScreenPos, lineColor);
            DrawLine(drawList, leftShoulderScreenPos, rightElbowScreenPos, lineColor);
            DrawLine(drawList, rightShoulderScreenPos, leftElbowScreenPos, lineColor);
            DrawLine(drawList, leftElbowScreenPos, rightWristJointScreenPos, lineColor);
            DrawLine(drawList, rightElbowScreenPos, leftWristJointScreenPos, lineColor);
            DrawLine(drawList, hipScreenPos, rightFootScreenPos, lineColor);
            DrawLine(drawList, hipScreenPos, leftFootScreenPos, lineColor);
            float distance = entity.Distance;
            float baseRadius = 50.0f;
            float circleRadius = baseRadius / distance;
            drawList.AddCircle(headScreenPos, circleRadius, circleColor, 30);
        }

        private static uint[] HealthColors = new uint[]
{
    ColorToUint32(Color.Green),
    ColorToUint32(Color.Yellow),
    ColorToUint32(Color.Red)
};


        public void DrawHealthText(Entity entity, int espHealthInt)
        {

            //Vector2 headScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.Head, Core.Width, Core.Height);
            //Vector2 pieDerechoScreenPos = W2S.WorldToScreen(Core.CameraMatrix, entity.RightFoot, Core.Width, Core.Height);


            //string healthText = "Health: " + espHealthInt.ToString();


            //Vector2 textPosition = new Vector2(.X - 25, pieDerechoScreenPos.Y + 10);


            //var drawList = ImGui.GetBackgroundDrawList();


            //uint borderColor = ColorToUint32(Color.Black);


            //float offset = 1.0f;

            //drawList.AddText(new Vector2(textPosition.X - offset, textPosition.Y), borderColor, healthText);
            //drawList.AddText(new Vector2(textPosition.X + offset, textPosition.Y), borderColor, healthText);
            //drawList.AddText(new Vector2(textPosition.X, textPosition.Y - offset), borderColor, healthText);
            //drawList.AddText(new Vector2(textPosition.X, textPosition.Y + offset), borderColor, healthText);


            //drawList.AddText(textPosition, ColorToUint32(Color.White), healthText);
        }

        //public void DrawGlowingBall(Vector2 position, Color color, float radius)
        //{
        //    var drawList = ImGui.GetBackgroundDrawList();
        //    uint ballColor = ColorToUint32(color);


        //    for (int i = 0; i < 5; i++)
        //    {
        //        float glowRadius = radius + (i * 2); 
        //        float alpha = 1.0f - (i * 0.2f);    

        //        drawList.AddCircleFilled(
        //            position,
        //            glowRadius,
        //            ImGui.ColorConvertFloat4ToU32(new Vector4(color.R / 255f, color.G / 255f, color.B / 255f, alpha)),
        //            50 
        //        );
        //    }


        //    drawList.AddCircleFilled(position, radius, ballColor, 50);
        //}

        public void DrawCorneredBox(float X, float Y, float W, float H, uint color, float thickness)
        {
            var vList = ImGui.GetBackgroundDrawList();
            float lineW = W / 3;
            float lineH = H / 3;

            vList.AddLine(new Vector2(X, Y - thickness / 2), new Vector2(X, Y + lineH), color, thickness);
            vList.AddLine(new Vector2(X - thickness / 2, Y), new Vector2(X + lineW, Y), color, thickness);
            vList.AddLine(new Vector2(X + W - lineW, Y), new Vector2(X + W + thickness / 2, Y), color, thickness);
            vList.AddLine(new Vector2(X + W, Y - thickness / 2), new Vector2(X + W, Y + lineH), color, thickness);
            vList.AddLine(new Vector2(X, Y + H - lineH), new Vector2(X, Y + H + thickness / 2), color, thickness);
            vList.AddLine(new Vector2(X - thickness / 2, Y + H), new Vector2(X + lineW, Y + H), color, thickness);
            vList.AddLine(new Vector2(X + W - lineW, Y + H), new Vector2(X + W + thickness / 2, Y + H), color, thickness);
            vList.AddLine(new Vector2(X + W, Y + H - lineH), new Vector2(X + W, Y + H + thickness / 2), color, thickness);
        }

        private static void DrawCorneredBoxDrawList(ImDrawListPtr dl, float X, float Y, float W, float H, uint color, float thickness)
        {
            float lineW = W / 3;
            float lineH = H / 3;
            dl.AddLine(new Vector2(X, Y - thickness / 2), new Vector2(X, Y + lineH), color, thickness);
            dl.AddLine(new Vector2(X - thickness / 2, Y), new Vector2(X + lineW, Y), color, thickness);
            dl.AddLine(new Vector2(X + W - lineW, Y), new Vector2(X + W + thickness / 2, Y), color, thickness);
            dl.AddLine(new Vector2(X + W, Y - thickness / 2), new Vector2(X + W, Y + lineH), color, thickness);
            dl.AddLine(new Vector2(X, Y + H - lineH), new Vector2(X, Y + H + thickness / 2), color, thickness);
            dl.AddLine(new Vector2(X - thickness / 2, Y + H), new Vector2(X + lineW, Y + H), color, thickness);
            dl.AddLine(new Vector2(X + W - lineW, Y + H), new Vector2(X + W + thickness / 2, Y + H), color, thickness);
            dl.AddLine(new Vector2(X + W, Y + H - lineH), new Vector2(X + W, Y + H + thickness / 2), color, thickness);
        }

        private const float EspOffsetRefCornerHeight = 200f;

        private static Vector2 ScaleEspOffsetPx(Vector2 editorPx, float cornerHeight)
        {
            float refH = Math.Max(1f, EspOffsetRefCornerHeight);
            float s = cornerHeight / refH;
            return editorPx * s;
        }

        private const float EspAnchorGapPx = 5f;

        private static bool IsNameAnchorTopRow(EspNameBoxAnchor a) =>
            a == EspNameBoxAnchor.BoxTopCenter || a == EspNameBoxAnchor.BoxTopLeft || a == EspNameBoxAnchor.BoxTopRight;

        private static void GetEspBoxBounds(Vector2 headScreen, float cornerWidth, float cornerHeight, Vector2 offBox,
            out float bl, out float bt, out float br, out float bb, out float midX, out float midY)
        {
            bl = headScreen.X - cornerWidth * 0.5f + offBox.X;
            bt = headScreen.Y + offBox.Y;
            br = bl + cornerWidth;
            bb = bt + cornerHeight;
            midX = bl + cornerWidth * 0.5f;
            midY = bt + cornerHeight * 0.5f;
        }

        private static Vector2 GetNameBoxAnchorTopLeft(EspNameBoxAnchor anchor, float bl, float bt, float br, float bb, float midX, float midY, float gap, float w, float h)
        {
            return anchor switch
            {
                EspNameBoxAnchor.BoxTopCenter => new Vector2(midX - w * 0.5f, bt - gap - h),
                EspNameBoxAnchor.BoxBottomCenter => new Vector2(midX - w * 0.5f, bb + gap),
                EspNameBoxAnchor.BoxTopLeft => new Vector2(bl - gap - w, bt),
                EspNameBoxAnchor.BoxTopRight => new Vector2(br + gap, bt),
                EspNameBoxAnchor.BoxBottomLeft => new Vector2(bl - gap - w, bb - h),
                EspNameBoxAnchor.BoxBottomRight => new Vector2(br + gap, bb - h),
                EspNameBoxAnchor.BoxMiddleLeft => new Vector2(bl - gap - w, midY - h * 0.5f),
                EspNameBoxAnchor.BoxMiddleRight => new Vector2(br + gap, midY - h * 0.5f),
                _ => new Vector2(midX - w * 0.5f, bt - gap - h),
            };
        }

        private static readonly EspNameBoxAnchor[] EspEditorNameAnchorOrder =
        {
            EspNameBoxAnchor.BoxTopCenter,
            EspNameBoxAnchor.BoxBottomCenter,
            EspNameBoxAnchor.BoxTopLeft,
            EspNameBoxAnchor.BoxTopRight,
            EspNameBoxAnchor.BoxBottomLeft,
            EspNameBoxAnchor.BoxBottomRight,
            EspNameBoxAnchor.BoxMiddleLeft,
            EspNameBoxAnchor.BoxMiddleRight,
        };

        private static Vector2 GetEspNameAnchorAttachPoint(EspNameBoxAnchor a, float bl, float bt, float br, float bb, float midX, float midY)
        {
            return a switch
            {
                EspNameBoxAnchor.BoxTopCenter => new Vector2(midX, bt),
                EspNameBoxAnchor.BoxBottomCenter => new Vector2(midX, bb),
                EspNameBoxAnchor.BoxTopLeft => new Vector2(bl, bt),
                EspNameBoxAnchor.BoxTopRight => new Vector2(br, bt),
                EspNameBoxAnchor.BoxBottomLeft => new Vector2(bl, bb),
                EspNameBoxAnchor.BoxBottomRight => new Vector2(br, bb),
                EspNameBoxAnchor.BoxMiddleLeft => new Vector2(bl, midY),
                EspNameBoxAnchor.BoxMiddleRight => new Vector2(br, midY),
                _ => new Vector2(midX, bt),
            };
        }

        private static Vector2 GetEspDistanceAnchorAttachPoint(EspDistanceBoxAnchor d, EspNameBoxAnchor nameA,
            Vector2 npTL, float npW, float npH, float bl, float bt, float br, float bb, float midX, float midY, float gap)
        {
            if (d == EspDistanceBoxAnchor.InsideNameplate)
                return npTL + new Vector2(npW * 0.5f, npH * 0.5f);
            if (d == EspDistanceBoxAnchor.AboveNameWhenNameAtTop)
            {
                if (IsNameAnchorTopRow(nameA))
                    return new Vector2(npTL.X + npW * 0.5f, npTL.Y);
                return new Vector2(midX, bt);
            }
            EspNameBoxAnchor map = d switch
            {
                EspDistanceBoxAnchor.BoxTopCenter => EspNameBoxAnchor.BoxTopCenter,
                EspDistanceBoxAnchor.BoxBottomCenter => EspNameBoxAnchor.BoxBottomCenter,
                EspDistanceBoxAnchor.BoxTopLeft => EspNameBoxAnchor.BoxTopLeft,
                EspDistanceBoxAnchor.BoxTopRight => EspNameBoxAnchor.BoxTopRight,
                EspDistanceBoxAnchor.BoxBottomLeft => EspNameBoxAnchor.BoxBottomLeft,
                EspDistanceBoxAnchor.BoxBottomRight => EspNameBoxAnchor.BoxBottomRight,
                EspDistanceBoxAnchor.BoxMiddleLeft => EspNameBoxAnchor.BoxMiddleLeft,
                EspDistanceBoxAnchor.BoxMiddleRight => EspNameBoxAnchor.BoxMiddleRight,
                _ => EspNameBoxAnchor.BoxTopCenter,
            };
            return GetEspNameAnchorAttachPoint(map, bl, bt, br, bb, midX, midY);
        }

        private static Vector2 GetEspHealthAnchorAttachPoint(EspHealthBoxAnchor a, float bl, float bt, float br, float bb, float midX, float midY, float cornerH)
        {
            float my = bt + cornerH * 0.5f;
            return a switch
            {
                EspHealthBoxAnchor.BoxTop => new Vector2(midX, bt),
                EspHealthBoxAnchor.BoxBottom => new Vector2(midX, bb),
                EspHealthBoxAnchor.BoxLeft => new Vector2(bl, my),
                _ => new Vector2(br, my),
            };
        }

        private static void DrawEspEditorNameSnapSlots(ImDrawListPtr dl, float boxL, float boxT, float boxR, float boxB, float boxMidX, float boxMidY, float gap, float slotW, float slotH, EspNameBoxAnchor currentAnchor, EspNameBoxAnchor? highlightAnchor)
        {
            const float rounding = 6f;
            uint fillBase = 0x20FFFFFF;
            uint fillCurrent = 0x40B0B0FF;
            uint fillHi = 0x55C8E0FF;
            uint borderCol = 0x5AFFFFFF;
            foreach (EspNameBoxAnchor a in EspEditorNameAnchorOrder)
            {
                Vector2 tl = GetNameBoxAnchorTopLeft(a, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gap, slotW, slotH);
                Vector2 brc = tl + new Vector2(slotW, slotH);
                bool isCur = a == currentAnchor;
                bool isHi = highlightAnchor.HasValue && a == highlightAnchor.Value;
                uint fill = isHi ? fillHi : (isCur ? fillCurrent : fillBase);
                dl.AddRectFilled(tl, brc, fill, rounding, ImDrawFlags.RoundCornersAll);
                dl.AddRect(tl, brc, borderCol, rounding, ImDrawFlags.RoundCornersAll, 1f);
            }
        }

        private static EspNameBoxAnchor? EspEditorNameSlotUnderMouse(Vector2 mouse, float boxL, float boxT, float boxR, float boxB, float boxMidX, float boxMidY, float gap, float slotW, float slotH)
        {
            const float padIn = 12f;
            foreach (EspNameBoxAnchor a in EspEditorNameAnchorOrder)
            {
                Vector2 tl = GetNameBoxAnchorTopLeft(a, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gap, slotW, slotH);
                Vector2 brc = tl + new Vector2(slotW, slotH);
                if (EspEditorPointInRectPad(mouse, tl, brc, padIn))
                    return a;
            }
            return null;
        }

        private static bool TrySnapEspNameAnchorAtMouse(Vector2 mouse, float boxL, float boxT, float boxR, float boxB, float boxMidX, float boxMidY, float gap, float slotW, float slotH, out EspNameBoxAnchor chosen)
        {
            const float padIn = 12f;
            chosen = EspNameBoxAnchor.BoxTopCenter;
            foreach (EspNameBoxAnchor a in EspEditorNameAnchorOrder)
            {
                Vector2 tl = GetNameBoxAnchorTopLeft(a, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gap, slotW, slotH);
                Vector2 brc = tl + new Vector2(slotW, slotH);
                if (EspEditorPointInRectPad(mouse, tl, brc, padIn))
                {
                    chosen = a;
                    return true;
                }
            }
            return false;
        }

        private static readonly EspDistanceBoxAnchor[] EspEditorDistanceAnchorOrder =
        {
            EspDistanceBoxAnchor.InsideNameplate,
            EspDistanceBoxAnchor.AboveNameWhenNameAtTop,
            EspDistanceBoxAnchor.BoxTopCenter,
            EspDistanceBoxAnchor.BoxBottomCenter,
            EspDistanceBoxAnchor.BoxTopLeft,
            EspDistanceBoxAnchor.BoxTopRight,
            EspDistanceBoxAnchor.BoxBottomLeft,
            EspDistanceBoxAnchor.BoxBottomRight,
            EspDistanceBoxAnchor.BoxMiddleLeft,
            EspDistanceBoxAnchor.BoxMiddleRight,
        };

        private static readonly EspHealthBoxAnchor[] EspEditorHealthAnchorOrder =
        {
            EspHealthBoxAnchor.BoxRight,
            EspHealthBoxAnchor.BoxLeft,
            EspHealthBoxAnchor.BoxTop,
            EspHealthBoxAnchor.BoxBottom,
        };

        private static void EspEditorGetDistanceSlotRect(EspDistanceBoxAnchor d, EspNameBoxAnchor nameA, Vector2 distSz,
            Vector2 npTL, float npW, float npH,
            float bl, float bt, float br, float bb, float midX, float midY, float gap,
            out Vector2 mn, out Vector2 mx)
        {
            if (d == EspDistanceBoxAnchor.InsideNameplate)
            {
                mn = npTL;
                mx = npTL + new Vector2(npW, npH);
                return;
            }
            Vector2 tl = GetDistanceBoxAnchorTopLeft(d, nameA, distSz, npTL, npW, npH, bl, bt, br, bb, midX, midY, gap);
            mn = tl;
            mx = tl + distSz;
        }

        private static void EspEditorGetHealthBarSlotRect(EspHealthBoxAnchor a, float bl, float bt, float br, float bb, float cornerW, float cornerH, float gap, float t,
            out Vector2 mn, out Vector2 mx)
        {
            switch (a)
            {
                case EspHealthBoxAnchor.BoxTop:
                    {
                        float y0 = bt - gap - t;
                        mn = new Vector2(bl, y0);
                        mx = new Vector2(br, y0 + t);
                        break;
                    }
                case EspHealthBoxAnchor.BoxBottom:
                    {
                        float y0 = bb + gap;
                        mn = new Vector2(bl, y0);
                        mx = new Vector2(br, y0 + t);
                        break;
                    }
                case EspHealthBoxAnchor.BoxLeft:
                    {
                        Vector2 p = new Vector2(bl - 5f - t, bt);
                        mn = p;
                        mx = p + new Vector2(t, cornerH);
                        break;
                    }
                default:
                    {
                        Vector2 p = new Vector2(br + 5f, bt);
                        mn = p;
                        mx = p + new Vector2(t, cornerH);
                        break;
                    }
            }
        }

        /// <summary>Actual drawn health bar rect (same math as editor preview draw) for hit-testing before draw.</summary>
        private static void EspEditorGetHealthBarDrawHitBounds(
            EspHealthBoxAnchor anchor,
            float boxL, float boxT, float boxR, float boxB,
            float cornerWidth, float cornerHeight, float gapScaled, float t,
            Vector2 offHealth,
            out Vector2 hitMin, out Vector2 hitMax)
        {
            switch (anchor)
            {
                case EspHealthBoxAnchor.BoxTop:
                    {
                        float y0 = boxT - gapScaled - t;
                        hitMin = new Vector2(boxL + offHealth.X, y0 + offHealth.Y);
                        hitMax = new Vector2(boxR + offHealth.X, y0 + t + offHealth.Y);
                        break;
                    }
                case EspHealthBoxAnchor.BoxBottom:
                    {
                        float y0 = boxB + gapScaled;
                        hitMin = new Vector2(boxL + offHealth.X, y0 + offHealth.Y);
                        hitMax = new Vector2(boxR + offHealth.X, y0 + t + offHealth.Y);
                        break;
                    }
                case EspHealthBoxAnchor.BoxLeft:
                    {
                        Vector2 p = new Vector2(boxL - 5f - t, boxT) + offHealth;
                        hitMin = p;
                        hitMax = p + new Vector2(t, cornerHeight);
                        break;
                    }
                default:
                    {
                        Vector2 p = new Vector2(boxR + 5f, boxT) + offHealth;
                        hitMin = p;
                        hitMax = p + new Vector2(t, cornerHeight);
                        break;
                    }
            }
        }

        private static void EspEditorGetWeaponSlotRect(EspWeaponBoxAnchor a, float bl, float bt, float br, float bb, float midX, float midY, float gap, Vector2 iconSz,
            out Vector2 mn, out Vector2 mx)
        {
            EspNameBoxAnchor map = a switch
            {
                EspWeaponBoxAnchor.BoxTopCenter => EspNameBoxAnchor.BoxTopCenter,
                EspWeaponBoxAnchor.BoxBottomCenter => EspNameBoxAnchor.BoxBottomCenter,
                EspWeaponBoxAnchor.BoxTopLeft => EspNameBoxAnchor.BoxTopLeft,
                EspWeaponBoxAnchor.BoxTopRight => EspNameBoxAnchor.BoxTopRight,
                EspWeaponBoxAnchor.BoxBottomLeft => EspNameBoxAnchor.BoxBottomLeft,
                EspWeaponBoxAnchor.BoxBottomRight => EspNameBoxAnchor.BoxBottomRight,
                EspWeaponBoxAnchor.BoxMiddleLeft => EspNameBoxAnchor.BoxMiddleLeft,
                EspWeaponBoxAnchor.BoxMiddleRight => EspNameBoxAnchor.BoxMiddleRight,
                _ => EspNameBoxAnchor.BoxTopCenter,
            };
            mn = GetNameBoxAnchorTopLeft(map, bl, bt, br, bb, midX, midY, gap, iconSz.X, iconSz.Y);
            mx = mn + iconSz;
        }

        private static void DrawEspEditorDistanceSnapSlots(ImDrawListPtr dl,
            EspNameBoxAnchor nameA, Vector2 distSz, Vector2 npTL, float npW, float npH,
            float bl, float bt, float br, float bb, float midX, float midY, float gap,
            EspDistanceBoxAnchor current, EspDistanceBoxAnchor? highlight)
        {
            const float rounding = 5f;
            uint fillBase = 0x22FFE090;
            uint fillCurrent = 0x45FFCC66;
            uint fillHi = 0x58FFEEDD;
            uint borderCol = 0x66FFCC44;
            foreach (EspDistanceBoxAnchor d in EspEditorDistanceAnchorOrder)
            {
                EspEditorGetDistanceSlotRect(d, nameA, distSz, npTL, npW, npH, bl, bt, br, bb, midX, midY, gap, out Vector2 mn, out Vector2 mx);
                bool isCur = d == current;
                bool isHi = highlight.HasValue && d == highlight.Value;
                uint fill = isHi ? fillHi : (isCur ? fillCurrent : fillBase);
                dl.AddRectFilled(mn, mx, fill, rounding, ImDrawFlags.RoundCornersAll);
                dl.AddRect(mn, mx, borderCol, rounding, ImDrawFlags.RoundCornersAll, 1f);
            }
        }

        private static void DrawEspEditorHealthSnapSlots(ImDrawListPtr dl,
            float bl, float bt, float br, float bb, float cornerW, float cornerH, float gap, float t,
            EspHealthBoxAnchor current, EspHealthBoxAnchor? highlight)
        {
            const float rounding = 5f;
            uint fillBase = 0x2290FF90;
            uint fillCurrent = 0x45AAFFAA;
            uint fillHi = 0x55CCFFCC;
            uint borderCol = 0x6644FF66;
            foreach (EspHealthBoxAnchor h in EspEditorHealthAnchorOrder)
            {
                EspEditorGetHealthBarSlotRect(h, bl, bt, br, bb, cornerW, cornerH, gap, t, out Vector2 mn, out Vector2 mx);
                bool isCur = h == current;
                bool isHi = highlight.HasValue && h == highlight.Value;
                uint fill = isHi ? fillHi : (isCur ? fillCurrent : fillBase);
                dl.AddRectFilled(mn, mx, fill, rounding, ImDrawFlags.RoundCornersAll);
                dl.AddRect(mn, mx, borderCol, rounding, ImDrawFlags.RoundCornersAll, 1f);
            }
        }

        private static void DrawEspEditorWeaponSnapSlots(ImDrawListPtr dl,
            float bl, float bt, float br, float bb, float midX, float midY, float gap, Vector2 iconSz,
            EspWeaponBoxAnchor current, EspWeaponBoxAnchor? highlight)
        {
            const float rounding = 5f;
            uint fillBase = 0x2290D0FF;
            uint fillCurrent = 0x45A0E0FF;
            uint fillHi = 0x55B8F0FF;
            uint borderCol = 0x6644AAFF;
            foreach (EspWeaponBoxAnchor w in new[]
            {
                EspWeaponBoxAnchor.BoxTopCenter,
                EspWeaponBoxAnchor.BoxBottomCenter,
                EspWeaponBoxAnchor.BoxTopLeft,
                EspWeaponBoxAnchor.BoxTopRight,
                EspWeaponBoxAnchor.BoxBottomLeft,
                EspWeaponBoxAnchor.BoxBottomRight,
                EspWeaponBoxAnchor.BoxMiddleLeft,
                EspWeaponBoxAnchor.BoxMiddleRight,
            })
            {
                EspEditorGetWeaponSlotRect(w, bl, bt, br, bb, midX, midY, gap, iconSz, out Vector2 mn, out Vector2 mx);
                bool isCur = w == current;
                bool isHi = highlight.HasValue && w == highlight.Value;
                uint fill = isHi ? fillHi : (isCur ? fillCurrent : fillBase);
                dl.AddRectFilled(mn, mx, fill, rounding, ImDrawFlags.RoundCornersAll);
                dl.AddRect(mn, mx, borderCol, rounding, ImDrawFlags.RoundCornersAll, 1f);
            }
        }

        private static EspDistanceBoxAnchor? EspEditorDistanceSlotUnderMouse(Vector2 mouse, EspNameBoxAnchor nameA, Vector2 distSz,
            Vector2 npTL, float npW, float npH, float bl, float bt, float br, float bb, float midX, float midY, float gap)
        {
            const float padIn = 10f;
            foreach (EspDistanceBoxAnchor d in EspEditorDistanceAnchorOrder)
            {
                EspEditorGetDistanceSlotRect(d, nameA, distSz, npTL, npW, npH, bl, bt, br, bb, midX, midY, gap, out Vector2 mn, out Vector2 mx);
                if (EspEditorPointInRectPad(mouse, mn, mx, padIn))
                    return d;
            }
            return null;
        }

        private static EspHealthBoxAnchor? EspEditorHealthSlotUnderMouse(Vector2 mouse, float bl, float bt, float br, float bb, float cornerW, float cornerH, float gap, float t)
        {
            const float padIn = 10f;
            foreach (EspHealthBoxAnchor h in EspEditorHealthAnchorOrder)
            {
                EspEditorGetHealthBarSlotRect(h, bl, bt, br, bb, cornerW, cornerH, gap, t, out Vector2 mn, out Vector2 mx);
                if (EspEditorPointInRectPad(mouse, mn, mx, padIn))
                    return h;
            }
            return null;
        }

        private static EspWeaponBoxAnchor? EspEditorWeaponSlotUnderMouse(Vector2 mouse, float bl, float bt, float br, float bb, float midX, float midY, float gap, Vector2 iconSz)
        {
            const float padIn = 10f;
            foreach (EspWeaponBoxAnchor w in new[]
            {
                EspWeaponBoxAnchor.BoxTopCenter,
                EspWeaponBoxAnchor.BoxBottomCenter,
                EspWeaponBoxAnchor.BoxTopLeft,
                EspWeaponBoxAnchor.BoxTopRight,
                EspWeaponBoxAnchor.BoxBottomLeft,
                EspWeaponBoxAnchor.BoxBottomRight,
                EspWeaponBoxAnchor.BoxMiddleLeft,
                EspWeaponBoxAnchor.BoxMiddleRight,
            })
            {
                EspEditorGetWeaponSlotRect(w, bl, bt, br, bb, midX, midY, gap, iconSz, out Vector2 mn, out Vector2 mx);
                if (EspEditorPointInRectPad(mouse, mn, mx, padIn))
                    return w;
            }
            return null;
        }

        private static bool TrySnapEspDistanceAnchorAtMouse(Vector2 mouse, EspNameBoxAnchor nameA, Vector2 distSz,
            Vector2 npTL, float npW, float npH, float bl, float bt, float br, float bb, float midX, float midY, float gap,
            out EspDistanceBoxAnchor chosen)
        {
            const float padIn = 10f;
            chosen = EspDistanceBoxAnchor.InsideNameplate;
            foreach (EspDistanceBoxAnchor d in EspEditorDistanceAnchorOrder)
            {
                EspEditorGetDistanceSlotRect(d, nameA, distSz, npTL, npW, npH, bl, bt, br, bb, midX, midY, gap, out Vector2 mn, out Vector2 mx);
                if (EspEditorPointInRectPad(mouse, mn, mx, padIn))
                {
                    chosen = d;
                    return true;
                }
            }
            return false;
        }

        private static bool TrySnapEspHealthAnchorAtMouse(Vector2 mouse, float bl, float bt, float br, float bb, float cornerW, float cornerH, float gap, float t,
            out EspHealthBoxAnchor chosen)
        {
            const float padIn = 10f;
            chosen = EspHealthBoxAnchor.BoxRight;
            foreach (EspHealthBoxAnchor h in EspEditorHealthAnchorOrder)
            {
                EspEditorGetHealthBarSlotRect(h, bl, bt, br, bb, cornerW, cornerH, gap, t, out Vector2 mn, out Vector2 mx);
                if (EspEditorPointInRectPad(mouse, mn, mx, padIn))
                {
                    chosen = h;
                    return true;
                }
            }
            return false;
        }

        private static bool TrySnapEspWeaponAnchorAtMouse(Vector2 mouse, float bl, float bt, float br, float bb, float midX, float midY, float gap, Vector2 iconSz,
            out EspWeaponBoxAnchor chosen)
        {
            const float padIn = 10f;
            chosen = EspWeaponBoxAnchor.BoxTopCenter;
            foreach (EspWeaponBoxAnchor w in new[]
            {
                EspWeaponBoxAnchor.BoxTopCenter,
                EspWeaponBoxAnchor.BoxBottomCenter,
                EspWeaponBoxAnchor.BoxTopLeft,
                EspWeaponBoxAnchor.BoxTopRight,
                EspWeaponBoxAnchor.BoxBottomLeft,
                EspWeaponBoxAnchor.BoxBottomRight,
                EspWeaponBoxAnchor.BoxMiddleLeft,
                EspWeaponBoxAnchor.BoxMiddleRight,
            })
            {
                EspEditorGetWeaponSlotRect(w, bl, bt, br, bb, midX, midY, gap, iconSz, out Vector2 mn, out Vector2 mx);
                if (EspEditorPointInRectPad(mouse, mn, mx, padIn))
                {
                    chosen = w;
                    return true;
                }
            }
            return false;
        }

        private static Vector2 GetDistanceBoxAnchorTopLeft(EspDistanceBoxAnchor d, EspNameBoxAnchor nameA, Vector2 distSz,
            Vector2 namePlateTL, float npW, float npH, float bl, float bt, float br, float bb, float midX, float midY, float gap)
        {
            if (d == EspDistanceBoxAnchor.InsideNameplate)
                return Vector2.Zero;

            if (d == EspDistanceBoxAnchor.AboveNameWhenNameAtTop)
            {
                if (IsNameAnchorTopRow(nameA))
                    return new Vector2(namePlateTL.X + npW * 0.5f - distSz.X * 0.5f, namePlateTL.Y - gap - distSz.Y);
                return new Vector2(midX - distSz.X * 0.5f, bt - gap - distSz.Y);
            }

            EspNameBoxAnchor map = d switch
            {
                EspDistanceBoxAnchor.BoxTopCenter => EspNameBoxAnchor.BoxTopCenter,
                EspDistanceBoxAnchor.BoxBottomCenter => EspNameBoxAnchor.BoxBottomCenter,
                EspDistanceBoxAnchor.BoxTopLeft => EspNameBoxAnchor.BoxTopLeft,
                EspDistanceBoxAnchor.BoxTopRight => EspNameBoxAnchor.BoxTopRight,
                EspDistanceBoxAnchor.BoxBottomLeft => EspNameBoxAnchor.BoxBottomLeft,
                EspDistanceBoxAnchor.BoxBottomRight => EspNameBoxAnchor.BoxBottomRight,
                EspDistanceBoxAnchor.BoxMiddleLeft => EspNameBoxAnchor.BoxMiddleLeft,
                EspDistanceBoxAnchor.BoxMiddleRight => EspNameBoxAnchor.BoxMiddleRight,
                _ => EspNameBoxAnchor.BoxTopCenter,
            };
            return GetNameBoxAnchorTopLeft(map, bl, bt, br, bb, midX, midY, gap, distSz.X, distSz.Y);
        }

        private static EspNameBoxAnchor MapHealthAnchorToNameAnchor(EspHealthBoxAnchor a)
        {
            return a switch
            {
                EspHealthBoxAnchor.BoxTop => EspNameBoxAnchor.BoxTopCenter,
                EspHealthBoxAnchor.BoxBottom => EspNameBoxAnchor.BoxBottomCenter,
                EspHealthBoxAnchor.BoxLeft => EspNameBoxAnchor.BoxMiddleLeft,
                _ => EspNameBoxAnchor.BoxMiddleRight,
            };
        }

        private static EspNameBoxAnchor MapWeaponAnchorToNameAnchor(EspWeaponBoxAnchor a)
        {
            return a switch
            {
                EspWeaponBoxAnchor.BoxTopCenter => EspNameBoxAnchor.BoxTopCenter,
                EspWeaponBoxAnchor.BoxBottomCenter => EspNameBoxAnchor.BoxBottomCenter,
                EspWeaponBoxAnchor.BoxTopLeft => EspNameBoxAnchor.BoxTopLeft,
                EspWeaponBoxAnchor.BoxTopRight => EspNameBoxAnchor.BoxTopRight,
                EspWeaponBoxAnchor.BoxBottomLeft => EspNameBoxAnchor.BoxBottomLeft,
                EspWeaponBoxAnchor.BoxBottomRight => EspNameBoxAnchor.BoxBottomRight,
                EspWeaponBoxAnchor.BoxMiddleLeft => EspNameBoxAnchor.BoxMiddleLeft,
                EspWeaponBoxAnchor.BoxMiddleRight => EspNameBoxAnchor.BoxMiddleRight,
                _ => EspNameBoxAnchor.BoxTopCenter,
            };
        }

        private static bool IsOccupiedByOthers(EspNameBoxAnchor candidate, bool excludeName, bool excludeHealth, bool excludeWeapon)
        {
            if (!excludeName && Config.ESPName && Config.EspNameAnchor == candidate)
                return true;

            if (!excludeHealth && Config.ESPHealth && MapHealthAnchorToNameAnchor(Config.EspHealthAnchor) == candidate)
                return true;

            if (!excludeWeapon && Config.espweapon && MapWeaponAnchorToNameAnchor(Config.EspWeaponAnchor) == candidate)
                return true;

            return false;
        }

        private static Vector4 ColorToVec4(Color c)
        {
            return new Vector4(c.R / 255f, c.G / 255f, c.B / 255f, c.A / 255f);
        }

        private static Color Vec4ToColor(Vector4 v)
        {
            v.X = Math.Clamp(v.X, 0f, 1f);
            v.Y = Math.Clamp(v.Y, 0f, 1f);
            v.Z = Math.Clamp(v.Z, 0f, 1f);
            v.W = Math.Clamp(v.W, 0f, 1f);
            return Color.FromArgb((int)(v.W * 255f), (int)(v.X * 255f), (int)(v.Y * 255f), (int)(v.Z * 255f));
        }

        private static string GetEspEditorColorTargetLabel(EspEditorColorTarget t)
        {
            return t switch
            {
                EspEditorColorTarget.Line => "Line",
                EspEditorColorTarget.Box => "Box",
                EspEditorColorTarget.Name => "Name",
                EspEditorColorTarget.Health => "Health",
                EspEditorColorTarget.Weapon => "Weapon",
                EspEditorColorTarget.Hacker => "Hacker",
                _ => "ESP",
            };
        }

        private bool TryGetEspEditorTargetColor(EspEditorColorTarget t, out Vector4 c)
        {
            c = new Vector4(1f, 1f, 1f, 1f);
            switch (t)
            {
                case EspEditorColorTarget.Line: c = ColorToVec4(Config.ESPLineColor); return true;
                case EspEditorColorTarget.Box: c = ColorToVec4(Config.ESPBoxColor); return true;
                case EspEditorColorTarget.Name: c = ColorToVec4(Config.ESPNameColor); return true;
                case EspEditorColorTarget.Health: c = ColorToVec4(Config.ESPHealthColor); return true;
                case EspEditorColorTarget.Weapon: c = Config.ICONCOLOR; return true;
                case EspEditorColorTarget.Hacker: c = ColorToVec4(Config.HackerTagColor); return true;
                default: return false;
            }
        }

        private void SetEspEditorTargetColor(EspEditorColorTarget t, Vector4 c)
        {
            switch (t)
            {
                case EspEditorColorTarget.Line: Config.ESPLineColor = Vec4ToColor(c); break;
                case EspEditorColorTarget.Box: Config.ESPBoxColor = Vec4ToColor(c); break;
                case EspEditorColorTarget.Name: Config.ESPNameColor = Vec4ToColor(c); break;
                case EspEditorColorTarget.Health: Config.ESPHealthColor = Vec4ToColor(c); break;
                case EspEditorColorTarget.Weapon: Config.ICONCOLOR = c; break;
                case EspEditorColorTarget.Hacker: Config.HackerTagColor = Vec4ToColor(c); break;
            }
        }

        private const float EspNameplateBarH = 20f;
        private const float EspNameplateHealthH = 3f;
        private const float EspNameplateAboveHeadGap = 5f;
        private const float EspNameplateIdW = 18f;
        private const float EspNameplateMinWidth = 128f;

        /// <summary>Keep full player names for ESP; only trim nulls / extreme lengths (memory-safe).</summary>
        private static string SanitizeEspDisplayName(string name)
        {
            if (string.IsNullOrWhiteSpace(name)) return "Training Bot";
            string t = name.Trim('\0', '\u200B', '\t');
            if (t.Length > 512) return t.Substring(0, 509) + "...";
            return t;
        }

        private static Vector2 EspCalcTextSizeName(string text)
        {
            if (string.IsNullOrEmpty(text)) text = " ";
            if (TextFont.IsLoaded())
            {
                ImGui.PushFont(TextFont);
                Vector2 s = ImGui.CalcTextSize(text);
                ImGui.PopFont();
                return s;
            }
            return ImGui.CalcTextSize(text);
        }

        private static void ComputeEspNameplateSize(string nameDisplay, string distStr, out float width, out float totalHeight)
        {
            const float padMain = 5f;
            Vector2 n = EspCalcTextSizeName(nameDisplay);
            Vector2 d = ImGui.CalcTextSize(distStr);
            width = EspNameplateIdW + padMain + n.X + 8f + d.X + padMain;
            width = Math.Max(EspNameplateMinWidth, width);
            totalHeight = EspNameplateBarH + EspNameplateHealthH;
        }

        private static void ComputeEspNameplateSizeNameOnly(string nameDisplay, out float width, out float totalHeight)
        {
            const float padMain = 5f;
            Vector2 n = EspCalcTextSizeName(nameDisplay);
            width = EspNameplateIdW + padMain + n.X + padMain;
            width = Math.Max(96f, width);
            totalHeight = EspNameplateBarH + EspNameplateHealthH;
        }

        private static void DrawEspInfoNameplate(
            ImDrawListPtr dl,
            Vector2 topLeft,
            float nameplateW,
            string nameDisplay,
            string distStr,
            int teamDigit,
            float healthFraction,
            bool knocked,
            bool drawDistanceInBar = true)
        {
            bool popFont = TextFont.IsLoaded();
            if (popFont) ImGui.PushFont(TextFont);

            const float padMain = 6f;
            float barH = EspNameplateBarH;
            float healthH = EspNameplateHealthH;
            float totalH = barH + healthH;
            teamDigit = Math.Abs(teamDigit % 10);

            Vector2 tl = topLeft;
            Vector2 br = tl + new Vector2(nameplateW, totalH);

            uint bgHealthTrack = ImGui.ColorConvertFloat4ToU32(new Vector4(0.04f, 0.04f, 0.05f, 0.95f));
            dl.AddRectFilled(new Vector2(tl.X, tl.Y + barH), br, bgHealthTrack);

            float hp = Math.Clamp(healthFraction, 0f, 1f);
            uint hpCol = knocked
                ? ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 0.25f, 0.25f, 1f))
                : ImGui.ColorConvertFloat4ToU32(new Vector4(0.15f, 0.95f, 0.2f, 1f));
            if (nameplateW * hp > 0.5f)
                dl.AddRectFilled(new Vector2(tl.X, tl.Y + barH), new Vector2(tl.X + nameplateW * hp, br.Y), hpCol);

            dl.AddRectFilled(tl, tl + new Vector2(EspNameplateIdW, barH), ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 1f, 1f, 1f)));
            dl.AddRect(tl, tl + new Vector2(EspNameplateIdW, barH), ImGui.ColorConvertFloat4ToU32(new Vector4(0f, 0f, 0f, 0.55f)), 0f, ImDrawFlags.None, 1f);

            string idStr = teamDigit.ToString();
            Vector2 idSz = ImGui.CalcTextSize(idStr);
            dl.AddText(tl + new Vector2((EspNameplateIdW - idSz.X) * 0.5f, (barH - idSz.Y) * 0.5f),
                ImGui.ColorConvertFloat4ToU32(new Vector4(0f, 0f, 0f, 1f)), idStr);

            Vector2 mainMin = tl + new Vector2(EspNameplateIdW, 0f);
            Vector2 mainMax = tl + new Vector2(nameplateW, barH);
            uint cTL = ImGui.ColorConvertFloat4ToU32(new Vector4(0.07f, 0.32f, 0.38f, 0.98f));
            uint cTR = ImGui.ColorConvertFloat4ToU32(new Vector4(0.14f, 0.52f, 0.56f, 0.98f));
            uint cBL = ImGui.ColorConvertFloat4ToU32(new Vector4(0.06f, 0.26f, 0.32f, 0.98f));
            uint cBR = ImGui.ColorConvertFloat4ToU32(new Vector4(0.11f, 0.42f, 0.48f, 0.98f));
            dl.AddRectFilledMultiColor(mainMin, mainMax, cTL, cTR, cBR, cBL);

            Vector2 nameSz = ImGui.CalcTextSize(nameDisplay);
            float nameX = mainMin.X + padMain;
            float textY = tl.Y + (barH - nameSz.Y) * 0.5f;
            dl.AddText(new Vector2(nameX, textY), ColorToUint32(Config.ESPNameColor), nameDisplay);

            if (drawDistanceInBar)
            {
                Vector2 distSz = ImGui.CalcTextSize(distStr);
                float distX = mainMax.X - padMain - distSz.X;
                dl.AddText(new Vector2(distX, textY), ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 0.93f, 0.2f, 1f)), distStr);
            }

            uint borderBlack = ImGui.ColorConvertFloat4ToU32(new Vector4(0f, 0f, 0f, 0.92f));
            dl.AddRect(tl, br, borderBlack, 0f, ImDrawFlags.None, 1f);

            if (popFont) ImGui.PopFont();
        }

        static uint ColorToUint32(Color color)
        {
            // Correct RGBA order: Red, Green, Blue, Alpha
            return ImGui.ColorConvertFloat4ToU32(new Vector4(
                color.R / 255.0f,
                color.G / 255.0f,
                color.B / 255.0f,
                color.A / 255.0f));
        }

        // HSV -> Color helper (for ESP RGB)
        private static Color ColorFromHSV(float hue, float saturation, float value)
        {
            int hi = System.Convert.ToInt32(Math.Floor(hue / 60)) % 6;
            float f = hue / 60 - (float)Math.Floor(hue / 60);
            value *= 255;
            int v = System.Convert.ToInt32(value);
            int p = System.Convert.ToInt32(value * (1 - saturation));
            int q = System.Convert.ToInt32(value * (1 - f * saturation));
            int t = System.Convert.ToInt32(value * (1 - (1 - f) * saturation));

            return hi switch
            {
                0 => Color.FromArgb(255, v, t, p),
                1 => Color.FromArgb(255, q, v, p),
                2 => Color.FromArgb(255, p, v, t),
                3 => Color.FromArgb(255, p, q, v),
                4 => Color.FromArgb(255, t, p, v),
                _ => Color.FromArgb(255, v, p, q),
            };
        }

        private int lastWidth = 0, lastHeight = 0;
        // Offset between emulator render area (Core coords origin) and overlay window coords origin.
        // Used so ESP stays aligned even if overlay window is expanded beyond the emulator.
        private Vector2 _emulatorScreenOffset = Vector2.Zero;
        private Vector2 _emulatorClipMin = Vector2.Zero;
        private Vector2 _emulatorClipMax = Vector2.Zero;
        void CreateHandle()
        {
            if (Core.Handle == IntPtr.Zero)
                return;
            if (Config.StreamMode)
            {
                SetWindowDisplayAffinity(hWnd, WDA_EXCLUDEFROMCAPTURE);
            }
            else
            {
                SetWindowDisplayAffinity(hWnd, WDA_NONE);
            }
            RECT emulatorRect;
            GetWindowRect(Core.Handle, out emulatorRect);

            int emulatorX = emulatorRect.Left;
            int emulatorY = emulatorRect.Top;
            int emulatorWidth = emulatorRect.Right - emulatorRect.Left;
            int emulatorHeight = emulatorRect.Bottom - emulatorRect.Top;

            // Expand overlay to FULL virtual desktop so UI can be dragged out from every side.
            // ESP projection stays in emulator-size; drawing positions are shifted by _emulatorScreenOffset.
            int virtualLeft = int.MaxValue;
            int virtualTop = int.MaxValue;
            int virtualRight = int.MinValue;
            int virtualBottom = int.MinValue;
            foreach (var screen in System.Windows.Forms.Screen.AllScreens)
            {
                virtualLeft = Math.Min(virtualLeft, screen.Bounds.Left);
                virtualTop = Math.Min(virtualTop, screen.Bounds.Top);
                virtualRight = Math.Max(virtualRight, screen.Bounds.Right);
                virtualBottom = Math.Max(virtualBottom, screen.Bounds.Bottom);
            }

            int overlayWidth = virtualRight - virtualLeft;
            int overlayHeight = virtualBottom - virtualTop;

            if (overlayWidth != lastWidth || overlayHeight != lastHeight)
            {
                lastWidth = overlayWidth;
                lastHeight = overlayHeight;
                ImGui.SetWindowSize(new Vector2(overlayWidth, overlayHeight));
            }

            Size = new Size(overlayWidth, overlayHeight);
            Position = new Point(virtualLeft, virtualTop);

            // Core coords origin is emulator top-left, but overlay coords origin is virtual top-left.
            _emulatorScreenOffset = new Vector2(emulatorX - virtualLeft, emulatorY - virtualTop);
            _emulatorClipMin = _emulatorScreenOffset;
            _emulatorClipMax = _emulatorScreenOffset + new Vector2(emulatorWidth, emulatorHeight);

            Core.Width = emulatorWidth;
            Core.Height = emulatorHeight;
        }

        public void DrawFOVCircle(float radius)
        {
            var drawList = ImGui.GetBackgroundDrawList();
            var center = new Vector2(Core.Width / 2f, Core.Height / 2f) + _emulatorScreenOffset;
            uint color = ColorToUint32(Config.FOVColor);

            drawList.AddCircle(center, radius, color, 0, 1f);
        }


        Vector4 darkBg = new Vector4(18 / 255f, 18 / 255f, 22 / 255f, 1.0f);

        // Premium accent — blue family (matches requested UI)
        Vector4 accent1 = new Vector4(0.10f, 0.78f, 0.94f, 1.0f);
        Vector4 accent2 = new Vector4(0.08f, 0.62f, 0.80f, 1.0f);
        Vector4 accent3 = new Vector4(0.06f, 0.22f, 0.30f, 1.0f);

        private void ApplyCustomStyle()
        {
            var style = ImGui.GetStyle();
            var colors = style.Colors;

            Vector4 darkBg = new Vector4(18 / 255f, 18 / 255f, 22 / 255f, 1.0f);
            Vector4 accent1 = new Vector4(0.10f, 0.78f, 0.94f, 1.0f);
            Vector4 accent2 = new Vector4(0.08f, 0.62f, 0.80f, 1.0f);
            Vector4 accent3 = new Vector4(0.06f, 0.22f, 0.30f, 1.0f);



            colors[(int)ImGuiCol.WindowBg] = darkBg;
            colors[(int)ImGuiCol.ChildBg] = new Vector4(0.12f, 0.12f, 0.15f, 0.95f);
            colors[(int)ImGuiCol.Border] = new Vector4(0.25f, 0.25f, 0.3f, 0.5f);
            colors[(int)ImGuiCol.BorderShadow] = new Vector4(0, 0, 0, 0.5f);

            colors[(int)ImGuiCol.Text] = new Vector4(0.98f, 0.98f, 0.98f, 1.0f);
            colors[(int)ImGuiCol.TextDisabled] = new Vector4(0.5f, 0.5f, 0.55f, 1.0f);

            colors[(int)ImGuiCol.Button] = new Vector4(0.22f, 0.22f, 0.26f, 0.8f);
            colors[(int)ImGuiCol.ButtonHovered] = new Vector4(0.28f, 0.28f, 0.32f, 1f);
            colors[(int)ImGuiCol.ButtonActive] = accent2;

            Vector4 sliderNeutral = new Vector4(0.52f, 0.54f, 0.58f, 1f);
            colors[(int)ImGuiCol.SliderGrab] = sliderNeutral;
            colors[(int)ImGuiCol.SliderGrabActive] = new Vector4(0.62f, 0.64f, 0.68f, 1f);

            colors[(int)ImGuiCol.CheckMark] = accent1;
            colors[(int)ImGuiCol.FrameBg] = new Vector4(0.15f, 0.15f, 0.18f, 0.6f);
            colors[(int)ImGuiCol.FrameBgHovered] = new Vector4(0.2f, 0.2f, 0.24f, 0.7f);
            colors[(int)ImGuiCol.FrameBgActive] = new Vector4(0.25f, 0.25f, 0.3f, 0.8f);

            colors[(int)ImGuiCol.Header] = accent3;
            colors[(int)ImGuiCol.HeaderHovered] = accent2;
            colors[(int)ImGuiCol.HeaderActive] = accent1;

            colors[(int)ImGuiCol.Separator] = new Vector4(42f / 255f, 42f / 255f, 42f / 255f, 0.9f);

            colors[(int)ImGuiCol.PopupBg] = new Vector4(0.12f, 0.12f, 0.15f, 0.98f);
            colors[(int)ImGuiCol.ScrollbarBg] = new Vector4(0.1f, 0.1f, 0.12f, 0.5f);
            colors[(int)ImGuiCol.ScrollbarGrab] = new Vector4(0.3f, 0.3f, 0.35f, 0.8f);
            colors[(int)ImGuiCol.ScrollbarGrabHovered] = new Vector4(0.4f, 0.4f, 0.45f, 1f);
            colors[(int)ImGuiCol.ScrollbarGrabActive] = sliderNeutral;

            // Premium rounding - smoother
            style.WindowRounding = 10.0f;
            style.FrameRounding = 6.0f;
            style.GrabRounding = 5.0f;
            style.ScrollbarRounding = 5.0f;
            style.ScrollbarSize = 0.0f;
            style.TabRounding = 6.0f;
            style.ChildRounding = 8.0f;
            style.PopupRounding = 8.0f;

            // Compact spacing (reduce gaps between functions)
            style.WindowPadding = new Vector2(8, 8);
            style.FramePadding = new Vector2(6, 4);
            style.ItemSpacing = new Vector2(2, 1);
            style.ItemInnerSpacing = new Vector2(3, 1);
            style.WindowTitleAlign = new Vector2(0.0f, 0.5f);
            style.ButtonTextAlign = new Vector2(0.5f, 0.5f);
        }
        private Vector4 Lerp(Vector4 a, Vector4 b, float t)
        {
            return new Vector4(
                a.X + (b.X - a.X) * t,
                a.Y + (b.Y - a.Y) * t,
                a.Z + (b.Z - a.Z) * t,
                a.W + (b.W - a.W) * t
            );
        }

        private void DrawBottomCenterTextSafe()
        {
            if (_uiAlpha <= 0.01f)
                return;

            ImGui.SetNextWindowPos(Vector2.Zero);
            ImGui.SetNextWindowSize(ImGui.GetIO().DisplaySize);

            ImGui.Begin("##bottom_text_overlay_cs",
                ImGuiWindowFlags.NoDecoration |
                ImGuiWindowFlags.NoInputs |
                ImGuiWindowFlags.NoBackground |
                ImGuiWindowFlags.NoBringToFrontOnFocus);

            var draw = ImGui.GetWindowDrawList();
            string text = "XPR X SXT  </>";
            if (LebelFont.IsLoaded())
                ImGui.PushFont(LebelFont);
            Vector2 textSize = ImGui.CalcTextSize(text);
            Vector2 pos = new Vector2(
                (ImGui.GetIO().DisplaySize.X - textSize.X) * 0.5f,
                ImGui.GetIO().DisplaySize.Y - textSize.Y - 80f
            );
            float t = (float)ImGui.GetTime() * 0.8f;
            Vector4 rainbow = new Vector4(
                0.5f + 0.5f * MathF.Sin(t),
                0.5f + 0.5f * MathF.Sin(t + 2.094f),
                0.5f + 0.5f * MathF.Sin(t + 4.188f),
                1f * _uiAlpha
            );
            for (int i = 3; i >= 1; i--)
            {
                float spread = i * 2f;
                float alpha = (0.18f * _uiAlpha) / i;
                draw.AddText(
                    pos + new Vector2(-spread, 0),
                    ImGui.GetColorU32(new Vector4(rainbow.X, rainbow.Y, rainbow.Z, alpha)),
                    text
                );
            }
            draw.AddText(pos, ImGui.GetColorU32(new Vector4(rainbow.X, rainbow.Y, rainbow.Z, _uiAlpha)), text);
            if (LebelFont.IsLoaded())
                ImGui.PopFont();
            ImGui.End();
        }

        private void ApplyThemeOverlay()
        {
            var style = ImGui.GetStyle();
            style.WindowRounding = 6f;
            style.FrameRounding = 5f;
            style.WindowBorderSize = 0f;
            style.ChildBorderSize = 0f;
            style.PopupBorderSize = 0f;
            style.ScrollbarSize = 0f;
            var c = style.Colors;
            c[(int)ImGuiCol.WindowBg] = new Vector4(0.06f, 0.06f, 0.08f, _uiAlpha);
            Vector4 panelA = new Vector4(PanelBg.X, PanelBg.Y, PanelBg.Z, _uiAlpha);
            c[(int)ImGuiCol.FrameBg] = panelA;
            c[(int)ImGuiCol.FrameBgHovered] = new Vector4(0.16f, 0.16f, 0.19f, _uiAlpha);
            c[(int)ImGuiCol.FrameBgActive] = new Vector4(0.19f, 0.19f, 0.22f, _uiAlpha);
            c[(int)ImGuiCol.Button] = panelA;
            c[(int)ImGuiCol.ButtonHovered] = new Vector4(0.11f, 0.20f, 0.24f, _uiAlpha);
            c[(int)ImGuiCol.ButtonActive] = new Vector4(0.13f, 0.28f, 0.34f, _uiAlpha);
            c[(int)ImGuiCol.PopupBg] = new Vector4(0.1f, 0.1f, 0.12f, 0.98f * _uiAlpha);
            c[(int)ImGuiCol.Header] = new Vector4(0.12f, 0.12f, 0.14f, _uiAlpha);
            c[(int)ImGuiCol.HeaderHovered] = new Vector4(0.16f, 0.16f, 0.18f, _uiAlpha);
            c[(int)ImGuiCol.HeaderActive] = new Vector4(0.18f, 0.18f, 0.2f, _uiAlpha);
            c[(int)ImGuiCol.Text] = new Vector4(1f, 1f, 1f, _uiAlpha);
            c[(int)ImGuiCol.Separator] = new Vector4(42f / 255f, 42f / 255f, 42f / 255f, 0.92f * _uiAlpha);
            c[(int)ImGuiCol.Border] = new Vector4(0.18f, 0.18f, 0.2f, 0f);
        }

        private static void DrawHeader()
        {
            const float headerHeight = 42f;
            const float extendDown = 8f;
            const float corner = 8f;

            ImGui.BeginChild("##header", new Vector2(0, headerHeight + extendDown), ImGuiChildFlags.None,
                ImGuiWindowFlags.NoScrollbar | ImGuiWindowFlags.NoScrollWithMouse);
            var draw = ImGui.GetWindowDrawList();

            Vector2 pos = ImGui.GetWindowPos();
            Vector2 size = ImGui.GetWindowSize();
            float t = (float)ImGui.GetTime();
            float pulse = 0.55f + 0.45f * MathF.Sin(t * 1.9f);

            Vector2 bgMin = pos;
            Vector2 bgMax = pos + new Vector2(size.X, headerHeight);
            draw.AddRectFilled(bgMin, bgMax, ImGui.GetColorU32(new Vector4(0.03f, 0.04f, 0.06f, 0.99f * _uiAlpha)), corner);
            draw.AddRect(bgMin, bgMax, ImGui.GetColorU32(new Vector4(0.34f, 0.84f, 0.96f, 0.12f * _uiAlpha)), corner);
            draw.AddRectFilled(
                bgMin + new Vector2(1f, 1f),
                new Vector2(bgMax.X - 1f, bgMin.Y + 13f),
                ImGui.GetColorU32(new Vector4(1f, 1f, 1f, 0.025f * _uiAlpha)),
                corner);

            if (TextFont.IsLoaded()) ImGui.PushFont(TextFont); else ImGui.PushFont(HeaderFont);
            const string brandLeft = "HARSH";
            const string brandRight = "X CHEAT";
            Vector2 leftSize = ImGui.CalcTextSize(brandLeft);
            Vector2 rightSize = ImGui.CalcTextSize(brandRight);
            float brandY = pos.Y + (headerHeight - leftSize.Y) * 0.5f - 1f;
            Vector2 brandPos = new Vector2(pos.X + 14f, brandY);
            draw.AddText(brandPos + new Vector2(1f, 1f), ImGui.GetColorU32(new Vector4(0.2f, 0.82f, 0.96f, (0.2f + pulse * 0.25f) * _uiAlpha)), brandLeft);
            draw.AddText(brandPos, ImGui.GetColorU32(new Vector4(0.14f, 0.78f, 0.94f, _uiAlpha)), brandLeft);
            Vector2 rightPos = new Vector2(brandPos.X + leftSize.X + 16f, brandY + (leftSize.Y - rightSize.Y) * 0.5f);
            draw.AddText(rightPos, ImGui.GetColorU32(new Vector4(0.9f, 0.95f, 1f, _uiAlpha)), brandRight);
            ImGui.PopFont();

            Vector2 closeSize = new Vector2(18f, 18f);
            Vector2 closePos = new(bgMax.X - closeSize.X - 10f, pos.Y + (headerHeight - closeSize.Y) * 0.5f);
            ImGui.SetCursorScreenPos(closePos);
            ImGui.InvisibleButton("##close_btn", closeSize);
            if (ImGui.IsItemClicked())
                _uiVisible = false;
            float pad = 3f;
            uint closeCol = ImGui.GetColorU32(new Vector4(1f, 1f, 1f, (ImGui.IsItemHovered() ? 0.95f : 0.65f) * _uiAlpha));
            if (ImGui.IsItemHovered())
                draw.AddCircleFilled(closePos + closeSize * 0.5f, 11f, ImGui.GetColorU32(new Vector4(0.20f, 0.82f, 0.96f, 0.18f * _uiAlpha)));
            draw.AddLine(closePos + new Vector2(pad, pad), closePos + closeSize - new Vector2(pad, pad), closeCol, 1.2f);
            draw.AddLine(new Vector2(closePos.X + pad, closePos.Y + closeSize.Y - pad), new Vector2(closePos.X + closeSize.X - pad, closePos.Y + pad), closeCol, 1.2f);
            ImGui.EndChild();
        }

        private void DrawPanelBackground(float rounding)
        {
            var draw = ImGui.GetWindowDrawList();
            Vector2 p = ImGui.GetWindowPos();
            Vector2 s = ImGui.GetWindowSize();
            float glowPhase = 0.5f + 0.5f * MathF.Sin((float)ImGui.GetTime() * 1.3f);
            float glowA = (0.004f + glowPhase * 0.008f) * _uiAlpha;

            draw.AddRectFilled(
                p - new Vector2(2f, 2f),
                p + s + new Vector2(2f, 2f),
                ImGui.GetColorU32(new Vector4(0.14f, 0.78f, 0.94f, glowA)),
                rounding + 2f);

            draw.AddRectFilled(p, p + s,
                ImGui.GetColorU32(new Vector4(PanelBg.X, PanelBg.Y, PanelBg.Z, PanelBg.W * _uiAlpha)),
                rounding);
            draw.AddRect(p, p + s,
                ImGui.GetColorU32(new Vector4(PanelBorder.X, PanelBorder.Y, PanelBorder.Z, PanelBorder.W * _uiAlpha)),
                rounding);
            ImGui.SetCursorPos(ImGui.GetCursorPos() + new Vector2(8, 8));
        }

        private bool DrawHexTabButton(string id, Vector2 center, string glyph, bool active, bool enabled)
        {
            var draw = ImGui.GetWindowDrawList();
            var io = ImGui.GetIO();

            float radius = 28f;
            Vector2 p = center - new Vector2(radius, radius);
            ImGui.SetCursorScreenPos(p);
            ImGui.InvisibleButton(id, new Vector2(radius * 2f, radius * 2f));

            bool hovered = enabled && ImGui.IsItemHovered();
            bool clicked = enabled && ImGui.IsItemClicked();

            Vector4 fill = active
                ? new Vector4(0.12f, 0.80f, 0.95f, 0.96f * _uiAlpha)
                : hovered
                    ? new Vector4(0.15f, 0.15f, 0.17f, 0.96f * _uiAlpha)
                    : new Vector4(0.09f, 0.09f, 0.11f, 0.94f * _uiAlpha);
            Vector4 border = active
                ? new Vector4(0.50f, 0.93f, 1f, 0.95f * _uiAlpha)
                : new Vector4(1f, 1f, 1f, 0.11f * _uiAlpha);

            draw.AddNgonFilled(center, radius, ImGui.GetColorU32(fill), 6);
            draw.AddNgon(center, radius, ImGui.GetColorU32(border), 6, 1.8f);
            if (active)
            {
                draw.AddNgon(center, radius + 3f, ImGui.GetColorU32(new Vector4(0.15f, 0.8f, 1f, 0.22f * _uiAlpha)), 6, 3f);
            }

            Vector2 txtSize = ImGui.CalcTextSize(glyph);
            Vector2 txtPos = center - txtSize * 0.5f;
            Vector4 iconCol = active
                ? new Vector4(1f, 1f, 1f, _uiAlpha)
                : new Vector4(0.88f, 0.88f, 0.9f, _uiAlpha);
            draw.AddText(txtPos, ImGui.GetColorU32(iconCol), glyph);

            return clicked;
        }

        private void DrawTabs()
        {
            _tabDropTargetValid = false;
            Vector2 basePos = ImGui.GetCursorScreenPos() + new Vector2(44f, 42f);
            Vector2[] centers =
            {
                basePos + new Vector2(0f, 0f),
                basePos + new Vector2(50f, -30f),
                basePos + new Vector2(100f, 0f),
                basePos + new Vector2(50f, 30f),
                basePos + new Vector2(0f, 60f),
                basePos + new Vector2(100f, 60f),
            };
            int[] tabMap = { 2, 0, 1, 3, 4, 2 };
            string[] glyphs = { "\uF085", "\uF05B", "\uF015", "\uF06E", "\uF007", "\uF53F" };

            bool enabled = _flowPhase == FlowPhase.Main;
            for (int i = 0; i < centers.Length; i++)
            {
                int tab = tabMap[i];
                bool active = enabled && _activeTab == tab;
                bool clicked = DrawHexTabButton($"##hex_tab_{i}", centers[i], glyphs[i], active, enabled);
                if (clicked && _activeTab != tab)
                {
                    _previousTab = _activeTab;
                    _activeTab = tab;
                    _showTabPopup = true;
                }
            }

            ImGui.SetCursorScreenPos(basePos + new Vector2(-8f, 128f));
            ImGui.Dummy(new Vector2(150f, 80f));

            if (TextFont.IsLoaded())
                ImGui.PushFont(TextFont);

            var io = ImGui.GetIO();
            foreach (var key in _tabFade.Keys)
            {
                float target = _flowPhase == FlowPhase.Login
                    ? (key == 4 ? 1f : 0f)
                    : ((_activeTab == key) ? 1f : 0f);
                _tabFade[key] += (target - _tabFade[key]) * io.DeltaTime * 14f;
                _tabFade[key] = Math.Clamp(_tabFade[key], 0f, 1f);
            }

            if (TextFont.IsLoaded())
                ImGui.PopFont();
        }

        private void DrawFloatingTabsWindow(Vector2 mainWindowPos)
        {
            Vector2 tabWindowSize = new Vector2(178f, 228f);
            Vector2 tabWindowPos = mainWindowPos + new Vector2(-194f, 92f);

            ImGui.SetNextWindowPos(tabWindowPos, ImGuiCond.Always);
            ImGui.SetNextWindowSize(tabWindowSize, ImGuiCond.Always);
            ImGui.SetNextWindowBgAlpha(0f);
            ImGui.Begin("##hex_tabs_floating",
                ImGuiWindowFlags.NoDecoration |
                ImGuiWindowFlags.NoBackground |
                ImGuiWindowFlags.NoMove |
                ImGuiWindowFlags.NoResize |
                ImGuiWindowFlags.NoScrollbar |
                ImGuiWindowFlags.NoScrollWithMouse |
                ImGuiWindowFlags.NoSavedSettings |
                ImGuiWindowFlags.NoBringToFrontOnFocus);

            if (_flowPhase != FlowPhase.Main)
                ImGui.BeginDisabled();
            DrawTabs();
            if (_flowPhase != FlowPhase.Main)
                ImGui.EndDisabled();

            ImGui.End();
        }

        private void DrawContentFade(Action drawAction)
        {
            float fade = _tabFade.ContainsKey(_activeTab) ? _tabFade[_activeTab] : 1f;
            if (fade <= 0f) return;

            float combinedAlpha = fade * _uiAlpha;
            ImGui.PushStyleColor(ImGuiCol.Text, new Vector4(1f, 1f, 1f, combinedAlpha));
            ImGui.PushStyleColor(ImGuiCol.ChildBg, new Vector4(1f, 1f, 1f, combinedAlpha));
            // Subtle dark divider under tab titles (~#2A2A2A), matches reference
            ImGui.PushStyleColor(ImGuiCol.Separator, new Vector4(42f / 255f, 42f / 255f, 42f / 255f, combinedAlpha));
            ImGui.PushStyleVar(ImGuiStyleVar.Alpha, combinedAlpha);
            drawAction.Invoke();
            ImGui.PopStyleVar();
            ImGui.PopStyleColor(3);
        }

        private void DrawMainContent()
        {
            switch (_activeTab)
            {
                case 0:
                    DrawContentFade(() =>
                    {
                        if (TextFont.IsLoaded()) ImGui.PushFont(TextFont);
                        RenderAimTab();
                        if (TextFont.IsLoaded()) ImGui.PopFont();
                    });
                    break;
                case 1:
                    DrawContentFade(() =>
                    {
                        if (TextFont.IsLoaded()) ImGui.PushFont(TextFont);
                        RenderVisualsTab();
                        if (TextFont.IsLoaded()) ImGui.PopFont();
                    });
                    break;
                case 2:
                    DrawContentFade(() =>
                    {
                        if (TextFont.IsLoaded()) ImGui.PushFont(TextFont);
                        RenderMiscTab();
                        if (TextFont.IsLoaded()) ImGui.PopFont();
                    });
                    break;
                case 3:
                    DrawContentFade(() =>
                    {
                        if (TextFont.IsLoaded()) ImGui.PushFont(TextFont);
                        RenderConfigTab();
                        if (TextFont.IsLoaded()) ImGui.PopFont();
                    });
                    break;
                case 4:
                    DrawContentFade(() =>
                    {
                        if (TextFont.IsLoaded()) ImGui.PushFont(TextFont);
                        RenderSkullTab();
                        if (TextFont.IsLoaded()) ImGui.PopFont();
                    });
                    break;
            }
        }

        private void DrawTabPopup()
        {
            var io = ImGui.GetIO();
            float speed = 18f;
            _tabPopupAnim += ((_showTabPopup ? 1f : 0f) - _tabPopupAnim) * io.DeltaTime * speed;
            _tabPopupAnim = Math.Clamp(_tabPopupAnim, 0f, 1f);

            if (_tabPopupAnim < 0.01f) return;

            Vector2 center = ImGui.GetMainViewport().GetCenter();
            Vector2 popupSize = new Vector2(220, 100) * _tabPopupAnim;
            var draw = ImGui.GetWindowDrawList();
            Vector2 p = center - popupSize * 0.5f;

            draw.AddRectFilled(p, p + popupSize,
                ImGui.GetColorU32(new Vector4(0.1f, 0.1f, 0.15f, 0.95f * _tabPopupAnim * _uiAlpha)),
                8f);
            draw.AddRectFilled(
                p - new Vector2(4f, 4f),
                p + popupSize + new Vector2(4f, 4f),
                ImGui.GetColorU32(new Vector4(1f, 0.12f, 0.12f, 0.06f * _tabPopupAnim * _uiAlpha)),
                10f);
            draw.AddRect(p, p + popupSize,
                ImGui.GetColorU32(new Vector4(1f, 1f, 1f, 0.6f * _tabPopupAnim * _uiAlpha)),
                8f);

            Vector2 textSize = ImGui.CalcTextSize("Tab Changed!");
            draw.AddText(p + (popupSize - textSize) * 0.5f,
                ImGui.GetColorU32(new Vector4(ThemeColor.X, ThemeColor.Y, ThemeColor.Z, _tabPopupAnim * _uiAlpha)),
                "Tab Changed!");

            if (_tabPopupAnim >= 0.99f)
                _showTabPopup = false;
        }

        private void EnsureTabIconsLoaded()
        {
            if (_tabIconsInitialized)
                return;
            _tabIconsInitialized = true;
            // Primary: Font Awesome glyphs via TextFont (fa-solid-900 merged). PNGs only if TextFont unavailable.
        }

        private IntPtr LoadTabIcon(string fileName)
        {
            string[] candidatePaths =
            {
                Path.Combine(AppContext.BaseDirectory, "Components", fileName),
                Path.Combine(Directory.GetCurrentDirectory(), "Components", fileName),
                Path.Combine(AppContext.BaseDirectory, fileName)
            };

            for (int i = 0; i < candidatePaths.Length; i++)
            {
                if (!File.Exists(candidatePaths[i]))
                    continue;

                IntPtr texturePtr = IntPtr.Zero;
                uint width = 0;
                uint height = 0;

                AddOrGetImagePointer(candidatePaths[i], false, out texturePtr, out width, out height);
                if (texturePtr != IntPtr.Zero)
                    return texturePtr;
            }

            return IntPtr.Zero;
        }

        private static string ResolveWav(string fileName)
        {
            string p = Path.Combine(AppContext.BaseDirectory, fileName);
            return File.Exists(p) ? p : "";
        }

        private static void PlayWav(string path)
        {
            Task.Run(() =>
            {
                try
                {
                    using (var audioFile = new AudioFileReader(path))
                    using (var output = new WaveOutEvent())
                    {
                        output.Init(audioFile);
                        output.Play();
                        while (output.PlaybackState == PlaybackState.Playing)
                            Task.Delay(10).Wait();
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine("Sound playback error: " + ex.Message);
                }
            });
        }

        private static void ResetLoaderVisualState()
        {
            _loaderTime = 0f;
            _connected = false;
            _connectedFade = 0f;
            _soundPlayed = false;
            _loaderAngle = 0f;
            _pulseAlpha = 0.5f;
            _pulseIncreasing = true;
        }

        private static void ResetFlowToLogin()
        {
            _flowPhase = FlowPhase.Login;
            _emulatorInitTask = null;
            _activeTab = 0;
            _showHookSuccessScreen = false;
            _hookSuccessAnimTime = 0f;
            ResetLoaderVisualState();
        }

        private static void AdvancePostLoginIfReady()
        {
            if (_flowPhase != FlowPhase.PostLoginLoad)
                return;
            if (!_connected || _loaderTime < 2.0f)
                return;
            if (_emulatorInitTask == null)
                return;
            if (!_emulatorInitTask.IsCompleted)
                return;

            if (_emulatorInitTask.IsFaulted)
            {
                _loginError = _emulatorInitTask.Exception?.GetBaseException().Message ?? "Init failed";
                ResetFlowToLogin();
                return;
            }

            string? err = _emulatorInitTask.Result;
            if (!string.IsNullOrEmpty(err))
            {
                _loginError = err;
                ResetFlowToLogin();
                return;
            }

            _showHookSuccessScreen = true;
            _hookSuccessAnimTime = 0f;
        }

        private static void DrawEmperorPanelBackground1(Vector2 position, Vector2 size, float rounding)
        {
            var draw = ImGui.GetWindowDrawList();
            Vector2 endPos = position + size;
            draw.AddRectFilled(position, endPos,
                ImGui.GetColorU32(new Vector4(PanelBg.X, PanelBg.Y, PanelBg.Z, PanelBg.W * _uiAlpha)),
                rounding);
            draw.AddRect(position, endPos,
                ImGui.GetColorU32(new Vector4(PanelBorder.X, PanelBorder.Y, PanelBorder.Z, PanelBorder.W * _uiAlpha)),
                rounding);
            ImGui.SetCursorPos(ImGui.GetCursorPos() + new Vector2(8, 8));
        }

        private static void DrawEmperorLoader()
        {
            if (_flowPhase != FlowPhase.PostLoginLoad)
                return;
            if (_showHookSuccessScreen)
                return;

            // If injection completes mid-frame (after RenderUI branch), prevent any loader draw flash.
            if (_emulatorInitTask != null && _emulatorInitTask.IsCompleted)
            {
                if (_emulatorInitTask.IsFaulted)
                {
                    _loginError = _emulatorInitTask.Exception?.GetBaseException().Message ?? "Init failed";
                    ResetFlowToLogin();
                    return;
                }

                string? err = _emulatorInitTask.Result;
                if (!string.IsNullOrEmpty(err))
                {
                    _loginError = err;
                    ResetFlowToLogin();
                    return;
                }

                _showHookSuccessScreen = true;
                _hookSuccessAnimTime = 0f;
                return;
            }

            float dt = ImGui.GetIO().DeltaTime;
            _loaderTime += dt;

            if (_loaderTime >= 1.0f && !_connected)
            {
                _connected = true;
                _loaderTime = 0f;
            }

            if (_connected)
            {
                _connectedFade += dt * 2.5f;
                if (_connectedFade > 1f)
                    _connectedFade = 1f;

                if (!_soundPlayed)
                {
                    string wav = ResolveWav("Notify.wav");
                    if (!string.IsNullOrEmpty(wav))
                        PlayWav(wav);
                    _soundPlayed = true;
                }
            }

            float availableHeight = ImGui.GetContentRegionAvail().Y;
            ImGui.PushStyleColor(ImGuiCol.ChildBg, new Vector4(PanelBg.X, PanelBg.Y, PanelBg.Z, 0f));
            ImGui.BeginChild("##loader_body", new Vector2(0, availableHeight),
                ImGuiChildFlags.None, ImGuiWindowFlags.NoBackground);

            var draw = ImGui.GetWindowDrawList();
            Vector2 loaderPos = ImGui.GetWindowPos();
            Vector2 loaderSize = ImGui.GetWindowSize();

            DrawEmperorPanelBackground1(loaderPos, loaderSize, 6f);

            Vector2 center = new Vector2(
                loaderPos.X + loaderSize.X * 0.5f,
                loaderPos.Y + loaderSize.Y * 0.5f
            );

            // Compact + smoother loader
            float radius = 28f;
            float thickness = 6f;
            _loaderAngle += dt * 3.2f;

            if (_pulseIncreasing)
            {
                _pulseAlpha += dt * 0.8f;
                if (_pulseAlpha >= 0.8f) _pulseIncreasing = false;
            }
            else
            {
                _pulseAlpha -= dt * 0.8f;
                if (_pulseAlpha <= 0.3f) _pulseIncreasing = true;
            }

            Vector4 neutralColor = new Vector4(1f, 1f, 1f, _uiAlpha);
            const float segmentLength = MathF.PI * 1.5f;
            float startAngle = _loaderAngle;
            Vector4 baseColor = new Vector4(1f, 1f, 1f, _uiAlpha);
            float endAngle = _loaderAngle + segmentLength;

            bool fullyConnected = _connected && _emulatorInitTask != null && _emulatorInitTask.IsCompleted;

            // Spinner: only while NOT connected (prevents any circular background behind the tick).
            if (!fullyConnected)
            {
                for (int i = 0; i < 8; i++)
                {
                    float offset = i * 0.2f;
                    float alpha = _pulseAlpha * (0.7f - i * 0.08f) * _uiAlpha;
                    if (alpha <= 0f) continue;

                    float a0 = startAngle - offset;
                    float a1 = endAngle - offset;

                    draw.PathArcTo(center, radius, a0, a1, 64);
                    draw.PathStroke(
                        ImGui.GetColorU32(new Vector4(baseColor.X, baseColor.Y, baseColor.Z, alpha)),
                        ImDrawFlags.None,
                        thickness
                    );

                    Vector2 pStart = new Vector2(
                        center.X + MathF.Cos(a0) * radius,
                        center.Y + MathF.Sin(a0) * radius
                    );
                    Vector2 pEnd = new Vector2(
                        center.X + MathF.Cos(a1) * radius,
                        center.Y + MathF.Sin(a1) * radius
                    );

                    draw.AddCircleFilled(pStart,
                        thickness * 0.5f,
                        ImGui.GetColorU32(new Vector4(baseColor.X, baseColor.Y, baseColor.Z, alpha)));
                    draw.AddCircleFilled(pEnd,
                        thickness * 0.5f,
                        ImGui.GetColorU32(new Vector4(baseColor.X, baseColor.Y, baseColor.Z, alpha)));
                }

                draw.PathArcTo(center, radius, startAngle, endAngle, 64);
                draw.PathStroke(
                    ImGui.GetColorU32(new Vector4(neutralColor.X, neutralColor.Y, neutralColor.Z, _pulseAlpha * _uiAlpha)),
                    ImDrawFlags.None, thickness
                );
            }

            // Connected state: animated green tick only (no circle)
            if (fullyConnected)
            {
                float t = Math.Clamp(_connectedFade, 0f, 1f);
                Vector4 tickCol = new Vector4(0.2f, 1f, 0.2f, t * _uiAlpha);
                uint tickU32 = ImGui.GetColorU32(tickCol);
                float tickThick = 5f;

                Vector2 p0 = center + new Vector2(-10f, 2f);
                Vector2 p1 = center + new Vector2(-2f, 10f);
                Vector2 p2 = center + new Vector2(14f, -6f);

                // Animate stroke draw: first segment then second.
                float t1 = Math.Clamp(t * 1.6f, 0f, 1f);
                float t2 = Math.Clamp((t - 0.55f) * 2.2f, 0f, 1f);
                Vector2 a = Vector2.Lerp(p0, p1, t1);
                draw.AddLine(p0, a, tickU32, tickThick);
                if (t2 > 0f)
                {
                    Vector2 b = Vector2.Lerp(p1, p2, t2);
                    draw.AddLine(p1, b, tickU32, tickThick);
                }
            }

            string text;
            Vector4 textColor;
            if (!_connected)
            {
                text = "Initializing...";
                textColor = new Vector4(1f, 1f, 1f, _pulseAlpha * _uiAlpha);
            }
            else if (_emulatorInitTask != null && !_emulatorInitTask.IsCompleted)
            {
                text = "ADB / injection running…";
                textColor = new Vector4(ThemeColor.X, ThemeColor.Y, ThemeColor.Z, _connectedFade * _uiAlpha);
            }
            else
            {
                text = "Connected : 5555";
                textColor = new Vector4(0.2f, 1f, 0.2f, _connectedFade * _uiAlpha);
            }

            ImGui.PushFont(TextFont);
            Vector2 textSize = ImGui.CalcTextSize(text);
            Vector2 textPos = new Vector2(center.X - textSize.X * 0.5f, center.Y + radius + 16f);
            for (int i = 3; i >= 1; i--)
            {
                float spread = i * 1.5f;
                float alpha = textColor.W * 0.25f / i;
                draw.AddText(
                    new Vector2(textPos.X - spread * 0.5f, textPos.Y),
                    ImGui.GetColorU32(new Vector4(textColor.X, textColor.Y, textColor.Z, alpha)),
                    text
                );
            }
            draw.AddText(textPos, ImGui.GetColorU32(textColor), text);
            ImGui.PopFont();

            ImGui.EndChild();
            ImGui.PopStyleColor();

            AdvancePostLoginIfReady();
        }

        private static void DrawHookSuccessScreen()
        {
            if (_flowPhase != FlowPhase.PostLoginLoad || !_showHookSuccessScreen)
                return;

            float dt = ImGui.GetIO().DeltaTime;
            _hookSuccessAnimTime += dt;
            float t = _hookSuccessAnimTime;

            float availH = ImGui.GetContentRegionAvail().Y;
            ImGui.PushStyleColor(ImGuiCol.ChildBg, new Vector4(PanelBg.X, PanelBg.Y, PanelBg.Z, 0f));
            ImGui.BeginChild("##hook_success_body", new Vector2(0, availH),
                ImGuiChildFlags.None, ImGuiWindowFlags.NoBackground);

            var draw = ImGui.GetWindowDrawList();
            Vector2 wp = ImGui.GetWindowPos();
            Vector2 ws = ImGui.GetWindowSize();
            DrawEmperorPanelBackground1(wp, ws, 6f);

            Vector2 center = new Vector2(wp.X + ws.X * 0.5f, wp.Y + ws.Y * 0.46f);

            static float EaseOutBack(float x)
            {
                const float c1 = 1.70158f;
                const float c3 = c1 + 1f;
                return 1f + c3 * MathF.Pow(x - 1f, 3f) + c1 * MathF.Pow(x - 1f, 2f);
            }

            static float EaseOutCubic(float x) => 1f - MathF.Pow(1f - x, 3f);

            // No circular background: only animated green tick.
            float tCircle = Math.Min(1f, t / 0.38f);
            float circleScale = EaseOutBack(tCircle);
            circleScale = Math.Min(circleScale, 1.08f);

            const float baseR = 44f;
            float ringR = baseR * circleScale;

            float tStrokeRaw = Math.Max(0f, t - 0.32f) / 0.62f;
            float strokeProg = EaseOutCubic(Math.Clamp(tStrokeRaw, 0f, 1f));
            float chkAlpha = MathF.Min(1f, (t - 0.28f) / 0.15f) * _uiAlpha;
            uint chkCol = ImGui.GetColorU32(new Vector4(0.2f, 1f, 0.2f, chkAlpha));

            float k = ringR * 0.52f;
            Vector2 p1 = center + new Vector2(-0.52f * k, 0.02f * k);
            Vector2 p2 = center + new Vector2(-0.1f * k, 0.44f * k);
            Vector2 p3 = center + new Vector2(0.56f * k, -0.46f * k);

            float len1 = Vector2.Distance(p1, p2);
            float len2 = Vector2.Distance(p2, p3);
            float total = len1 + len2;
            float drawLen = total * strokeProg;
            const float lineThick = 4.6f;

            if (drawLen > 0f && len1 > 0.001f)
            {
                float u = MathF.Min(drawLen, len1) / len1;
                Vector2 q = p1 + (p2 - p1) * u;
                draw.AddLine(p1, q, chkCol, lineThick);
            }
            if (drawLen > len1 && len2 > 0.001f)
            {
                float u = MathF.Min(drawLen - len1, len2) / len2;
                Vector2 q = p2 + (p3 - p2) * u;
                draw.AddLine(p2, q, chkCol, lineThick);
            }

            float capA = strokeProg * chkAlpha * 0.35f;
            if (capA > 0.01f && drawLen > 0.01f)
            {
                uint capCol = ImGui.GetColorU32(new Vector4(0.2f, 1f, 0.2f, capA));
                float rad = lineThick * 0.5f;
                if (drawLen < len1)
                {
                    float u = MathF.Min(drawLen, len1) / len1;
                    Vector2 tip = p1 + (p2 - p1) * u;
                    draw.AddCircleFilled(tip, rad, capCol, 12);
                }
                else
                {
                    float u = MathF.Min(drawLen - len1, len2) / len2;
                    Vector2 tip = p2 + (p3 - p2) * u;
                    draw.AddCircleFilled(tip, rad, capCol, 12);
                }
            }

            float textT = Math.Clamp((t - 0.58f) / 0.42f, 0f, 1f);
            float textEase = 1f - MathF.Pow(1f - textT, 2.5f);
            float textAlpha = textEase * _uiAlpha;
            float yLift = (1f - textEase) * 18f;
            string hookMsg = "Successfully Hooked!";
            ImGui.PushFont(LebelFont.IsLoaded() ? LebelFont : TextFont);
            Vector2 msgSz = ImGui.CalcTextSize(hookMsg);
            Vector2 msgPos = new Vector2(
                center.X - msgSz.X * 0.5f,
                center.Y + ringR + 28f - yLift);
            for (int i = 3; i >= 1; i--)
            {
                float a = textAlpha * 0.12f / i;
                draw.AddText(
                    msgPos + new Vector2(i, i * 0.5f),
                    ImGui.GetColorU32(new Vector4(0f, 0f, 0f, a)),
                    hookMsg);
            }
            draw.AddText(msgPos, ImGui.GetColorU32(new Vector4(1f, 1f, 1f, textAlpha)), hookMsg);
            ImGui.PopFont();

            ImGui.EndChild();
            ImGui.PopStyleColor();

            if (t >= HookSuccessAnimDuration)
            {
                _flowPhase = FlowPhase.Main;
                _activeTab = 0;
                _showHookSuccessScreen = false;
                _hookSuccessAnimTime = 0f;
                _ = BootstrapRuntime.StartCheatsAsync();
                CustomNotification.NotifyMessage("Notification", "Success — emulator ready (port 5555)", 4.5f);
            }
        }

        private void DrawEmbeddedLoginPanel()
        {
            float contentW = ImGui.GetContentRegionAvail().X;
            const float topMargin = 10f;
            const float sidePad = 10f;
            const float maxFormW = 360f;
            float w = Math.Min(maxFormW, contentW - sidePad * 2f);
            if (w < 120f)
                w = Math.Max(100f, contentW - 12f);

            Vector2 colLeft = ImGui.GetCursorPos() + new Vector2(sidePad, topMargin);
            ImGui.SetCursorPos(colLeft);

            const float btnH = 32f;
            const float btnPadX = 34f;

            if (LebelFont.IsLoaded())
                ImGui.PushFont(LebelFont);
            Vector2 loginTitleSz = ImGui.CalcTextSize("Login");
            ImGui.PushStyleColor(ImGuiCol.Text, new Vector4(1f, 1f, 1f, _uiAlpha));
            ImGui.TextUnformatted("Login");
            ImGui.PopStyleColor();
            if (LebelFont.IsLoaded())
                ImGui.PopFont();

            float btnW = MathF.Max(loginTitleSz.X + btnPadX, 92f);

            ImGui.SetCursorPos(new Vector2(colLeft.X, ImGui.GetCursorPos().Y + 8f));

            ImGui.PushStyleVar(ImGuiStyleVar.FrameRounding, 2f);
            ImGui.PushStyleVar(ImGuiStyleVar.FramePadding, new Vector2(10f, 10f));
            ImGui.PushStyleColor(ImGuiCol.FrameBg, new Vector4(0.04f, 0.04f, 0.05f, _uiAlpha));
            ImGui.PushStyleColor(ImGuiCol.FrameBgHovered, new Vector4(0.06f, 0.06f, 0.07f, _uiAlpha));
            ImGui.PushStyleColor(ImGuiCol.FrameBgActive, new Vector4(0.07f, 0.07f, 0.08f, _uiAlpha));
            ImGui.PushStyleColor(ImGuiCol.Border, new Vector4(0.22f, 0.22f, 0.24f, 0.9f * _uiAlpha));
            ImGui.PushStyleVar(ImGuiStyleVar.FrameBorderSize, 1f);
            ImGui.SetNextItemWidth(w);
            ImGui.InputText("##emb_license", ref _loginLicense, 384);
            ImGui.PopStyleVar(2);
            ImGui.PopStyleColor(4);

            ImGui.SetCursorPos(new Vector2(colLeft.X, ImGui.GetCursorPos().Y + 10f));

            var draw = ImGui.GetWindowDrawList();
            Vector2 btnMin = ImGui.GetCursorScreenPos();
            Vector2 btnMax = btnMin + new Vector2(btnW, btnH);

            uint cTop = ImGui.GetColorU32(new Vector4(0.13f, 0.13f, 0.15f, _uiAlpha));
            uint cBot = ImGui.GetColorU32(new Vector4(0.05f, 0.05f, 0.06f, _uiAlpha));
            draw.AddRectFilledMultiColor(btnMin, btnMax, cTop, cTop, cBot, cBot);
            uint hiTop = ImGui.GetColorU32(new Vector4(0.38f, 0.38f, 0.42f, 0.55f * _uiAlpha));
            draw.AddRectFilled(btnMin, new Vector2(btnMin.X + btnW, btnMin.Y + 1.5f), hiTop);
            uint borderCol = ImGui.GetColorU32(new Vector4(0.22f, 0.22f, 0.26f, 0.9f * _uiAlpha));
            draw.AddRect(btnMin, btnMax, borderCol, 2f);
            uint innerShadow = ImGui.GetColorU32(new Vector4(0f, 0f, 0f, 0.35f * _uiAlpha));
            draw.AddLine(new Vector2(btnMin.X + 1f, btnMax.Y - 1f), new Vector2(btnMax.X - 1f, btnMax.Y - 1f), innerShadow, 1f);

            ImGui.SetCursorScreenPos(btnMin);
            ImGui.InvisibleButton("##login_hex_btn", new Vector2(btnW, btnH));
            bool btnPressed = ImGui.IsItemClicked(ImGuiMouseButton.Left);
            if (btnPressed && !_loginBusy)
            {
                string lic = _loginLicense.Trim();
                if (string.IsNullOrEmpty(lic))
                    _loginError = "Enter license key.";
                else
                {
                    _loginBusy = true;
                    _loginError = "";
                    Task.Run(() =>
                    {
                        try
                        {
                            BootstrapRuntime.Auth.license(lic);
                            _loginOk = BootstrapRuntime.Auth.response.success;
                            _loginResultMsg = BootstrapRuntime.Auth.response.message ?? "";
                        }
                        catch (Exception ex)
                        {
                            _loginOk = false;
                            _loginResultMsg = ex.Message;
                        }
                        finally
                        {
                            _loginBusy = false;
                            _loginDone = true;
                        }
                    });
                }
            }

            if (TextFont.IsLoaded())
                ImGui.PushFont(TextFont);
            else if (LebelFont.IsLoaded())
                ImGui.PushFont(LebelFont);
            Vector2 loginSz = ImGui.CalcTextSize("Login");
            Vector2 loginPos = new Vector2(
                btnMin.X + (btnW - loginSz.X) * 0.5f,
                btnMin.Y + (btnH - loginSz.Y) * 0.5f);
            draw.AddText(loginPos, ImGui.GetColorU32(new Vector4(1f, 1f, 1f, 0.98f * _uiAlpha)), "Login");
            if (TextFont.IsLoaded() || LebelFont.IsLoaded())
                ImGui.PopFont();

            ImGui.SetCursorPos(new Vector2(colLeft.X, ImGui.GetCursorPos().Y + 14f));

            string statusLine;
            if (_loginBusy)
                statusLine = "Signing in...";
            else if (!string.IsNullOrEmpty(_loginError))
                statusLine = _loginError;
            else
                statusLine = "Enter license key.";

            ImGui.PushStyleColor(ImGuiCol.Text, new Vector4(0.95f, 0.95f, 0.96f, 0.92f * _uiAlpha));
            if (TextFont.IsLoaded())
                ImGui.PushFont(TextFont);
            ImGui.TextUnformatted("Status: ");
            ImGui.SameLine(0f, 0f);
            ImGui.TextUnformatted(statusLine);
            if (TextFont.IsLoaded())
                ImGui.PopFont();
            ImGui.PopStyleColor();
        }

        public void RenderUI()
        {
            if (_uiAlpha <= 0.001f && !_uiVisible)
                return;

            ApplyThemeOverlay();

            if (_flowPhase == FlowPhase.Main)
                DrawBottomCenterTextSafe();

            ImGui.SetNextWindowSize(new Vector2(520, 390), ImGuiCond.Always);
            ImGui.SetNextWindowPos(new Vector2(50, 50) + _emulatorScreenOffset, ImGuiCond.FirstUseEver);
            ImGui.SetNextWindowBgAlpha(0.92f * _uiAlpha);

            ImGui.Begin("##EmperorExternal",
                ImGuiWindowFlags.NoResize |
                ImGuiWindowFlags.NoCollapse |
                ImGuiWindowFlags.NoScrollbar |
                ImGuiWindowFlags.NoTitleBar |
                ImGuiWindowFlags.NoBringToFrontOnFocus);

            var wdraw = ImGui.GetWindowDrawList();
            Vector2 wpos = ImGui.GetWindowPos();
            Vector2 wsize = ImGui.GetWindowSize();

            wdraw.AddRectFilled(wpos, wpos + wsize,
                ImGui.GetColorU32(new Vector4(PanelBg.X, PanelBg.Y, PanelBg.Z, 0.97f * _uiAlpha)),
                8f);

            DrawHeader();

            ImGui.BeginChild("##body_cs", new Vector2(0, 0), ImGuiChildFlags.None,
                ImGuiWindowFlags.NoScrollbar | ImGuiWindowFlags.NoScrollWithMouse);
            ImGui.BeginChild("##content_cs", new Vector2(0, 0), ImGuiChildFlags.None,
                ImGuiWindowFlags.NoScrollbar | ImGuiWindowFlags.NoScrollWithMouse);
            DrawPanelBackground(2f);

            if (_flowPhase == FlowPhase.Login)
                DrawEmbeddedLoginPanel();
            else if (_flowPhase == FlowPhase.PostLoginLoad)
            {
                // Decide success-screen state BEFORE rendering this frame to prevent flicker.
                if (!_showHookSuccessScreen && _emulatorInitTask != null && _emulatorInitTask.IsCompleted)
                {
                    if (_emulatorInitTask.IsFaulted)
                    {
                        _loginError = _emulatorInitTask.Exception?.GetBaseException().Message ?? "Init failed";
                        ResetFlowToLogin();
                    }
                    else
                    {
                        string? err = _emulatorInitTask.Result;
                        if (!string.IsNullOrEmpty(err))
                        {
                            _loginError = err;
                            ResetFlowToLogin();
                        }
                        else
                        {
                            _showHookSuccessScreen = true;
                            _hookSuccessAnimTime = 0f;
                        }
                    }
                }

                if (_showHookSuccessScreen)
                    DrawHookSuccessScreen();
                else
                    DrawEmperorLoader();
            }
            else
                DrawMainContent();

            ImGui.EndChild();

            ImGui.EndChild();

            if (_flowPhase == FlowPhase.Main)
                DrawTabPopup();

            if (_flowPhase == FlowPhase.Main)
            {
                _mainPanelPos = wpos;
                _mainPanelSize = wsize;
            }

            ImGui.End();

            if (_flowPhase == FlowPhase.Main && _uiAlpha > 0.001f)
                DrawFloatingTabsWindow(_mainPanelPos);

            if (_flowPhase == FlowPhase.Main && _activeTab == 1 && Config.ESPEditor && _uiAlpha > 0.001f)
            {
                EnsureEspEditorPreview();
                const float gap = 12f;
                ImGui.SetNextWindowPos(new Vector2(_mainPanelPos.X + _mainPanelSize.X + gap, _mainPanelPos.Y), ImGuiCond.Always);
                ImGui.SetNextWindowSize(new Vector2(340, _mainPanelSize.Y), ImGuiCond.Always);
                float panelA = 0.78f * Math.Clamp(_uiAlpha, 0f, 1f);
                Vector4 panelBg = new Vector4(0.09f, 0.09f, 0.1f, panelA);
                ImGui.PushStyleColor(ImGuiCol.WindowBg, panelBg);
                ImGui.PushStyleColor(ImGuiCol.TitleBg, panelBg);
                ImGui.PushStyleColor(ImGuiCol.TitleBgActive, panelBg);
                ImGui.PushStyleColor(ImGuiCol.Border, new Vector4(0.04f, 0.04f, 0.05f, panelA));
                ImGui.Begin("ESP EDITOR", ImGuiWindowFlags.NoResize | ImGuiWindowFlags.NoCollapse);
                DrawEspEditorPreviewPanel();
                ImGui.End();
                ImGui.PopStyleColor(4);
            }
        }

        private void PushVisualsComboChrome()
        {
            float a = Math.Clamp(_uiAlpha, 0f, 1f);
            // Compact professional combo: dark fill, thin #333 border, low rounding (matches reference UI).
            ImGui.PushStyleVar(ImGuiStyleVar.FramePadding, new Vector2(8f, 5f));
            ImGui.PushStyleVar(ImGuiStyleVar.ItemInnerSpacing, new Vector2(6f, 0f));
            ImGui.PushStyleVar(ImGuiStyleVar.FrameBorderSize, 1f);
            ImGui.PushStyleVar(ImGuiStyleVar.FrameRounding, 4f);
            Vector4 fill = new Vector4(0.1f, 0.1f, 0.11f, a);
            Vector4 fillH = new Vector4(0.13f, 0.13f, 0.14f, a);
            Vector4 fillA = new Vector4(0.12f, 0.12f, 0.13f, a);
            Vector4 border = new Vector4(51f / 255f, 51f / 255f, 51f / 255f, a);
            ImGui.PushStyleColor(ImGuiCol.FrameBg, fill);
            ImGui.PushStyleColor(ImGuiCol.FrameBgHovered, fillH);
            ImGui.PushStyleColor(ImGuiCol.FrameBgActive, fillA);
            ImGui.PushStyleColor(ImGuiCol.Button, fill);
            ImGui.PushStyleColor(ImGuiCol.ButtonHovered, fillH);
            ImGui.PushStyleColor(ImGuiCol.ButtonActive, fillA);
            ImGui.PushStyleColor(ImGuiCol.Border, border);
            ImGui.PushStyleColor(ImGuiCol.Text, new Vector4(1f, 1f, 1f, a));
            ImGui.PushStyleColor(ImGuiCol.NavHighlight, new Vector4(1f, 1f, 1f, 0.12f * a));
            ImGui.PushStyleColor(ImGuiCol.Header, new Vector4(0.14f, 0.14f, 0.16f, a));
            ImGui.PushStyleColor(ImGuiCol.HeaderHovered, new Vector4(0.18f, 0.18f, 0.2f, a));
            ImGui.PushStyleColor(ImGuiCol.HeaderActive, new Vector4(0.2f, 0.2f, 0.22f, a));
            ImGui.PushStyleColor(ImGuiCol.TextSelectedBg, new Vector4(0.22f, 0.22f, 0.26f, 0.55f * a));
        }

        private void PopVisualsComboChrome()
        {
            ImGui.PopStyleColor(13);
            ImGui.PopStyleVar(4);
        }

        private static void PushVisualsPopupChrome()
        {
            ImGui.PushStyleColor(ImGuiCol.PopupBg, new Vector4(0.08f, 0.08f, 0.09f, 0.98f));
            ImGui.PushStyleColor(ImGuiCol.Border, new Vector4(51f / 255f, 51f / 255f, 51f / 255f, 1f));
            ImGui.PushStyleColor(ImGuiCol.Header, new Vector4(0.12f, 0.12f, 0.14f, 1f));
            ImGui.PushStyleColor(ImGuiCol.HeaderHovered, new Vector4(0.16f, 0.16f, 0.18f, 1f));
            ImGui.PushStyleColor(ImGuiCol.HeaderActive, new Vector4(0.18f, 0.18f, 0.2f, 1f));
            ImGui.PushStyleVar(ImGuiStyleVar.PopupRounding, 3f);
            ImGui.PushStyleVar(ImGuiStyleVar.PopupBorderSize, 1f);
        }

        private static void PopVisualsPopupChrome()
        {
            ImGui.PopStyleVar(2);
            ImGui.PopStyleColor(5);
        }

        private void StepVisualsAnim(ref float a, float tgt)
        {
            float dt = ImGui.GetIO().DeltaTime;
            a += (tgt - a) * Math.Min(1f, dt * 14f);
        }

        private void DrawVisualsComboStrip(string childId, float anim, float fullH, bool showContent, Action drawCombo)
        {
            if (anim <= 0.01f) return;

            float eased = anim * anim * (3f - 2f * anim);

            float maxH = 20f;
            float h = Math.Max(1f, eased * Math.Min(fullH, maxH));

            float availW = ImGui.GetContentRegionAvail().X;
            float width = availW * 0.6f;
            float offsetX = 25f; // keep horizontal placement fixed; user asked for vertical centering
            float topGap = 8f;
            ImGui.SetCursorPosY(ImGui.GetCursorPosY() + topGap);
            ImGui.SetCursorPosX(ImGui.GetCursorPosX() + offsetX);

            Vector4 bg = new Vector4(0.1f, 0.1f, 0.11f, 0.98f);

            ImGui.PushStyleColor(ImGuiCol.WindowBg, bg);
            ImGui.PushStyleColor(ImGuiCol.ChildBg, bg);

            ImGui.PushStyleVar(ImGuiStyleVar.ChildRounding, 6f);

            //  FIXED: less top padding (removes dark strip)
            ImGui.PushStyleVar(ImGuiStyleVar.WindowPadding, new Vector2(6f, 3f));

            ImGui.PushStyleVar(ImGuiStyleVar.ItemSpacing, new Vector2(4f, 4f));

            ImGui.BeginChild(childId, new Vector2(width, h), ImGuiChildFlags.None,
                ImGuiWindowFlags.NoScrollbar | ImGuiWindowFlags.NoScrollWithMouse);

            if (showContent && h > 15f)
            {
                ImGui.PushStyleVar(ImGuiStyleVar.Alpha, eased);

                //  CLEAN spacing (no dark artifact)
                ImGui.SetCursorPosY(ImGui.GetCursorPosY() + 1f);

                // Compact frame with clear arrow area.
                ImGui.PushStyleVar(ImGuiStyleVar.FramePadding, new Vector2(8f, 5f));

                drawCombo();

                ImGui.PopStyleVar(); // FramePadding
                ImGui.PopStyleVar(); // Alpha
            }

            ImGui.EndChild();

            ImGui.Dummy(new Vector2(0f, 1f));

            ImGui.PopStyleVar(3);
            ImGui.PopStyleColor(2);
        }
        private void DrawEspEditorPreviewPanel()
        {
            if (_espEditorPreviewPtr == IntPtr.Zero || _espEditorPreviewW == 0 || _espEditorPreviewH == 0)
            {
                ImGui.TextWrapped("esp_editor_preview.png not found (embedded or Resources\\Icons next to exe).");
                return;
            }

            Vector2 availCanvas = ImGui.GetContentRegionAvail();
            float iw = _espEditorPreviewW;
            float ih = _espEditorPreviewH;
            float scaleImg = Math.Min(availCanvas.X / iw, Math.Max(96f, availCanvas.Y) / ih);
            // Slightly shorter preview for better fit/balance in editor panel.
            scaleImg *= 0.89f;
            Vector2 sz = new Vector2(iw * scaleImg, ih * scaleImg);

            ImGui.SetNextItemAllowOverlap();
            ImGui.InvisibleButton("##esp_editor_canvas", new Vector2(availCanvas.X, sz.Y));
            Vector2 canvasMin = ImGui.GetItemRectMin();
            Vector2 canvasMax = ImGui.GetItemRectMax();
            Vector2 mouse = ImGui.GetIO().MousePos;
            bool mouseInCanvas = mouse.X >= canvasMin.X && mouse.X <= canvasMax.X
                && mouse.Y >= canvasMin.Y && mouse.Y <= canvasMax.Y;
            // Geometric hit only: overlay + stacked windows often fail IsItemHovered/IsWindowHovered for the canvas.
            bool canvasClickOk = mouseInCanvas;
            float padImgX = Math.Max(0f, (availCanvas.X - sz.X) * 0.5f);
            Vector2 imgMin = new Vector2(canvasMin.X + padImgX, canvasMin.Y);
            Vector2 imgMax = imgMin + sz;

            Vector2 imgCenter = (imgMin + imgMax) * 0.5f;
            float imgH = imgMax.Y - imgMin.Y;
            // Preview PNG: align head/feet to character silhouette (line top = above head, not panel edge)
            const float headFrac = 0.06f;
            const float feetFrac = 0.985f;
            Vector2 headScreen = new Vector2(imgCenter.X, imgMin.Y + imgH * headFrac);
            Vector2 bottomScreen = new Vector2(imgCenter.X, imgMin.Y + imgH * feetFrac);
            float cornerHeight = MathF.Max(1f, Math.Abs(headScreen.Y - bottomScreen.Y));
            float cornerWidth = cornerHeight * 0.65f;

            Vector2 offLineStart = ScaleEspOffsetPx(Config.EspSnapLineStartOffsetPx, cornerHeight);
            Vector2 offLineEnd = ScaleEspOffsetPx(Config.EspSnapLineEndOffsetPx, cornerHeight);
            Vector2 offBox = ScaleEspOffsetPx(Config.EspBoxOffsetPx, cornerHeight);
            Vector2 offName = ScaleEspOffsetPx(Config.EspNameOffsetPx, cornerHeight);
            Vector2 offHacker = ScaleEspOffsetPx(Config.EspHackerOffsetPx, cornerHeight);
            Vector2 offWeapon = ScaleEspOffsetPx(Config.EspWeaponIconOffsetPx, cornerHeight);
            Vector2 offHealth = ScaleEspOffsetPx(Config.EspHealthBarOffsetPx, cornerHeight);
            Vector2 offDist = ScaleEspOffsetPx(Config.EspDistanceOffsetPx, cornerHeight);

            Vector2 baseLineStart = Config.ESPLinePosition switch
            {
                LinePosition.Center => new Vector2(imgCenter.X, (imgMin.Y + imgMax.Y) * 0.5f),
                LinePosition.Bottom => new Vector2(imgCenter.X, imgMax.Y),
                _ => new Vector2(imgCenter.X, headScreen.Y - 20f),
            };
            Vector2 lineStart = baseLineStart + offLineStart;
            Vector2 lineEndAnchor = Config.ESPLinePosition == LinePosition.Bottom ? bottomScreen : headScreen;
            Vector2 endPos = lineEndAnchor + offLineEnd;

            GetEspBoxBounds(headScreen, cornerWidth, cornerHeight, offBox,
                out float boxL, out float boxT, out float boxR, out float boxB, out float boxMidX, out float boxMidY);
            float gapScaled = Math.Max(3f, EspAnchorGapPx * (cornerHeight / EspOffsetRefCornerHeight));

            bool showOverlay = Config.ESPLine || Config.ESPBox || Config.ESPInformation || Config.ESPName || Config.espdistance
                || (Config.ESPHealth && !Config.ESPInformation) || Config.ESPHealthText || Config.espweapon || Config.HackerTag;
            if (!showOverlay)
                return;

            bool eInfo = Config.ESPInformation;
            bool eNameSimple = Config.ESPName && !Config.ESPInformation;
            bool eHealth = Config.ESPHealth && !Config.ESPInformation;
            bool eWeapon = Config.espweapon;
            bool eBox = Config.ESPBox;
            bool eLine = Config.ESPLine;

            string nameDisplay = SanitizeEspDisplayName("Training Bot");
            string distStr = "15m";
            bool showDist = Config.espdistance;
            EspDistanceBoxAnchor distAnchor = Config.EspDistanceAnchor;
            if (!eInfo && (distAnchor == EspDistanceBoxAnchor.InsideNameplate || distAnchor == EspDistanceBoxAnchor.AboveNameWhenNameAtTop))
                distAnchor = EspDistanceBoxAnchor.BoxTopCenter;
            bool distInNameplate = showDist && eInfo && distAnchor == EspDistanceBoxAnchor.InsideNameplate;
            float npW = 0f;
            float npH = 0f;
            Vector2 npTL = Vector2.Zero;
            if (eInfo)
            {
                if (distInNameplate)
                    ComputeEspNameplateSize(nameDisplay, distStr, out npW, out npH);
                else
                    ComputeEspNameplateSizeNameOnly(nameDisplay, out npW, out npH);
                npTL = GetNameBoxAnchorTopLeft(Config.EspNameAnchor, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled, npW, npH) + offName;
            }

            Vector2 nameAnchorSize = eNameSimple ? EspCalcTextSizeName(nameDisplay) : new Vector2(60, 14);
            Vector2 namePos = GetNameBoxAnchorTopLeft(Config.EspNameAnchor, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled, nameAnchorSize.X, nameAnchorSize.Y) + offName;

            float healthBarWidth = 3f;
            float healthBarFullHeight = cornerHeight;
            float healthPercentage = 0.75f;
            Vector2 healthBarPosRight = new Vector2(boxR + 5f, boxT) + offHealth;
            Vector2 healthHitMin = healthBarPosRight;
            Vector2 healthHitMax = healthBarPosRight + new Vector2(healthBarWidth, healthBarFullHeight);

            Vector2 weaponIconPos = Vector2.Zero;
            Vector2 weaponIconSz = Vector2.Zero;
            if (eWeapon)
            {
                Vector2 iconSize = new Vector2(60 * Config.iconsize, 20 * Config.iconsize);
                weaponIconSz = iconSize;
                EspNameBoxAnchor wMap = MapWeaponAnchorToNameAnchor(Config.EspWeaponAnchor);
                weaponIconPos = GetNameBoxAnchorTopLeft(wMap, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled, iconSize.X, iconSize.Y) + offWeapon;
            }

            Vector2 hackerPos = Vector2.Zero;
            Vector2 hackerSz = Vector2.Zero;
            if (Config.HackerTag)
            {
                string hackerText = "HACKER";
                hackerSz = ImGui.CalcTextSize(hackerText);
                float anchorW = eInfo ? npW : nameAnchorSize.X;
                float anchorX = eInfo ? npTL.X : namePos.X;
                float anchorTopY = eInfo ? npTL.Y : namePos.Y;
                hackerPos = new Vector2(
                    anchorX + (anchorW - hackerSz.X) * 0.5f,
                    anchorTopY - hackerSz.Y - 3f);
                hackerPos += offHacker;
                // Keep hacker label visible inside preview area.
                hackerPos.Y = MathF.Max(imgMin.Y + 2f, hackerPos.Y);
            }

            Vector2 simpleNamePos = Vector2.Zero;
            Vector2 simpleNameSz = Vector2.Zero;
            if (eNameSimple)
            {
                simpleNameSz = EspCalcTextSizeName(nameDisplay);
                simpleNamePos = GetNameBoxAnchorTopLeft(Config.EspNameAnchor, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled, simpleNameSz.X, simpleNameSz.Y) + offName;
            }

            float nameSlotW = 0f, nameSlotH = 0f;
            if (eInfo && npW > 0f)
            {
                nameSlotW = npW;
                nameSlotH = npH;
            }
            else if (eNameSimple && simpleNameSz.X > 0f)
            {
                nameSlotW = MathF.Max(48f, simpleNameSz.X);
                nameSlotH = MathF.Max(14f, simpleNameSz.Y);
            }
            Vector2 distSzEditor = showDist ? ImGui.CalcTextSize(distStr) : Vector2.Zero;

            if (eHealth)
                EspEditorGetHealthBarDrawHitBounds(Config.EspHealthAnchor, boxL, boxT, boxR, boxB, cornerWidth, cornerHeight, gapScaled, healthBarWidth, offHealth, out healthHitMin, out healthHitMax);

            float inv = EspOffsetRefCornerHeight / cornerHeight;
            Vector2 boxTL = new Vector2(headScreen.X - (cornerWidth / 2) + offBox.X, headScreen.Y + offBox.Y);
            Vector2 boxBR = boxTL + new Vector2(cornerWidth, cornerHeight);

            const float nameHitPad = 16f;
            const float npHitPad = 12f;
            const float distHitPad = 6f;
            Vector2 editorDistHitMin = Vector2.Zero;
            Vector2 editorDistHitMax = Vector2.Zero;
            bool editorDistDraggable = showDist && !distInNameplate && showOverlay;
            if (editorDistDraggable)
            {
                Vector2 distSzHit = ImGui.CalcTextSize(distStr);
                Vector2 distTLHit = GetDistanceBoxAnchorTopLeft(distAnchor, Config.EspNameAnchor, distSzHit, npTL, npW, npH, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled) + offDist;
                editorDistHitMin = distTLHit;
                editorDistHitMax = distTLHit + distSzHit;
            }

            EspEditorColorTarget hoveredColorTarget = EspEditorColorTarget.None;
            if (canvasClickOk)
            {
                if (eWeapon && weaponIconSz.X > 0f && EspEditorPointInRect(mouse, weaponIconPos, weaponIconPos + weaponIconSz))
                    hoveredColorTarget = EspEditorColorTarget.Weapon;
                else if (Config.HackerTag && hackerSz.X > 0f && EspEditorPointInRect(mouse, hackerPos, hackerPos + hackerSz))
                    hoveredColorTarget = EspEditorColorTarget.Hacker;
                else if (eInfo && npW > 0f && EspEditorPointInRectPad(mouse, npTL, npTL + new Vector2(npW, npH), npHitPad))
                    hoveredColorTarget = EspEditorColorTarget.Name;
                else if (eNameSimple && simpleNameSz.X > 0f && EspEditorPointInRectPad(mouse, simpleNamePos, simpleNamePos + simpleNameSz, nameHitPad))
                    hoveredColorTarget = EspEditorColorTarget.Name;
                else if (eHealth && EspEditorPointInRect(mouse, healthHitMin, healthHitMax))
                    hoveredColorTarget = EspEditorColorTarget.Health;
                else if (eBox && EspEditorPointInRectPad(mouse, boxTL, boxBR, 6f))
                    hoveredColorTarget = EspEditorColorTarget.Box;
                else if (eLine && EspEditorDistToSegmentSq(mouse, lineStart, endPos) <= (12f * 12f))
                    hoveredColorTarget = EspEditorColorTarget.Line;
            }

            if (ImGui.IsMouseClicked(ImGuiMouseButton.Left) && canvasClickOk)
            {
                _espEditorColorClickTarget = hoveredColorTarget;
                _espEditorColorClickStart = mouse;
                _espEditorDragKind = EspEditorDragKind.None;

                if (eInfo && npW > 0f)
                {
                    Vector2 npBR = npTL + new Vector2(npW, npH);
                    if (EspEditorPointInRectPad(mouse, npTL, npBR, npHitPad))
                        _espEditorDragKind = EspEditorDragKind.Name;
                }
                if (_espEditorDragKind == EspEditorDragKind.None && eNameSimple && simpleNameSz.X > 0f
                    && EspEditorPointInRectPad(mouse, simpleNamePos, simpleNamePos + simpleNameSz, nameHitPad))
                    _espEditorDragKind = EspEditorDragKind.Name;

                if (_espEditorDragKind == EspEditorDragKind.None && eWeapon && weaponIconSz.X > 0f && EspEditorPointInRect(mouse, weaponIconPos, weaponIconPos + weaponIconSz))
                    _espEditorDragKind = EspEditorDragKind.Weapon;
                if (_espEditorDragKind == EspEditorDragKind.None && Config.HackerTag && hackerSz.X > 0f && EspEditorPointInRect(mouse, hackerPos, hackerPos + hackerSz))
                    _espEditorDragKind = EspEditorDragKind.Hacker;
                if (_espEditorDragKind == EspEditorDragKind.None && Config.ESPHealthText && !Config.ESPInformation && eHealth)
                {
                    Vector2 hts = ImGui.CalcTextSize("200");
                    Vector2 hpTextPos = new Vector2(healthHitMin.X - hts.X - 6f, healthHitMin.Y + (healthHitMax.Y - healthHitMin.Y) * 0.5f - hts.Y * 0.5f);
                    if (EspEditorPointInRect(mouse, hpTextPos, hpTextPos + hts))
                        _espEditorDragKind = EspEditorDragKind.Health;
                }
                if (_espEditorDragKind == EspEditorDragKind.None && eHealth
                    && EspEditorPointInRect(mouse, healthHitMin, healthHitMax))
                    _espEditorDragKind = EspEditorDragKind.Health;
            }

            if (_espEditorDragKind != EspEditorDragKind.None && ImGui.IsMouseDown(ImGuiMouseButton.Left))
            {
                Vector2 md = ImGui.GetIO().MouseDelta;
                Vector2 ed = md * inv;
                switch (_espEditorDragKind)
                {
                    // Snap-only editor: no free pixel placement.
                    case EspEditorDragKind.Name:
                    case EspEditorDragKind.Health:
                    case EspEditorDragKind.Weapon:
                        break;
                }
            }

            bool holdName = _espEditorDragKind == EspEditorDragKind.Name
                && (ImGui.IsMouseDown(ImGuiMouseButton.Left) || ImGui.IsMouseReleased(ImGuiMouseButton.Left));
            bool holdDist = _espEditorDragKind == EspEditorDragKind.Distance
                && (ImGui.IsMouseDown(ImGuiMouseButton.Left) || ImGui.IsMouseReleased(ImGuiMouseButton.Left));
            bool holdHealth = _espEditorDragKind == EspEditorDragKind.Health
                && (ImGui.IsMouseDown(ImGuiMouseButton.Left) || ImGui.IsMouseReleased(ImGuiMouseButton.Left));
            bool holdWeapon = _espEditorDragKind == EspEditorDragKind.Weapon
                && (ImGui.IsMouseDown(ImGuiMouseButton.Left) || ImGui.IsMouseReleased(ImGuiMouseButton.Left));

            var dl = ImGui.GetWindowDrawList();
            dl.AddImage(_espEditorPreviewPtr, imgMin, imgMax, Vector2.Zero, Vector2.One,
                ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 1f, 1f, 1f)));

            if (_espEditorDragKind == EspEditorDragKind.None && hoveredColorTarget != EspEditorColorTarget.None)
            {
                Vector2 iconPos = mouse + new Vector2(10f, -8f);
                Vector2 iconSize = new Vector2(16f, 16f);
                Vector2 iconMax = iconPos + iconSize;
                dl.AddRectFilled(iconPos, iconMax, ImGui.ColorConvertFloat4ToU32(new Vector4(0.08f, 0.08f, 0.08f, 0.9f)), 4f);
                dl.AddRect(iconPos, iconMax, ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 1f, 1f, 0.32f)), 4f);
                if (TextFont.IsLoaded())
                    ImGui.PushFont(TextFont);
                dl.AddText(iconPos + new Vector2(2f, 1f), ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 1f, 1f, 0.92f)), "\ue176");
                if (TextFont.IsLoaded())
                    ImGui.PopFont();
            }

            uint lineColor = ColorToUint32(Config.ESPLineColor);
            uint boxColor = ColorToUint32(Config.ESPBoxColor);
            if (Config.ESPRGB)
            {
                Color rainbow = ColorFromHSV((float)(ImGui.GetTime() * 100 % 360), 1.0f, 1.0f);
                lineColor = ColorToUint32(rainbow);
                boxColor = lineColor;
            }

            if (eLine)
                dl.AddLine(lineStart, endPos, lineColor, 1.5f);

            if (eBox)
                DrawCorneredBoxDrawList(dl, headScreen.X - (cornerWidth / 2) + offBox.X, headScreen.Y + offBox.Y, cornerWidth, cornerHeight, boxColor, 1f);

            Vector2 mouseForSlots = ImGui.GetIO().MousePos;
            EspNameBoxAnchor? nameSlotHi = holdName && nameSlotW > 0f
                ? EspEditorNameSlotUnderMouse(mouseForSlots, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled, nameSlotW, nameSlotH)
                : null;
            if (eBox && nameSlotW > 0f && holdName)
                DrawEspEditorNameSnapSlots(dl, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled, nameSlotW, nameSlotH, Config.EspNameAnchor, nameSlotHi);

            EspDistanceBoxAnchor? distSlotHi = null;
            if (showDist && holdDist)
            {
                distSlotHi = EspEditorDistanceSlotUnderMouse(mouseForSlots, Config.EspNameAnchor, distSzEditor, npTL, npW, npH, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled);
                DrawEspEditorDistanceSnapSlots(dl, Config.EspNameAnchor, distSzEditor, npTL, npW, npH, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled,
                    distAnchor, distSlotHi);
            }

            EspHealthBoxAnchor? healthSlotHi = null;
            if (eHealth && eBox && holdHealth)
            {
                healthSlotHi = EspEditorHealthSlotUnderMouse(mouseForSlots, boxL, boxT, boxR, boxB, cornerWidth, cornerHeight, gapScaled, healthBarWidth);
                DrawEspEditorHealthSnapSlots(dl, boxL, boxT, boxR, boxB, cornerWidth, cornerHeight, gapScaled, healthBarWidth, Config.EspHealthAnchor, healthSlotHi);
            }

            EspWeaponBoxAnchor? weaponSlotHi = null;
            if (eWeapon && eBox && weaponIconSz.X > 0f && holdWeapon)
            {
                weaponSlotHi = EspEditorWeaponSlotUnderMouse(mouseForSlots, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled, weaponIconSz);
                DrawEspEditorWeaponSnapSlots(dl, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled, weaponIconSz, Config.EspWeaponAnchor, weaponSlotHi);
            }

            if (eInfo)
            {
                DrawEspInfoNameplate(dl, npTL, npW, nameDisplay, distStr, 8, healthPercentage, false, showDist && distInNameplate);
                if (showDist && !distInNameplate)
                {
                    Vector2 distSz = ImGui.CalcTextSize(distStr);
                    Vector2 distTL = GetDistanceBoxAnchorTopLeft(distAnchor, Config.EspNameAnchor, distSz, npTL, npW, npH, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled) + offDist;
                    dl.AddText(distTL, ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 0.93f, 0.2f, 1f)), distStr);
                }
            }
            else if (eNameSimple)
            {
                if (TextFont.IsLoaded()) ImGui.PushFont(TextFont);
                dl.AddText(simpleNamePos, ColorToUint32(Config.ESPNameColor), nameDisplay);
                if (TextFont.IsLoaded()) ImGui.PopFont();
            }

            if (!eInfo && showDist)
            {
                Vector2 distSz = ImGui.CalcTextSize(distStr);
                Vector2 distTL = GetDistanceBoxAnchorTopLeft(distAnchor, Config.EspNameAnchor, distSz, npTL, npW, npH, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled) + offDist;
                dl.AddText(distTL, ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 0.93f, 0.2f, 1f)), distStr);
            }

            if (Config.HackerTag)
            {
                uint hackerCol = ColorToUint32(Config.HackerTagColor);
                dl.AddText(hackerPos + new Vector2(1, 1), ColorToUint32(Color.Black), "HACKER");
                dl.AddText(hackerPos, hackerCol, "HACKER");
            }

            if (eWeapon)
            {
                EnsureEspEditorWeaponPreviewIcon();
                if (_espEditorWeaponPreviewPtr != IntPtr.Zero)
                {
                    dl.AddImage(_espEditorWeaponPreviewPtr, weaponIconPos, weaponIconPos + weaponIconSz,
                        Vector2.Zero, Vector2.One, ImGui.ColorConvertFloat4ToU32(Config.ICONCOLOR));
                }
                else
                {
                    uint ph = ImGui.ColorConvertFloat4ToU32(new Vector4(0.4f, 0.7f, 1f, 0.45f));
                    dl.AddRectFilled(weaponIconPos, weaponIconPos + weaponIconSz, ph);
                    dl.AddRect(weaponIconPos, weaponIconPos + weaponIconSz, ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 1f, 1f, 0.6f)));
                }
            }

            if (eHealth)
            {
                float t = healthBarWidth;
                uint healthBarColor = ColorToUint32(Color.Lime);
                switch (Config.EspHealthAnchor)
                {
                    case EspHealthBoxAnchor.BoxTop:
                        {
                            float y0 = boxT - gapScaled - t;
                            Vector2 o = offHealth;
                            healthHitMin = new Vector2(boxL + o.X, y0 + o.Y);
                            healthHitMax = new Vector2(boxR + o.X, y0 + t + o.Y);
                            dl.AddRect(healthHitMin - new Vector2(1, 1), healthHitMax + new Vector2(1, 1), 0xFF000000, 1.0f);
                            dl.AddRectFilled(healthHitMin, healthHitMax, 0x90000000, 1.0f);
                            dl.AddRectFilled(healthHitMin, new Vector2(boxL + cornerWidth * healthPercentage + o.X, y0 + t + o.Y), healthBarColor, 1.0f);
                            break;
                        }
                    case EspHealthBoxAnchor.BoxBottom:
                        {
                            float y0 = boxB + gapScaled;
                            Vector2 o = offHealth;
                            healthHitMin = new Vector2(boxL + o.X, y0 + o.Y);
                            healthHitMax = new Vector2(boxR + o.X, y0 + t + o.Y);
                            dl.AddRect(healthHitMin - new Vector2(1, 1), healthHitMax + new Vector2(1, 1), 0xFF000000, 1.0f);
                            dl.AddRectFilled(healthHitMin, healthHitMax, 0x90000000, 1.0f);
                            dl.AddRectFilled(healthHitMin, new Vector2(boxL + cornerWidth * healthPercentage + o.X, y0 + t + o.Y), healthBarColor, 1.0f);
                            break;
                        }
                    case EspHealthBoxAnchor.BoxLeft:
                        {
                            Vector2 p = new Vector2(boxL - 5f - t, boxT) + offHealth;
                            healthHitMin = p;
                            healthHitMax = p + new Vector2(t, cornerHeight);
                            dl.AddRect(healthHitMin - new Vector2(1, 1), healthHitMax + new Vector2(1, 1), 0xFF000000, 1.0f);
                            dl.AddRectFilled(healthHitMin, healthHitMax, 0x90000000, 1.0f);
                            dl.AddRectFilled(new Vector2(p.X, p.Y + cornerHeight * (1 - healthPercentage)), healthHitMax, healthBarColor, 1.0f);
                            break;
                        }
                    default:
                        {
                            healthBarPosRight = new Vector2(boxR + 5f, boxT) + offHealth;
                            healthHitMin = healthBarPosRight;
                            healthHitMax = healthBarPosRight + new Vector2(t, healthBarFullHeight);
                            dl.AddRect(healthHitMin - new Vector2(1, 1), healthHitMax + new Vector2(1, 1), 0xFF000000, 1.0f);
                            dl.AddRectFilled(healthHitMin, healthHitMax, 0x90000000, 1.0f);
                            dl.AddRectFilled(new Vector2(healthHitMin.X, healthHitMin.Y + healthBarFullHeight * (1 - healthPercentage)), healthHitMax, healthBarColor, 1.0f);
                            break;
                        }
                }
            }

            if (Config.ESPHealthText && !Config.ESPInformation && eHealth)
            {
                string ht = "200";
                Vector2 hts = ImGui.CalcTextSize(ht);
                Vector2 hpTextPos = new Vector2(healthHitMin.X - hts.X - 6f, healthHitMin.Y + (healthHitMax.Y - healthHitMin.Y) * 0.5f - hts.Y * 0.5f);
                dl.AddText(hpTextPos, ColorToUint32(Config.ESPHealthColor), ht);
            }

            if (holdName
                && ((eInfo && npW > 0f) || (eNameSimple && simpleNameSz.X > 0f)) && nameSlotW > 0f)
            {
                Vector2 offN = ScaleEspOffsetPx(Config.EspNameOffsetPx, cornerHeight);
                Vector2 attach = GetEspNameAnchorAttachPoint(Config.EspNameAnchor, boxL, boxT, boxR, boxB, boxMidX, boxMidY);
                Vector2 nameCenter;
                if (eInfo && npW > 0f)
                {
                    Vector2 nptl = GetNameBoxAnchorTopLeft(Config.EspNameAnchor, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled, npW, npH) + offN;
                    nameCenter = nptl + new Vector2(npW * 0.5f, npH * 0.5f);
                }
                else
                {
                    Vector2 sptl = GetNameBoxAnchorTopLeft(Config.EspNameAnchor, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled, simpleNameSz.X, simpleNameSz.Y) + offN;
                    nameCenter = sptl + simpleNameSz * 0.5f;
                }
                uint lc = ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 1f, 1f, 0.6f));
                dl.AddLine(attach, nameCenter, lc, 1.35f);
            }

            if (holdDist && showDist)
            {
                Vector2 offD = ScaleEspOffsetPx(Config.EspDistanceOffsetPx, cornerHeight);
                Vector2 ds = ImGui.CalcTextSize(distStr);
                Vector2 dtl = GetDistanceBoxAnchorTopLeft(distAnchor, Config.EspNameAnchor, ds, npTL, npW, npH, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled) + offD;
                Vector2 dCenter = dtl + ds * 0.5f;
                Vector2 dAttach = GetEspDistanceAnchorAttachPoint(distAnchor, Config.EspNameAnchor, npTL, npW, npH, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled);
                uint dlc = ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 0.92f, 0.35f, 0.65f));
                dl.AddLine(dAttach, dCenter, dlc, 1.25f);
            }

            if (holdHealth && eHealth)
            {
                Vector2 hAttach = GetEspHealthAnchorAttachPoint(Config.EspHealthAnchor, boxL, boxT, boxR, boxB, boxMidX, boxMidY, cornerHeight);
                Vector2 hCenter = (healthHitMin + healthHitMax) * 0.5f;
                uint hlc = ImGui.ColorConvertFloat4ToU32(new Vector4(0.45f, 1f, 0.5f, 0.65f));
                dl.AddLine(hAttach, hCenter, hlc, 1.25f);
            }

            if (holdWeapon && eWeapon && weaponIconSz.X > 0f)
            {
                Vector2 wAttach = GetEspNameAnchorAttachPoint(MapWeaponAnchorToNameAnchor(Config.EspWeaponAnchor), boxL, boxT, boxR, boxB, boxMidX, boxMidY);
                Vector2 wCenter = weaponIconPos + weaponIconSz * 0.5f;
                uint wlc = ImGui.ColorConvertFloat4ToU32(new Vector4(0.5f, 0.82f, 1f, 0.65f));
                dl.AddLine(wAttach, wCenter, wlc, 1.25f);
            }

            // Drag ghost: follows cursor until snapped on release.
            Vector2 ghostPos = ImGui.GetIO().MousePos + new Vector2(16f, 12f);
            if (holdName)
            {
                if (eInfo && npW > 0f)
                {
                    Vector2 gsz = new Vector2(npW, npH);
                    Vector2 gtl = ghostPos - gsz * 0.5f;
                    Vector2 gbr = gtl + gsz;
                    dl.AddRectFilled(gtl + new Vector2(2f, 2f), gbr + new Vector2(2f, 2f), ImGui.ColorConvertFloat4ToU32(new Vector4(0f, 0f, 0f, 0.45f)), 6f);
                    DrawEspInfoNameplate(dl, gtl, npW, nameDisplay, distStr, 8, healthPercentage, false, distInNameplate);
                }
                else if (eNameSimple && simpleNameSz.X > 0f)
                {
                    if (TextFont.IsLoaded()) ImGui.PushFont(TextFont);
                    dl.AddText(ghostPos + new Vector2(2f, 2f), ImGui.ColorConvertFloat4ToU32(new Vector4(0f, 0f, 0f, 0.65f)), nameDisplay);
                    dl.AddText(ghostPos, ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 1f, 1f, 0.9f)), nameDisplay);
                    if (TextFont.IsLoaded()) ImGui.PopFont();
                }
            }

            if (holdHealth && eHealth)
            {
                Vector2 hbSz = new Vector2(Math.Max(40f, cornerWidth), Math.Max(6f, healthBarWidth));
                Vector2 hbTl = ghostPos - hbSz * 0.5f;
                Vector2 hbBr = hbTl + hbSz;
                float fillW = hbSz.X * Math.Clamp(healthPercentage, 0.05f, 1f);
                dl.AddRectFilled(hbTl + new Vector2(2f, 2f), hbBr + new Vector2(2f, 2f), ImGui.ColorConvertFloat4ToU32(new Vector4(0f, 0f, 0f, 0.45f)), 4f);
                dl.AddRectFilled(hbTl, hbBr, ImGui.ColorConvertFloat4ToU32(new Vector4(0.1f, 0.1f, 0.1f, 0.7f)), 4f);
                dl.AddRectFilled(hbTl, new Vector2(hbTl.X + fillW, hbBr.Y), ImGui.ColorConvertFloat4ToU32(new Vector4(0.2f, 1f, 0.2f, 0.88f)), 4f);
                dl.AddRect(hbTl, hbBr, ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 1f, 1f, 0.45f)), 4f);
            }

            if (holdWeapon && eWeapon && weaponIconSz.X > 0f)
            {
                Vector2 wTl = ghostPos - weaponIconSz * 0.5f;
                Vector2 wBr = wTl + weaponIconSz;
                dl.AddRectFilled(wTl + new Vector2(2f, 2f), wBr + new Vector2(2f, 2f), ImGui.ColorConvertFloat4ToU32(new Vector4(0f, 0f, 0f, 0.45f)), 4f);
                if (_espEditorWeaponPreviewPtr != IntPtr.Zero)
                {
                    dl.AddImage(_espEditorWeaponPreviewPtr, wTl, wBr, Vector2.Zero, Vector2.One,
                        ImGui.ColorConvertFloat4ToU32(new Vector4(Config.ICONCOLOR.X, Config.ICONCOLOR.Y, Config.ICONCOLOR.Z, 0.9f)));
                }
                else
                {
                    dl.AddRectFilled(wTl, wBr, ImGui.ColorConvertFloat4ToU32(new Vector4(0.4f, 0.7f, 1f, 0.55f)));
                    dl.AddRect(wTl, wBr, ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 1f, 1f, 0.6f)));
                }
            }

            if (ImGui.IsMouseReleased(ImGuiMouseButton.Left))
            {
                Vector2 relMouse = ImGui.GetIO().MousePos;
                float clickMoveSq = EspEditorDistSq(relMouse, _espEditorColorClickStart);
                if (_espEditorColorClickTarget != EspEditorColorTarget.None && clickMoveSq <= (5f * 5f))
                {
                    _espEditorColorTarget = _espEditorColorClickTarget;
                    _espEditorColorPopupPos = relMouse + new Vector2(14f, 10f);
                    _espEditorColorPopupOpen = true;
                }
                _espEditorColorClickTarget = EspEditorColorTarget.None;
                EspEditorDragKind released = _espEditorDragKind;
                if (released == EspEditorDragKind.Name && nameSlotW > 0f
                    && TrySnapEspNameAnchorAtMouse(relMouse, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled, nameSlotW, nameSlotH, out EspNameBoxAnchor pickedName))
                {
                    if (!IsOccupiedByOthers(pickedName, excludeName: true, excludeHealth: false, excludeWeapon: false))
                        Config.EspNameAnchor = pickedName;
                    Config.EspNameOffsetPx = Vector2.Zero;
                }
                if (released == EspEditorDragKind.Health && eHealth
                    && TrySnapEspHealthAnchorAtMouse(relMouse, boxL, boxT, boxR, boxB, cornerWidth, cornerHeight, gapScaled, healthBarWidth, out EspHealthBoxAnchor pickedHealth))
                {
                    EspNameBoxAnchor healthSlot = MapHealthAnchorToNameAnchor(pickedHealth);
                    if (!IsOccupiedByOthers(healthSlot, excludeName: false, excludeHealth: true, excludeWeapon: false))
                        Config.EspHealthAnchor = pickedHealth;
                    Config.EspHealthBarOffsetPx = Vector2.Zero;
                }
                if (released == EspEditorDragKind.Weapon && eWeapon && weaponIconSz.X > 0f
                    && TrySnapEspWeaponAnchorAtMouse(relMouse, boxL, boxT, boxR, boxB, boxMidX, boxMidY, gapScaled, weaponIconSz, out EspWeaponBoxAnchor pickedWeapon))
                {
                    EspNameBoxAnchor weaponSlot = MapWeaponAnchorToNameAnchor(pickedWeapon);
                    if (!IsOccupiedByOthers(weaponSlot, excludeName: false, excludeHealth: false, excludeWeapon: true))
                        Config.EspWeaponAnchor = pickedWeapon;
                    Config.EspWeaponIconOffsetPx = Vector2.Zero;
                }
                _espEditorDragKind = EspEditorDragKind.None;
            }

            if (_espEditorColorPopupOpen && _espEditorColorTarget != EspEditorColorTarget.None)
            {
                ImGui.SetNextWindowPos(_espEditorColorPopupPos, ImGuiCond.Always);
                ImGui.SetNextWindowBgAlpha(0.96f);
                ImGui.PushStyleVar(ImGuiStyleVar.WindowPadding, new Vector2(8f, 8f));
                ImGui.PushStyleVar(ImGuiStyleVar.WindowRounding, 6f);
                ImGui.PushStyleColor(ImGuiCol.Border, new Vector4(0.25f, 0.25f, 0.28f, 1f));
                bool keepOpen = true;
                bool popupHovered = false;
                if (ImGui.Begin("##esp_editor_color_popup", ref keepOpen,
                    ImGuiWindowFlags.NoResize | ImGuiWindowFlags.AlwaysAutoResize | ImGuiWindowFlags.NoCollapse | ImGuiWindowFlags.NoTitleBar))
                {
                    popupHovered = ImGui.IsWindowHovered(ImGuiHoveredFlags.RootAndChildWindows);
                    ImGui.TextUnformatted($"{GetEspEditorColorTargetLabel(_espEditorColorTarget)} Color");
                    if (TryGetEspEditorTargetColor(_espEditorColorTarget, out Vector4 picked))
                    {
                        if (ImGui.ColorPicker4("##esp_editor_color_picker", ref picked,
                            ImGuiColorEditFlags.NoSidePreview | ImGuiColorEditFlags.AlphaBar | ImGuiColorEditFlags.AlphaPreview))
                        {
                            SetEspEditorTargetColor(_espEditorColorTarget, picked);
                        }
                    }

                    if (ImGui.Button("Close"))
                        _espEditorColorPopupOpen = false;
                }
                ImGui.End();
                ImGui.PopStyleColor();
                ImGui.PopStyleVar(2);
                // Hide when user clicks anywhere in editor panel except inside picker popup.
                if (ImGui.IsMouseClicked(ImGuiMouseButton.Left) && canvasClickOk && !popupHovered)
                    _espEditorColorPopupOpen = false;
                if (!keepOpen)
                    _espEditorColorPopupOpen = false;
            }
        }

        void EnsureEspEditorPreview()
        {
            if (_espEditorPreviewPtr != IntPtr.Zero)
                return;
            if (_espEditorPreviewLoadAttempted)
                return;
            _espEditorPreviewLoadAttempted = true;
            const string embeddedName = "pixel.Resources.Icons.esp_editor_preview.png";
            try
            {
                Assembly asm = Assembly.GetExecutingAssembly();
                Stream? stream = asm.GetManifestResourceStream(embeddedName);
                if (stream == null)
                {
                    // Assembly name can vary (pixel/CAX/etc), so resolve by suffix.
                    string[] resourceNames = asm.GetManifestResourceNames();
                    for (int i = 0; i < resourceNames.Length; i++)
                    {
                        if (resourceNames[i].EndsWith("Resources.Icons.esp_editor_preview.png", StringComparison.OrdinalIgnoreCase))
                        {
                            stream = asm.GetManifestResourceStream(resourceNames[i]);
                            if (stream != null)
                                break;
                        }
                    }
                }
                if (stream == null)
                {
                    string? dir = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
                    if (dir != null)
                    {
                        string[] candidates =
                        {
                            Path.Combine(dir, "Resources", "Icons", "esp_editor_preview.png"),
                            Path.Combine(AppContext.BaseDirectory, "Resources", "Icons", "esp_editor_preview.png"),
                            Path.Combine(Directory.GetCurrentDirectory(), "Resources", "Icons", "esp_editor_preview.png"),
                            Path.Combine(dir, "..", "Resources", "Icons", "esp_editor_preview.png"),
                            Path.Combine(dir, "..", "..", "Resources", "Icons", "esp_editor_preview.png"),
                        };

                        for (int i = 0; i < candidates.Length; i++)
                        {
                            string p = Path.GetFullPath(candidates[i]);
                            if (!File.Exists(p))
                                continue;
                            stream = File.OpenRead(p);
                            break;
                        }
                    }
                }

                if (stream == null)
                    return;

                using (stream)
                using (var bitmap = new System.Drawing.Bitmap(stream))
                {
                    string tempPath = Path.Combine(Path.GetTempPath(), $"esp_editor_{Guid.NewGuid()}.png");
                    bitmap.Save(tempPath);
                    AddOrGetImagePointer(tempPath, true, out var ptr, out var w, out var h);
                    try { File.Delete(tempPath); } catch { }
                    if (ptr != IntPtr.Zero)
                    {
                        _espEditorPreviewPtr = ptr;
                        _espEditorPreviewW = w;
                        _espEditorPreviewH = h;
                    }
                }
            }
            catch
            {
                // keep ptr zero
            }
        }

        private void EnsureEspEditorWeaponPreviewIcon()
        {
            if (_espEditorWeaponPreviewPtr != IntPtr.Zero)
                return;
            if (_espEditorWeaponPreviewLoadAttempted)
                return;
            _espEditorWeaponPreviewLoadAttempted = true;
            string[] tryNames = { "ak47.png", "m4a1.png", "fist.png" };
            try
            {
                Assembly asm = Assembly.GetExecutingAssembly();
                foreach (string fileName in tryNames)
                {
                    Stream? stream = null;

                    // 1) Direct legacy resource name.
                    stream = asm.GetManifestResourceStream($"pixel.Resources.Icons.{fileName}");

                    // 2) Assembly-name agnostic suffix match.
                    if (stream == null)
                    {
                        string[] resourceNames = asm.GetManifestResourceNames();
                        for (int i = 0; i < resourceNames.Length; i++)
                        {
                            if (resourceNames[i].EndsWith($"Resources.Icons.{fileName}", StringComparison.OrdinalIgnoreCase))
                            {
                                stream = asm.GetManifestResourceStream(resourceNames[i]);
                                if (stream != null)
                                    break;
                            }
                        }
                    }

                    // 3) File-system fallbacks.
                    if (stream == null)
                    {
                        string? dir = Path.GetDirectoryName(asm.Location);
                        if (dir != null)
                        {
                            string[] candidates =
                            {
                                Path.Combine(dir, "Resources", "Icons", fileName),
                                Path.Combine(AppContext.BaseDirectory, "Resources", "Icons", fileName),
                                Path.Combine(Directory.GetCurrentDirectory(), "Resources", "Icons", fileName),
                                Path.Combine(dir, "..", "Resources", "Icons", fileName),
                                Path.Combine(dir, "..", "..", "Resources", "Icons", fileName),
                            };

                            for (int i = 0; i < candidates.Length; i++)
                            {
                                string p = Path.GetFullPath(candidates[i]);
                                if (!File.Exists(p))
                                    continue;
                                stream = File.OpenRead(p);
                                break;
                            }
                        }
                    }
                    if (stream == null)
                        continue;
                    using (stream)
                    using (var bitmap = new System.Drawing.Bitmap(stream))
                    {
                        string tempPath = Path.Combine(Path.GetTempPath(), $"esp_editor_wep_{Guid.NewGuid()}.png");
                        bitmap.Save(tempPath);
                        AddOrGetImagePointer(tempPath, true, out var ptr, out var w, out var h);
                        try { File.Delete(tempPath); } catch { }
                        if (ptr != IntPtr.Zero)
                        {
                            _espEditorWeaponPreviewPtr = ptr;
                            _espEditorWeaponPreviewW = w;
                            _espEditorWeaponPreviewH = h;
                            return;
                        }
                    }
                }
            }
            catch
            {
            }
        }

        private async Task<bool> ApplyMemoryPatch(CX memory, string[] searchPatterns, string[] replacePatterns)
        {
            bool success = false;

            for (int i = 0; i < searchPatterns.Length; i++)
            {
                if (string.IsNullOrEmpty(searchPatterns[i]) || string.IsNullOrEmpty(replacePatterns[i])) continue;

                var matches = await memory.AoBScan(searchPatterns[i]);
                if (matches.Any())
                {
                    foreach (long id in matches)
                    {
                        memory.AobReplace(id, replacePatterns[i]);
                    }
                    success = true;
                }
                else
                {
                }
            }

            return success;
        }
        private void NeonSeparator(float thickness, Vector4 glowColor)
        {
            Vector2 start = ImGui.GetCursorScreenPos();
            Vector2 end = new Vector2(start.X + ImGui.GetContentRegionAvail().X, start.Y);

            // Glow effect
            ImGui.GetWindowDrawList().AddLine(
                start, end,
                ImGui.GetColorU32(new Vector4(glowColor.X, glowColor.Y, glowColor.Z, glowColor.W * 0.3f)),
                thickness + 4.0f
            );

            // Main line
            ImGui.GetWindowDrawList().AddLine(
                start, end,
                ImGui.GetColorU32(glowColor),
                thickness
            );

            ImGui.Dummy(new Vector2(0, thickness + 6));
        }
        private async Task flyahackk()
        {
            string processName = "HD-Player";
            CX memoryfast = new CX();

            if (!memoryfast.SetProcess(new[] { processName }))
            {
                return;
            }

            string[] searchPatterns =
            {
    "AC C5 27 37 00 10 A0 E1"
};

            string[] replacePatterns =
            {
    "AC C5 A9 3F 00 10 A0 E1"
};

            try
            {
                if (Config.flyhack)
                {
                    bool success = await ApplyMemoryPatch(memoryfast, searchPatterns, replacePatterns);
                }
                else
                {
                    bool success = await ApplyMemoryPatch(memoryfast, replacePatterns, searchPatterns);
                }
            }
            catch (Exception ex)
            {
            }
        }

        private async Task highjump()
        {
            string processName = "HD-Player";
            CX memoryfast = new CX();

            if (!memoryfast.SetProcess(new[] { processName }))
            {
                return;
            }

            string[] searchPatterns =
            {
            "00 00 C8 42 F0 48 2D E9 10 B0 8D E2 01 50 A0 E1 00 40 A0 E1 00 00 55 E3 02 00 00 0A BC 70 D5 E1"
};

            string[] replacePatterns =
            {
            "00 00 7A 43 F0 48 2D E9 10 B0 8D E2 01 50 A0 E1 00 40 A0 E1 00 00 55 E3 02 00 00 0A BC 70 D5 E1"
};

            try
            {
                if (Config.highjumppp)
                {
                    bool success = await ApplyMemoryPatch(memoryfast, searchPatterns, replacePatterns);
                }
                else
                {
                    bool success = await ApplyMemoryPatch(memoryfast, replacePatterns, searchPatterns);
                }
            }
            catch (Exception ex)
            {
            }
        }

        private async Task cameraleftt()
        {
            string processName = "HD-Player";
            CX memoryfast = new CX();

            if (!memoryfast.SetProcess(new[] { processName }))
            {
                return;
            }

            string[] searchPatterns =
            {
            "00 00 00 00 00 00 80 3F 00 00 00 00 00 00 00 00 00 00 80 BF 00 00 00 00 00 00 80 BF 00 00 00 00 00 00 00 00 00 00 80 3F 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 80 3F 00 00 00 00 00 00 00 00 00 00 80 BF 00 00 80 FF"
};

            string[] replacePatterns =
            {
            "00 00 00 00 00 00 30 40 00 00 00 00 00 00 00 00 00 00 80 BF 00 00 00 00 00 00 80 BF 00 00 00 00 00 00 00 00 00 00 80 3F 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 80 3F 00 00 00 00 00 00 00 00 00 00 80 BF 00 00 80 7F 00 00 80 7F 00 00 80 7F 00 00 80 FF"
};

            try
            {
                if (Config.cameraleftt)
                {
                    bool success = await ApplyMemoryPatch(memoryfast, searchPatterns, replacePatterns);
                }
                else
                {
                    bool success = await ApplyMemoryPatch(memoryfast, replacePatterns, searchPatterns);
                }
            }
            catch (Exception ex)
            {
            }
        }

        private async Task visionhack()
        {
            string processName = "HD-Player";
            CX memoryfast = new CX();

            if (!memoryfast.SetProcess(new[] { processName }))
            {
                return;
            }

            string[] searchPatterns =
            {
            "DB 0F 49 40 10 2A 00 EE 00"
};

            string[] replacePatterns =
            {
            "DB 0F 99 40 10 2A 00 EE 00"
};

            try
            {
                if (Config.visionhackkk)
                {
                    bool success = await ApplyMemoryPatch(memoryfast, searchPatterns, replacePatterns);
                }
                else
                {
                    bool success = await ApplyMemoryPatch(memoryfast, replacePatterns, searchPatterns);
                }
            }
            catch (Exception ex)
            {
            }
        }
        private async Task wallhackrisk()
        {
            string processName = "HD-Player";
            CX memoryfast = new CX();

            if (!memoryfast.SetProcess(new[] { processName }))
            {
                return;
            }

            string[] searchPatterns =
            {
            "3F AE 47 81 3F AE 47 81 3F AE 47 81 3F 00 1A B7 EE DC 3A 9F ED 30 00 4F E2"
};

            string[] replacePatterns =
            {
            "3F AE 47 81 3F AE 47 81 BF AE 47 81 3F AE 1A B7 EE DC 3A 9F ED 30 00 4F E2"
};

            try
            {
                if (Config.wallhack)
                {
                    bool success = await ApplyMemoryPatch(memoryfast, searchPatterns, replacePatterns);
                }
                else
                {
                    bool success = await ApplyMemoryPatch(memoryfast, replacePatterns, searchPatterns);
                }
            }
            catch (Exception ex)
            {
            }
        }

        private async Task brustfire()
        {
            string processName = "HD-Player";
            CX memoryfast = new CX();

            if (!memoryfast.SetProcess(new[] { processName }))
            {
                return;
            }

            string[] searchPatterns =
            {
  "00 00 80 40 33 33 93 40 3D 0A F7 3F",
    "02 2B 07 3D 02 2B 07 3D 02 2B 07 3D",};

            string[] replacePatterns =
            {
   "00 00 80 40 00 00 80 40 CB D2 4D 3E",
    "08 39 60 3B 08 39 60 3B 08 39 85 3B",};

            try
            {
                if (Config.brsutfiree)
                {
                    bool success = await ApplyMemoryPatch(memoryfast, searchPatterns, replacePatterns);
                }
                else
                {
                    bool success = await ApplyMemoryPatch(memoryfast, replacePatterns, searchPatterns);
                }
            }
            catch (Exception ex)
            {
            }
        }
        private IEnumerable<long> speedResult;

        private async Task loadspeedext()
        {
            string[] pocessname = { "HD-Player" };
            bool success = memoryfast.SetProcess(pocessname);

            if (!success)
            {
                return;
            }

            speedResult = await memoryfast.AoBScan("02 2B 07 3D 02 2B 07 3D 02 2B 07 3D");
            speedonext();
            Console.Beep(400, 200);
        }

        private void speedonext()
        {
            if (Config.speedext)
            {
                foreach (long id in speedResult)
                {
                    memoryfast.AobReplace(id, "E3 A5 9B 3C");
                }

                Console.Beep(400, 200);
            }
            else
            {
                foreach (long id in speedResult)
                {
                    memoryfast.AobReplace(id, "02 2B 07 3D 02 2B 07 3D 02 2B 07 3D");
                }

                Console.Beep(400, 200);
            }
        }
        private bool GradientTabButton(string label, bool active, float width, Vector4 activeColor, Vector4 inactiveColor, Vector4 hoverColor)
        {

            Vector4 color = active ? activeColor : inactiveColor;
            Vector2 size = new Vector2(width, 30);

            ImGui.PushStyleColor(ImGuiCol.Button, color);
            ImGui.PushStyleColor(ImGuiCol.ButtonHovered, hoverColor);
            ImGui.PushStyleColor(ImGuiCol.ButtonActive, activeColor * 0.8f);

            Vector2 p_min = ImGui.GetCursorScreenPos();
            Vector2 p_max = new Vector2(p_min.X + size.X, p_min.Y + size.Y);
            ImGui.GetWindowDrawList().AddRectFilledMultiColor(
                p_min, p_max,
                ImGui.GetColorU32(color * 1.1f),
                ImGui.GetColorU32(color * 0.9f),
                ImGui.GetColorU32(color * 0.9f),
                ImGui.GetColorU32(color * 1.1f)
            );

            if (active)
            {
                // Blue border highlight
                ImGui.GetWindowDrawList().AddRect(
                    p_min, p_max,
                    ImGui.GetColorU32(new Vector4(0.4f, 0.6f, 1.0f, 0.5f)),
                    0, ImDrawFlags.None, 2.0f
                );
            }

            bool clicked = ImGui.Button(label, size);

            ImGui.PopStyleColor(3);
            return clicked;
        }

        private bool GradientSliderFloat(string label, ref float value, float min, float max)
        {
            Vector4 colorStart = new Vector4(1f, 0.72f, 0.74f, 1.0f);
            Vector4 colorEnd = new Vector4(0.42f, 0.06f, 0.09f, 1.0f);

            bool changed = false;
            float sliderWidth = ImGui.CalcItemWidth();
            Vector2 cursorPos = ImGui.GetCursorScreenPos();
            float frameHeight = ImGui.GetFrameHeight();


            for (float x = 0; x < sliderWidth; x += 2.0f)
            {
                float t = x / sliderWidth;
                Vector4 color = Lerp(colorStart, colorEnd, t);
                ImGui.GetWindowDrawList().AddRectFilled(
                    new Vector2(cursorPos.X + x, cursorPos.Y),
                    new Vector2(cursorPos.X + x + 2, cursorPos.Y + frameHeight),
                    ImGui.GetColorU32(color)
                );
            }


            ImGui.SetCursorScreenPos(cursorPos);
            changed = ImGui.SliderFloat(label, ref value, min, max);


            if (ImGui.IsItemActive())
            {
                Vector2 grabPos = new Vector2(
                    cursorPos.X + (value - min) / (max - min) * sliderWidth,
                    cursorPos.Y + frameHeight / 2
                );
                ImGui.GetWindowDrawList().AddCircleFilled(
                    grabPos,
                    8.0f,
                    ImGui.GetColorU32(new Vector4(1, 1, 1, 0.3f))
                );
                ImGui.GetWindowDrawList().AddCircle(
                    grabPos,
                    8.0f,
                    ImGui.GetColorU32(new Vector4(0.55f, 0.56f, 0.6f, 1f)),
                    0,
                    2.0f
                );
            }

            return changed;
        }

        private const float ContentLabelWidth = 48f;
        private const float ContentSliderWidth = 280f;

        private bool FilledSliderFloat(string label, ref float value, float min, float max)
        {
            float sw = ContentSliderWidth;
            float fh = 22f;
            Vector2 cur = ImGui.GetCursorScreenPos();
            Vector2 pmin = new Vector2(cur.X, cur.Y), pmax = new Vector2(cur.X + sw, cur.Y + fh);
            float t = (max > min) ? Math.Clamp((value - min) / (max - min), 0, 1) : 0;
            uint fillCol = ImGui.GetColorU32(new Vector4(0.48f, 0.5f, 0.54f, 1f));
            uint bgCol = ImGui.GetColorU32(new Vector4(0.25f, 0.25f, 0.3f, 1f));
            ImGui.GetWindowDrawList().AddRectFilled(pmin, pmax, bgCol, 2.5f);
            ImGui.GetWindowDrawList().AddRectFilled(pmin, new Vector2(pmin.X + sw * t, pmax.Y), fillCol, 2.5f);
            // Make ImGui slider visuals fully transparent (prevents click/drag fade overlay)
            ImGui.PushStyleColor(ImGuiCol.FrameBg, new Vector4(0, 0, 0, 0));
            ImGui.PushStyleColor(ImGuiCol.FrameBgHovered, new Vector4(0, 0, 0, 0));
            ImGui.PushStyleColor(ImGuiCol.FrameBgActive, new Vector4(0, 0, 0, 0));
            ImGui.PushStyleColor(ImGuiCol.SliderGrab, new Vector4(0, 0, 0, 0));
            ImGui.PushStyleColor(ImGuiCol.SliderGrabActive, new Vector4(0, 0, 0, 0));
            ImGui.SetNextItemWidth(sw);
            bool ch = ImGui.SliderFloat("##" + label, ref value, min, max, "", ImGuiSliderFlags.NoInput);
            ImGui.PopStyleColor(5);
            ImGui.Dummy(new Vector2(0, fh));
            return ch;
        }

        void RenderAimTab()
        {
            ImGui.PushStyleVar(ImGuiStyleVar.ItemSpacing, new Vector2(4, 4));
            ImGui.PushStyleVar(ImGuiStyleVar.FramePadding, new Vector2(8, 6));
            ImGui.PushStyleColor(ImGuiCol.Separator, new Vector4(0.1f, 0.78f, 0.94f, 0.95f));

            float colW = (ImGui.GetContentRegionAvail().X - 14f) * 0.5f;
            if (ImGui.BeginTable("##aim_align_table", 2, ImGuiTableFlags.SizingFixedFit | ImGuiTableFlags.NoSavedSettings))
            {
                ImGui.TableSetupColumn("left", ImGuiTableColumnFlags.WidthFixed, colW);
                ImGui.TableSetupColumn("right", ImGuiTableColumnFlags.WidthFixed, colW);
                ImGui.TableNextRow();

                ImGui.TableNextColumn();
                ImGui.TextColored(new Vector4(0.73f, 0.82f, 0.95f, _uiAlpha), "Targeting");
                ImGui.Separator();
                CustomCheckbox.CaxCustomCheckbox("Enable Aimbot", ref Config.enableAimBot);
                CustomCheckbox.CaxCustomCheckbox("Visible Check", ref Config.AimbotVisible);
                CustomCheckbox.CaxCustomCheckbox("Silent Aim", ref Config.kcsafeSilentAim);
                CustomCheckbox.CaxCustomCheckbox("Rage Aim", ref Config.AimBotRage);
                CustomCheckbox.CaxCustomCheckbox("Ignore Knocked", ref Config.IgnoreKnocked);
                CustomCheckbox.CaxCustomCheckbox("Show FOV Circle", ref Config.FOVEnabled);
                CustomSlider.CaxCustomSlider("FOV Radius", ref Config.AimFov, 0, 1000, 165f);
                CustomCheckbox.CaxCustomCheckbox("Up Player", ref Config.UpPlayer);

                ImGui.TableNextColumn();
                ImGui.TextColored(new Vector4(0.73f, 0.82f, 0.95f, _uiAlpha), "Weapon / Actions");
                ImGui.Separator();
                CustomCheckbox.CaxCustomCheckbox("No Recoil", ref Config.NoRecoil);
                CustomCheckbox.CaxCustomCheckbox("Fast Reload", ref Config.FastReload);
                CustomCheckbox.CaxCustomCheckbox("Rapid Fire", ref Config.RapidFire);
                CustomSlider.CaxCustomSlider("Rapid Fire Speed", ref Config.RapidFireSpeed, 0.1f, 1.0f, 165f);
                CustomCheckbox.CaxCustomCheckbox("Unlimited Ammo", ref Config.UnlimitedAmmo);
                CustomCheckbox.CaxCustomCheckbox("TeleKill 10m", ref Config.tele);
                CustomCheckbox.CaxCustomCheckbox("Shake Kill", ref Config.ShakeKill);
                if (CustomCheckbox.CaxCustomCheckbox("Fly Hack Ext", ref Config.flyhack))
                {
                    if (fly) flyahackk(); else flyahackk();
                }

                ImGui.EndTable();
            }

            ImGui.PopStyleColor();
            ImGui.PopStyleVar(2);
        }

        void RenderVisualsTab()
        {
            ImGui.PushStyleVar(ImGuiStyleVar.ItemSpacing, new Vector2(4, 2));
            PushVisualsPopupChrome();

            float fullWidth = ImGui.GetContentRegionAvail().X;

            StepVisualsAnim(ref _visLineComboAnim, Config.ESPLine ? 1f : 0f);
            StepVisualsAnim(ref _visInfoComboAnim, Config.ESPInformation ? 1f : 0f);
            StepVisualsAnim(ref _visNameComboAnim, (Config.ESPInformation || Config.ESPName) ? 1f : 0f);
            StepVisualsAnim(ref _espEditorHealthSectionAnim, Config.ESPHealth ? 1f : 0f);
            StepVisualsAnim(ref _visWeaponComboAnim, Config.espweapon ? 1f : 0f);

            const float comboStripH = 24f;
            _comboBox = (int)Config.ESPLinePosition;

            // ===== BASIC =====
            CustomCheckbox.CaxCustomCheckbox("Esp Editor", ref Config.ESPEditor);
            CustomCheckbox.CaxCustomCheckbox("Detect Hacker In-Game", ref Config.HackerTag);

            bool espRgb = Config.ESPRGB;
            if (CustomCheckbox.CaxCustomCheckbox("Esp RGB", ref espRgb))
                Config.ESPRGB = espRgb;

            CustomCheckbox.CaxCustomCheckbox("Count Valid Match", ref Config.EspTimer);
            CustomCheckbox.CaxCustomCheckbox("Aim Track Line", ref Config.AimTrackLine);

            CustomSeparator.Draw("Esp Line Settings");

            CustomCheckbox.CaxCustomCheckbox("Snap Esp Line", ref Config.ESPLine);

            DrawVisualsComboStrip("##vis_strip_line", _visLineComboAnim, comboStripH, Config.alignment, () =>
            {
                ImGui.SetNextItemWidth(ImGui.GetContentRegionAvail().X);
                PushVisualsComboChrome();
                if (ImGui.Combo("##linePos", ref _comboBox, _comboItems, _comboItems.Length))
                    Config.ESPLinePosition = (LinePosition)_comboBox;
                PopVisualsComboChrome();
            });

            // ===== BOX SETTINGS =====
            CustomSeparator.Draw("Esp Box Settings");

            CustomCheckbox.CaxCustomCheckbox("ESP Box", ref Config.ESPBox);

            // ===== INFO =====
            CustomSeparator.Draw("Esp Information");

            ImGui.PushStyleVar(ImGuiStyleVar.CellPadding, new Vector2(4f, 2f));
            if (ImGui.BeginTable("##esp_cfg_grid", 3, ImGuiTableFlags.SizingStretchSame | ImGuiTableFlags.NoSavedSettings))
            {
                ImGui.TableNextRow();
                ImGui.TableNextColumn();
                CustomCheckbox.CaxCustomCheckbox("ESP Name", ref Config.ESPName);
                ImGui.TableNextColumn();
                CustomCheckbox.CaxCustomCheckbox("ESP Health Bar", ref Config.ESPHealth);
                ImGui.TableNextColumn();
                bool espw = Config.espweapon;
                if (CustomCheckbox.CaxCustomCheckbox("ESP Weapon", ref espw))
                    Config.espweapon = espw;

                ImGui.TableNextRow();
                ImGui.TableNextColumn();
                CustomCheckbox.CaxCustomCheckbox("ESP Information", ref Config.ESPInformation);
                ImGui.TableNextColumn();
                CustomCheckbox.CaxCustomCheckbox("ESP Health Text", ref Config.ESPHealthText);
                ImGui.TableNextColumn();
                bool rbg = Config.rgb;
                if (CustomCheckbox.CaxCustomCheckbox("Weapon RGB", ref rbg))
                    Config.rgb = rbg;

                ImGui.TableNextRow();
                ImGui.TableNextColumn();
                CustomCheckbox.CaxCustomCheckbox("ESP Skeleton", ref Config.ESPSkeleton);
                ImGui.TableNextColumn();
                CustomCheckbox.CaxCustomCheckbox("Esp Distance", ref Config.espdistance);
                ImGui.TableNextColumn();
                ImGui.Dummy(new Vector2(1f, 1f));

                ImGui.EndTable();
            }
            ImGui.PopStyleVar();

            // Distance editor (separate control): choose distance anchor position.
            DrawVisualsComboStrip(
                "##vis_strip_distance",
                _visInfoComboAnim,
                comboStripH,
                Config.ESPInformation && Config.espdistance,
                () =>
                {
                    ImGui.SetNextItemWidth(ImGui.GetContentRegionAvail().X);
                    PushVisualsComboChrome();
                    int d = (int)Config.EspDistanceAnchor;
                    if (ImGui.Combo("##vis_esp_distance_pos", ref d, _espEditorDistanceItems, _espEditorDistanceItems.Length))
                        Config.EspDistanceAnchor = (EspDistanceBoxAnchor)Math.Clamp(d, 0, _espEditorDistanceItems.Length - 1);
                    PopVisualsComboChrome();
                });

            CustomSeparator.Draw("Esp Render Settings");
            float espRender = Math.Clamp(Config.Esprender, 0f, 100f);
            if (CustomSlider.CaxCustomSlider("ESP Render Distance (m)", ref espRender, 0f, 100f, 280f))
                Config.Esprender = espRender;

            PopVisualsPopupChrome();
            ImGui.PopStyleVar();
        }

        void RenderMiscTab()
        {
            ImGui.PushStyleVar(ImGuiStyleVar.ItemSpacing, new Vector2(2, 3));
            ImGui.PushStyleColor(ImGuiCol.Separator, new Vector4(0.1f, 0.78f, 0.94f, 0.95f));

            float colW = (ImGui.GetContentRegionAvail().X - 14f) * 0.5f;
            if (ImGui.BeginTable("##misc_like_ref_table", 2, ImGuiTableFlags.SizingFixedFit | ImGuiTableFlags.NoSavedSettings))
            {
                ImGui.TableSetupColumn("left", ImGuiTableColumnFlags.WidthFixed, colW);
                ImGui.TableSetupColumn("right", ImGuiTableColumnFlags.WidthFixed, colW);
                ImGui.TableNextRow();

                ImGui.TableNextColumn();
                ImGui.TextColored(new Vector4(0.9f, 0.9f, 0.9f, 1f), "Misc Hacks");
                ImGui.Separator();
                CustomCheckbox.CaxCustomCheckbox("Teleport", ref Config.teliport);
                CustomCheckbox.CaxCustomCheckbox("Spawn kill", ref Config.Kcspawnkill);
                CustomCheckbox.CaxCustomCheckbox("Climb Up", ref Config.ClimbUpEnabled);
                CustomCheckbox.CaxCustomCheckbox("Downkill", ref Config.DownPlayer);
                CustomCheckbox.CaxCustomCheckbox("UnderGround Killl", ref Config.undergroundkill);
                CustomCheckbox.CaxCustomCheckbox("Enemy Pull 360*", ref Config.EnemyPullEnabled);
                CustomCheckbox.CaxCustomCheckbox("Enemy Pull V2", ref Config.EnemyPullEnabledv2);
                CustomCheckbox.CaxCustomCheckbox("Magnet pull", ref Config.MAGNETPULL);
                if (CustomCheckbox.CaxCustomCheckbox("Spin Bot", ref Config.spinbot))
                {
                    if (Config.spinbot) RapidSpin.Activate();
                    else RapidSpin.Deactivate();
                }
                CustomSlider.CaxCustomSlider("Spin Speed", ref Config.SpinBotSpeed, 0.1f, 20.0f, 165f);

                ImGui.TableNextColumn();
                ImGui.TextColored(new Vector4(0.9f, 0.9f, 0.9f, 1f), "Others");
                ImGui.Separator();

                if (CustomCheckbox.CaxCustomCheckbox("High Jump", ref Config.highjumppp))
                {
                    if (jump) highjump(); else highjump();
                }
                if (CustomCheckbox.CaxCustomCheckbox("No Gravity Fly", ref Config.FlyHack))
                    KCNOGRAVITYFLY.SetState(Config.FlyHack);
                if (CustomCheckbox.CaxCustomCheckbox("Camera Left", ref Config.cameraleftt))
                {
                    if (left) cameraleftt(); else cameraleftt();
                }
                if (CustomCheckbox.CaxCustomCheckbox("Vision Hack", ref Config.visionhackkk))
                {
                    if (vision) visionhack(); else visionhack();
                }
                CustomCheckbox.CaxCustomCheckbox("Mark Teleport", ref Config.teleportmap);
                if (CustomCheckbox.CaxCustomCheckbox("Rapid Fire", ref Config.brsutfiree))
                {
                    if (brust) brustfire(); else brustfire();
                }
                CustomCheckbox.CaxCustomCheckbox("Speed Hack Timer Int", ref Config.speedint);
                if (CustomCheckbox.CaxCustomCheckbox("Speed Hack External", ref Config.speedext))
                {
                    if (speedext) loadspeedext(); else loadspeedext();
                }
                if (CustomCheckbox.CaxCustomCheckbox("Wallhack - Risk", ref Config.wallhack))
                {
                    if (wallhackkk) wallhackrisk(); else wallhackrisk();
                }
                CustomCheckbox.CaxCustomCheckbox("Ai Teleport Wall", ref Config.teliportwall);

                ImGui.EndTable();
            }

            ImGui.PopStyleColor();
            ImGui.PopStyleVar();
        }

        void RenderConfigTab()
        {
            ImGui.PushStyleVar(ImGuiStyleVar.ItemSpacing, new Vector2(2, 3));
            ImGui.PushStyleColor(ImGuiCol.Separator, new Vector4(0.1f, 0.78f, 0.94f, 0.95f));
            float colW = (ImGui.GetContentRegionAvail().X - 14f) * 0.5f;
            if (ImGui.BeginTable("##cfg_align_table", 2, ImGuiTableFlags.SizingFixedFit | ImGuiTableFlags.NoSavedSettings))
            {
                ImGui.TableSetupColumn("left", ImGuiTableColumnFlags.WidthFixed, colW);
                ImGui.TableSetupColumn("right", ImGuiTableColumnFlags.WidthFixed, colW);
                ImGui.TableNextRow();

                ImGui.TableNextColumn();
                ImGui.TextColored(new Vector4(0.9f, 0.9f, 0.9f, 1f), "Panel");
                ImGui.Separator();
                CustomCheckbox.CaxCustomCheckbox("Stream Mode", ref Config.StreamMode);
                bool close = false;
                CustomCheckbox.CaxCustomCheckbox("Close Panel", ref close);
                if (close) { KillProcess("HD-Adb"); KillProcess("HD-Player"); Environment.Exit(0); }
                ImGui.TextDisabled("Insert = Show/Hide");

                ImGui.TableNextColumn();
                ImGui.TextColored(new Vector4(0.9f, 0.9f, 0.9f, 1f), "Theme");
                ImGui.Separator();
                CustomColourPicker.CaxCustomColourPicker("UI Theme", ref ThemeColor);

                ImGui.EndTable();
            }
            ImGui.PopStyleColor();
            ImGui.PopStyleVar();
        }

        void RenderSkullTab()
        {
            ImGui.PushStyleVar(ImGuiStyleVar.ItemSpacing, new Vector2(2, 3));
            ImGui.PushStyleColor(ImGuiCol.Separator, new Vector4(0.1f, 0.78f, 0.94f, 0.95f));
            float colW = (ImGui.GetContentRegionAvail().X - 14f) * 0.5f;
            if (ImGui.BeginTable("##ext_align_table", 2, ImGuiTableFlags.SizingFixedFit | ImGuiTableFlags.NoSavedSettings))
            {
                ImGui.TableSetupColumn("left", ImGuiTableColumnFlags.WidthFixed, colW);
                ImGui.TableSetupColumn("right", ImGuiTableColumnFlags.WidthFixed, colW);
                ImGui.TableNextRow();
                ImGui.TableNextColumn();
                ImGui.TextColored(new Vector4(0.9f, 0.9f, 0.9f, 1f), "Ext");
                ImGui.Separator();
                CustomCheckbox.CaxCustomCheckbox("Ai Teleport Wall", ref Config.teliportwall);
                ImGui.TableNextColumn();
                ImGui.TextColored(new Vector4(0.9f, 0.9f, 0.9f, 1f), "Wardrobe Preview");
                ImGui.Separator();
                RenderWardrobePreview();
                ImGui.EndTable();
            }
            ImGui.PopStyleColor();
            ImGui.PopStyleVar();
        }

        private void RenderWardrobePreview()
        {
            ClothingCatalog.EnsureLoaded();
            CustomCheckbox.CaxCustomCheckbox("Enable wardrobe preview", ref Config.WardrobeEnabled);
            if (!Config.WardrobeEnabled)
                return;

            ImGui.TextDisabled("Category");
            if (ImGui.BeginCombo("##wardrobe_category", Config.SelectedWardrobeCategory))
            {
                foreach (string category in ClothingCatalog.Categories)
                {
                    bool selected = category == Config.SelectedWardrobeCategory;
                    if (ImGui.Selectable(category, selected))
                        Config.SelectedWardrobeCategory = category;
                    if (selected)
                        ImGui.SetItemDefaultFocus();
                }
                ImGui.EndCombo();
            }

            ImGui.TextDisabled($"Available items ({ClothingCatalog.Count})");
            ImGui.BeginChild("##wardrobe_items", new Vector2(0, 112), ImGuiChildFlags.Border);
            foreach (ClothingItem item in ClothingCatalog.ForCategory(Config.SelectedWardrobeCategory))
            {
                bool selected = item.Id == Config.SelectedWardrobeItemId;
                string label = $"{item.Name}##wardrobe_{item.Id}";
                if (ImGui.Selectable(label, selected))
                {
                    Config.SelectedWardrobeItemId = item.Id;
                    Config.SelectedWardrobeItemName = item.Name;
                    CustomNotification.Notify("Wardrobe preview", $"Selected {item.Name}", 2.5f);
                }
            }
            ImGui.EndChild();

            ImGui.TextDisabled($"Selected: {Config.SelectedWardrobeItemName}");
            if (CustomButton.CaxCustomButton("Clear selection", new Vector2(145, 25)))
            {
                Config.SelectedWardrobeItemId = 0;
                Config.SelectedWardrobeItemName = "No item selected";
                CustomNotification.Notify("Wardrobe preview", "Selection cleared", 2.5f);
            }
        }

        private void Text(string text, float opacity = 1)
        {
            var textColor = ImGui.GetColorU32(ImGuiCol.Text);
            var textColorVec = ImGui.ColorConvertU32ToFloat4(textColor);
            textColorVec.W = opacity;
            ImGui.PushStyleColor(ImGuiCol.Text, textColorVec);
            {
                ImGui.Text(text);
            }
            ImGui.PopStyleColor();
        }
        public void KillProcess(string processName)
        {
            var processes = Process.GetProcessesByName(processName);
            foreach (var process in processes)
            {
                process.Kill();
                process.WaitForExit();
            }
        }

        private static byte[]? LoadFaSolidFontBytes()
        {
            if (_retainedFaSolidBytes != null)
                return _retainedFaSolidBytes;
            try
            {
                using var s = typeof(ESP).Assembly.GetManifestResourceStream("fa_solid_900.ttf");
                if (s != null)
                {
                    using var ms = new MemoryStream();
                    s.CopyTo(ms);
                    _retainedFaSolidBytes = ms.ToArray();
                    return _retainedFaSolidBytes;
                }
            }
            catch { }
            try
            {
                if (File.Exists(UiFonts.FaSolidPath))
                    _retainedFaSolidBytes = File.ReadAllBytes(UiFonts.FaSolidPath);
            }
            catch { }
            return _retainedFaSolidBytes;
        }

        private static string? ResolveUiBodyFontPath()
        {
            if (File.Exists(UiFonts.TahomabdPath))
                return UiFonts.TahomabdPath;

            string dir = Environment.GetFolderPath(Environment.SpecialFolder.Fonts);
            string[] names =
            {
                "TAHOMABD.TTF", "tahoma.ttf", "TAHOMA.TTF",
                "segoeuib.ttf", "seguisb.ttf", "seguiui.ttf", "arialbd.ttf", "arial.ttf"
            };
            foreach (string name in names)
            {
                string p = Path.Combine(dir, name);
                if (File.Exists(p))
                    return p;
            }
            return null;
        }

        protected override unsafe Task PostInitialized()
        {
            ReplaceFont(config =>
            {
                var io = ImGui.GetIO();
                _faMergedIntoTextFont = false;

                string? bodyPath = ResolveUiBodyFontPath();
                if (string.IsNullOrEmpty(bodyPath))
                    return;

                // CounterStrike2 Program.cs: 14px tahomabd + FA merge (14px, PixelSnapH, 0xF000–0xF8FF)
                TextFont = io.Fonts.AddFontFromFileTTF(bodyPath, 14f, null, io.Fonts.GetGlyphRangesDefault());

                // Merge extra Unicode blocks into TextFont so in-game names don't show as '?'
                unsafe
                {
                    config->MergeMode = 1;
                    config->PixelSnapH = 1;
                    io.Fonts.AddFontFromFileTTF(bodyPath, 14f, config, io.Fonts.GetGlyphRangesCyrillic());
                    io.Fonts.AddFontFromFileTTF(bodyPath, 14f, config, io.Fonts.GetGlyphRangesVietnamese());
                    io.Fonts.AddFontFromFileTTF(bodyPath, 14f, config, io.Fonts.GetGlyphRangesThai());
                    ushort[] devanagari = { 0x0900, 0x097F, 0 };
                    fixed (ushort* p = devanagari)
                        io.Fonts.AddFontFromFileTTF(bodyPath, 14f, config, (nint)p);
                    ushort[] arabic = { 0x0600, 0x06FF, 0 };
                    fixed (ushort* p = arabic)
                        io.Fonts.AddFontFromFileTTF(bodyPath, 14f, config, (nint)p);
                    ushort[] bengali = { 0x0980, 0x09FF, 0 };
                    fixed (ushort* p = bengali)
                        io.Fonts.AddFontFromFileTTF(bodyPath, 14f, config, (nint)p);
                    // Extra ranges often used in game nicknames (Tahoma/body may lack glyphs → '?' in ESP).
                    ushort[] latinExtMisc = { 0x0100, 0x024F, 0x2600, 0x26FF, 0x2700, 0x27BF, 0xFE00, 0xFE0F, 0 };
                    fixed (ushort* p = latinExtMisc)
                        io.Fonts.AddFontFromFileTTF(bodyPath, 14f, config, (nint)p);
                    // Segoe UI (Windows): broad fallback for symbols / CJK / extended Latin when body font lacks glyphs.
                    string segoeUi = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Fonts), "segoeui.ttf");
                    if (File.Exists(segoeUi))
                    {
                        io.Fonts.AddFontFromFileTTF(segoeUi, 14f, config, io.Fonts.GetGlyphRangesDefault());
                        // Common CJK nicknames (avoid Japanese/Korean full ranges here — very large atlas).
                        io.Fonts.AddFontFromFileTTF(segoeUi, 14f, config, io.Fonts.GetGlyphRangesChineseSimplifiedCommon());
                    }
                    config->MergeMode = 0;
                    config->PixelSnapH = 0;
                }

                ushort[] faRange = { 0xF000, 0xF8FF, 0 };
                if (File.Exists(UiFonts.FaSolidPath))
                {
                    unsafe
                    {
                        fixed (ushort* gr = faRange)
                        {
                            config->MergeMode = 1;
                            config->PixelSnapH = 1;
                            io.Fonts.AddFontFromFileTTF(UiFonts.FaSolidPath, 14f, config, (nint)gr);
                            config->MergeMode = 0;
                            config->PixelSnapH = 0;
                        }
                    }
                    _faMergedIntoTextFont = true;
                }
                else
                {
                    byte[]? faBytes = LoadFaSolidFontBytes();
                    if (faBytes != null && faBytes.Length > 0)
                    {
                        unsafe
                        {
                            fixed (byte* fp = faBytes)
                            {
                                fixed (ushort* gr = faRange)
                                {
                                    config->MergeMode = 1;
                                    config->PixelSnapH = 1;
                                    io.Fonts.AddFontFromMemoryTTF((nint)fp, faBytes.Length, 14f, config, (nint)gr);
                                    config->MergeMode = 0;
                                    config->PixelSnapH = 0;
                                }
                            }
                        }
                        _faMergedIntoTextFont = true;
                    }
                }

                HeaderFont = io.Fonts.AddFontFromFileTTF(bodyPath, 20f, null, io.Fonts.GetGlyphRangesDefault());
                LebelFont = io.Fonts.AddFontFromFileTTF(bodyPath, 18f, null, io.Fonts.GetGlyphRangesDefault());
                CustomNotification.TextFont = TextFont;
            });

            return base.PostInitialized();
        }

    }


}
