using System;
using System.Collections.Generic;
using System.Numerics;
using ImGuiNET;

namespace AotForms.Components
{
    public class CustomNotification
    {
        private class Notification
        {
            public string Title = "";
            public string StatusText = "";
            public float Duration;
            public float Timer;
            public float SlideOffset;
            public bool Finished;
            public float Alpha;
            public float Scale = 1f;
            public Vector4 AccentColor;
        }

        public static ImFontPtr TextFont;
        private static readonly List<Notification> notifications = new();
        private static float width = 300f;
        private static float height = 64f;

        public static IntPtr DragonLogoPtr = IntPtr.Zero;
        private const float DragonLogoSize = 28f;
        private const float DragonLogoPaddingLeft = 12f;

        private static float Lerp(float a, float b, float t) => a + (b - a) * t;
        private static float EaseOut(float t) => 1f - MathF.Pow(1f - t, 3f);
        private static float EaseIn(float t) => t * t * t;

        public static void Notify(string title, string msg, float duration = 4f)
        {
            notifications.Add(new Notification
            {
                Title = title,
                StatusText = msg,
                Duration = duration,
                SlideOffset = -(width + 20f),
                Alpha = 0f,
                Scale = 0.85f,
                AccentColor = new Vector4(1f, 0.82f, 0.12f, 1f)
            });
        }

        public static void Render()
        {
            float spacing = 8f;

            for (int i = 0; i < notifications.Count; i++)
            {
                var n = notifications[i];
                n.Timer += ImGui.GetIO().DeltaTime;

                const float enter = 0.45f;
                const float exit = 0.35f;

                if (!n.Finished)
                {
                    if (n.Timer < enter)
                    {
                        float p = EaseOut(n.Timer / enter);
                        n.SlideOffset = Lerp(-(width + 20f), 0, p);
                        n.Alpha = p;
                        n.Scale = Lerp(0.9f, 1f, p);
                    }
                    else if (n.Timer > n.Duration)
                    {
                        n.Finished = true;
                        n.Timer = 0;
                    }
                }
                else
                {
                    if (n.Timer < exit)
                    {
                        float p = EaseIn(n.Timer / exit);
                        n.SlideOffset = Lerp(0, -(width + 20f), p);
                        n.Alpha = 1 - p;
                        n.Scale = Lerp(1f, 0.92f, p);
                    }
                    else
                    {
                        notifications.RemoveAt(i);
                        i--;
                        continue;
                    }
                }

                Vector2 pos = new Vector2(10 + n.SlideOffset, 10 + i * (height + spacing));
                ImGui.SetNextWindowPos(pos);
                ImGui.SetNextWindowSize(new Vector2(width, height));
                ImGui.PushStyleVar(ImGuiStyleVar.WindowPadding, Vector2.Zero);
                ImGui.PushStyleColor(ImGuiCol.WindowBg, Vector4.Zero);

                if (ImGui.Begin($"##notif{i}", ImGuiWindowFlags.NoDecoration | ImGuiWindowFlags.NoInputs))
                {
                    var draw = ImGui.GetWindowDrawList();
                    Vector2 pMin = ImGui.GetWindowPos();
                    Vector2 pMax = pMin + ImGui.GetWindowSize();
                    float a = n.Alpha;

                    DrawSoftGlow(draw, pMin, pMax, n.AccentColor, a);
                    DrawBackground(draw, pMin, pMax, a);
                    DrawReflection(draw, pMin, pMax, a);
                    DrawBorder(draw, pMin, pMax, a, n.AccentColor);
                    DrawProgress(draw, n, pMin, pMax, a);

                    if (DragonLogoPtr != IntPtr.Zero)
                    {
                        float iconY = pMin.Y + (height - DragonLogoSize) * 0.5f;
                        Vector2 iconPos = new Vector2(pMin.X + DragonLogoPaddingLeft, iconY);
                        Vector2 iconMax = iconPos + new Vector2(DragonLogoSize, DragonLogoSize);
                        draw.AddImage(
                            DragonLogoPtr,
                            iconPos,
                            iconMax,
                            Vector2.Zero,
                            Vector2.One,
                            ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 1f, 1f, a))
                        );
                    }
                    else
                    {
                        float iconY = pMin.Y + (height - DragonLogoSize) * 0.5f;
                        Vector2 iconPos = new Vector2(pMin.X + DragonLogoPaddingLeft, iconY);
                        Vector2 iconMax = iconPos + new Vector2(DragonLogoSize, DragonLogoSize);
                        uint outline = ImGui.GetColorU32(new Vector4(1f, 0.2f, 0.2f, 0.9f * a));
                        uint fill = ImGui.GetColorU32(new Vector4(0.02f, 0.02f, 0.02f, 0.55f * a));
                        draw.AddRectFilled(iconPos, iconMax, fill, 4f);
                        draw.AddRect(iconPos, iconMax, outline, 4f, 0, 1.4f);
                        draw.AddLine(iconPos, iconMax, outline, 1.2f);
                        draw.AddLine(new Vector2(iconPos.X, iconMax.Y), new Vector2(iconMax.X, iconPos.Y), outline, 1.2f);
                    }

                    draw.AddText(new Vector2(pMin.X + 50, pMin.Y + 10),
                        ImGui.GetColorU32(new Vector4(1f, 0.9f, 0.3f, a)), n.Title);

                    draw.AddText(new Vector2(pMin.X + 50, pMin.Y + 30),
                        ImGui.GetColorU32(new Vector4(1f, 1f, 1f, a)), n.StatusText);
                }

                ImGui.End();
                ImGui.PopStyleVar();
                ImGui.PopStyleColor();
            }
        }

        private static void DrawSoftGlow(ImDrawListPtr draw, Vector2 min, Vector2 max, Vector4 col, float a)
        {
            int layers = 18;
            for (int i = 0; i < layers; i++)
            {
                float t = i / (float)layers;
                float spread = 2f + t * 10f;
                float alpha = (1f - t) * 0.08f * a;
                draw.AddRectFilled(
                    min - new Vector2(spread),
                    max + new Vector2(spread),
                    ImGui.ColorConvertFloat4ToU32(new Vector4(col.X, col.Y, col.Z, alpha)),
                    6f + t * 6f
                );
            }
        }

        private static void DrawBackground(ImDrawListPtr draw, Vector2 min, Vector2 max, float a)
        {
            draw.AddRectFilled(min, max,
                ImGui.ColorConvertFloat4ToU32(new Vector4(0.05f, 0.05f, 0.06f, 0.96f * a)), 6f);
        }

        private static void DrawReflection(ImDrawListPtr draw, Vector2 pMin, Vector2 pMax, float a)
        {
            float time = (float)ImGui.GetTime();
            float widthLocal = pMax.X - pMin.X;
            float heightLocal = pMax.Y - pMin.Y;

            float t = (time * 0.5f) % 1.3f;
            float rw = widthLocal * 0.28f;
            float x = pMin.X + (t * (widthLocal + rw)) - rw;
            float slant = heightLocal * 0.6f;

            int layers = 28;
            for (int i = 0; i < layers; i++)
            {
                float k = i / (float)layers;
                float alpha = MathF.Sin(k * MathF.PI) * 0.10f * a;

                float o0 = k * rw;
                float o1 = (k + 1f / layers) * rw;
                Vector2 p1 = new Vector2(x + o0, pMin.Y);
                Vector2 p2 = new Vector2(x + o1, pMin.Y);
                Vector2 p3 = new Vector2(x + o1 + slant, pMax.Y);
                Vector2 p4 = new Vector2(x + o0 + slant, pMax.Y);

                draw.AddQuadFilled(p1, p2, p3, p4,
                    ImGui.ColorConvertFloat4ToU32(new Vector4(1, 1, 1, alpha)));
            }
        }

        private static void DrawBorder(ImDrawListPtr draw, Vector2 mn, Vector2 mx, float a, Vector4 col)
        {
            float w = mx.X - mn.X;
            float h = mx.Y - mn.Y;
            float L = 2f * (w + h);

            float phase = (float)(ImGui.GetTime() * 0.9 % 1.0);
            float start = phase * L;
            float seg = L * 0.22f;
            int steps = 100;

            for (int i = 0; i < steps; i++)
            {
                float t0 = start + seg * i / steps;
                float t1 = start + seg * (i + 1) / steps;
                Vector2 a0 = Point(mn, mx, t0 % L);
                Vector2 a1 = Point(mn, mx, t1 % L);
                float fade = 1f - i / (float)steps;

                draw.AddLine(a0, a1,
                    ImGui.ColorConvertFloat4ToU32(new Vector4(col.X, col.Y, col.Z, 0.25f * fade * a)), 4f);
                draw.AddLine(a0, a1,
                    ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 0.95f, 0.5f, fade * a)), 2f);
            }
        }

        private static Vector2 Point(Vector2 mn, Vector2 mx, float d)
        {
            float w = mx.X - mn.X;
            float h = mx.Y - mn.Y;
            if (d < w) return new Vector2(mn.X + d, mn.Y);
            d -= w;
            if (d < h) return new Vector2(mx.X, mn.Y + d);
            d -= h;
            if (d < w) return new Vector2(mx.X - d, mx.Y);
            d -= w;
            return new Vector2(mn.X, mx.Y - d);
        }

        private static void DrawProgress(ImDrawListPtr draw, Notification n, Vector2 pMin, Vector2 pMax, float a)
        {
            float h = 3f;
            Vector2 min = new Vector2(pMin.X, pMax.Y - h);
            float progress = Math.Clamp(n.Timer / n.Duration, 0f, 1f);
            float eased = 1f - MathF.Pow(1f - progress, 2f);
            float w = (pMax.X - pMin.X) * (1f - eased);
            Vector2 max = new Vector2(pMin.X + w, pMax.Y);

            draw.AddRectFilled(min, max,
                ImGui.ColorConvertFloat4ToU32(new Vector4(n.AccentColor.X, n.AccentColor.Y, n.AccentColor.Z, 0.95f * a)));
            float glow = 30f;
            draw.AddRectFilled(
                new Vector2(max.X - glow, min.Y),
                max,
                ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 1f, 0.9f, 0.25f * a))
            );
        }

        // Compatibility wrappers used by current codebase.
        public static void RenderNotifications() => Render();
        public static void NotifyMessage(string title, string message, float duration = 4f) => Notify(title, message, duration);
    }
}
