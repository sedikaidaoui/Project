using ImGuiNET;
using System;
using System.Collections.Generic;
using System.Numerics;

namespace AotForms.Components
{
    public static class CustomCheckbox
    {
        static Dictionary<uint, float> Anim = new();

        public static bool CaxCustomCheckbox(string text, ref bool value)
        {
            ImGuiStylePtr style = ImGui.GetStyle();
            ImDrawListPtr drawList = ImGui.GetWindowDrawList();
            ImGuiIOPtr io = ImGui.GetIO();

            float boxSize = 20.0f;
            float spacing = 2.0f;
            float shiftRight = 10.0f;
            float extraVerticalGap = 2.0f;
            float topGap = 3.0f;

            Vector2 originalCursor = ImGui.GetCursorScreenPos();
            Vector2 drawCursor = originalCursor;
            drawCursor.X += shiftRight;
            drawCursor.Y += topGap;

            Vector2 textSize = ImGui.CalcTextSize(text);
            Vector2 totalSize = new Vector2(boxSize + spacing + textSize.X, MathF.Max(boxSize, textSize.Y));

            uint id = ImGui.GetID(text);

            ImGui.SetCursorScreenPos(drawCursor);
            bool pressed = ImGui.InvisibleButton(text, totalSize);
            if (pressed)
            {
                value = !value;
                CustomNotification.Notify(text, value ? "Activated" : "De-Activated", 2.5f);
            }

            ImGui.SetCursorScreenPos(new Vector2(originalCursor.X, originalCursor.Y + totalSize.Y + style.ItemSpacing.Y + extraVerticalGap));

            if (!Anim.ContainsKey(id)) Anim[id] = value ? 1f : 0f;
            float target = value ? 1f : 0f;
            Anim[id] += (target - Anim[id]) * io.DeltaTime * 14f;
            float anim = Anim[id];

            Vector2 boxMin = drawCursor + new Vector2(0, extraVerticalGap);
            Vector2 boxMax = boxMin + new Vector2(boxSize, boxSize);

            Vector4 uncheckedDark = new(0.10f, 0.10f, 0.10f, 1.0f);
            Vector4 checkedColor = ESP.ThemeColor;
            Vector4 fillColor = uncheckedDark + (checkedColor - uncheckedDark) * anim;
            uint tickWhite = ImGui.GetColorU32(new Vector4(1f, 1f, 1f, MathF.Pow(anim, 0.5f)));

            if (anim > 0.01f)
            {
                int layers = 8;
                for (int i = 0; i < layers; i++)
                {
                    float tVal = i / (float)layers;
                    float falloff = MathF.Exp(-tVal * tVal * 4.5f);
                    float spread = tVal * 28f;
                    float alphaLayer = falloff * 0.035f * anim;

                    Vector2 halfExpanded = new Vector2(spread);
                    drawList.AddRectFilled(
                        boxMin - halfExpanded,
                        boxMax + halfExpanded,
                        ImGui.GetColorU32(new Vector4(checkedColor.X, checkedColor.Y, checkedColor.Z, alphaLayer)),
                        3f + spread);
                }
            }

            drawList.AddRectFilled(boxMin, boxMax, ImGui.GetColorU32(fillColor), 3f);
            if (anim < 0.99f)
                drawList.AddRect(boxMin, boxMax, ImGui.GetColorU32(new Vector4(0.18f, 0.18f, 0.18f, 1f * (1f - anim))), 3f, ImDrawFlags.None, 1.5f);

            if (anim > 0.05f)
            {
                Vector2 center = (boxMin + boxMax) * 0.5f;
                float tickAnim = MathF.Pow(anim, 0.6f);
                float tSize = (boxSize * 0.5f) * tickAnim;

                drawList.AddLine(
                    new Vector2(center.X - tSize * 0.55f, center.Y + tSize * 0.1f),
                    new Vector2(center.X - tSize * 0.15f, center.Y + tSize * 0.55f),
                    tickWhite, 2.8f);
                drawList.AddLine(
                    new Vector2(center.X - tSize * 0.15f, center.Y + tSize * 0.55f),
                    new Vector2(center.X + tSize * 0.7f, center.Y - tSize * 0.45f),
                    tickWhite, 2.8f);
            }

            Vector2 textPos = new Vector2(boxMax.X + spacing + 10f, boxMin.Y + (boxSize - textSize.Y) * 0.5f);
            Vector4 textDark = new Vector4(0.40f, 0.40f, 0.40f, 1f);
            Vector4 textWhite = new Vector4(0.85f, 0.85f, 0.85f, 1f);
            Vector4 textColor = textDark + (textWhite - textDark) * MathF.Pow(anim, 0.5f);
            drawList.AddText(textPos, ImGui.GetColorU32(textColor), text);

            return pressed;
        }
    }
}
