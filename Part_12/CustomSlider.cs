using ImGuiNET;
using System;
using System.Collections.Generic;
using System.Numerics;

namespace AotForms.Components
{
    public static class CustomSlider
    {
        static Dictionary<uint, float> Anim = new();

        public static bool CaxCustomSlider(
            string label,
            ref float value,
            float min,
            float max,
            float width = 280f)
        {
            var draw = ImGui.GetWindowDrawList();
            var io = ImGui.GetIO();
            var style = ImGui.GetStyle();

            float height = 14f;
            float rounding = 5f;
            float spacing = 12f;
            float shiftRight = 30f;
            float verticalGap = 6f;

            Vector2 cursor = ImGui.GetCursorScreenPos();
            Vector2 sliderMin = new(cursor.X + shiftRight, cursor.Y + verticalGap);
            Vector2 sliderMax = sliderMin + new Vector2(width, height);

            ImGui.SetCursorScreenPos(sliderMin);
            ImGui.InvisibleButton($"##slider_{label}", new Vector2(width, height));

            bool isChanged = false;

            if (ImGui.IsItemActive())
            {
                Vector2 mousePos = ImGui.GetMousePos();
                float newValue = min + ((mousePos.X - sliderMin.X) / width) * (max - min);
                newValue = Math.Clamp(newValue, min, max);

                if (Math.Abs(newValue - value) > 0.001f)
                {
                    value = newValue;
                    isChanged = true;
                }
            }

            if (ImGui.IsItemClicked(ImGuiMouseButton.Left))
            {
                Vector2 mousePos = ImGui.GetMousePos();
                float newValue = min + ((mousePos.X - sliderMin.X) / width) * (max - min);
                newValue = Math.Clamp(newValue, min, max);
                value = newValue;
                isChanged = true;
            }

            uint id = ImGui.GetID(label);
            if (!Anim.ContainsKey(id)) Anim[id] = value;
            Anim[id] += (value - Anim[id]) * MathF.Min(io.DeltaTime * 5f, 1f);
            float animValue = Anim[id];

            float normalized = (animValue - min) / (max - min);
            normalized = Math.Clamp(normalized, 0f, 1f);
            float fillWidth = width * normalized;
            Vector2 fillMax = sliderMin + new Vector2(fillWidth, height);

            Vector4 bgColor = new(0.08f, 0.10f, 0.12f, 1f);
            Vector4 themeColor = ESP.ThemeColor;

            draw.AddRectFilled(sliderMin, sliderMax, ImGui.GetColorU32(bgColor), rounding);
            draw.AddRect(sliderMin, sliderMax, ImGui.GetColorU32(new Vector4(0.24f, 0.74f, 0.90f, 0.55f)), rounding, ImDrawFlags.None, 1.5f);

            if (fillWidth > 0)
            {
                int layers = 10;
                for (int i = 0; i < layers; i++)
                {
                    float t = i / 10f;
                    float falloff = MathF.Exp(-t * t * 4f);
                    float spread = t * 24f;
                    float alphaLayer = falloff * 0.05f * normalized;
                    Vector2 halfExpanded = new Vector2(spread, spread);
                    draw.AddRectFilled(
                        sliderMin - halfExpanded,
                        fillMax + halfExpanded,
                        ImGui.GetColorU32(new Vector4(themeColor.X, themeColor.Y, themeColor.Z, alphaLayer)),
                        rounding + spread);
                }

                draw.AddRectFilled(sliderMin, fillMax, ImGui.GetColorU32(themeColor), rounding);
            }

            string valueText = value.ToString("0.###");
            Vector2 valueSize = ImGui.CalcTextSize(valueText);
            Vector2 valuePos = new(
                sliderMin.X + (width - valueSize.X) * 0.5f,
                sliderMin.Y + (height - valueSize.Y) * 0.5f
            );
            draw.AddText(valuePos, ImGui.GetColorU32(new Vector4(1f, 1f, 1f, 1f)), valueText);

            Vector2 labelSize = ImGui.CalcTextSize(label);
            Vector2 labelPos = new(
                sliderMax.X + spacing + 6f,
                sliderMin.Y + (height - labelSize.Y) * 0.5f - 1f
            );
            draw.AddText(labelPos, ImGui.GetColorU32(new Vector4(1f, 1f, 1f, 1f)), label);

            ImGui.SetCursorScreenPos(new Vector2(cursor.X, cursor.Y + height + verticalGap * 2 + style.ItemSpacing.Y));

            return isChanged;
        }
    }
}
