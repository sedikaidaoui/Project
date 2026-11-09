using System.Collections.Generic;
using System.Numerics;
using ImGuiNET;

namespace AotForms.Components
{
    public static class CustomButton
    {
        private static Dictionary<string, float> _animations = new Dictionary<string, float>();

        public static bool CaxCustomButton(string label, Vector2 size, bool enabled = true)
        {
            string id = $"##btn_{label}";

            if (!_animations.ContainsKey(id))
                _animations[id] = 0f;

            var draw = ImGui.GetWindowDrawList();
            Vector2 pos = ImGui.GetCursorScreenPos();

            Vector2 originalCursorPos = ImGui.GetCursorPos();

            float rounding = 4f;
            Vector4 bgColor = enabled ?
                new Vector4(
                    ESP.ThemeColor.X * 0.2f + 0.1f * _animations[id],
                    ESP.ThemeColor.Y * 0.2f + 0.1f * _animations[id],
                    ESP.ThemeColor.Z * 0.2f + 0.1f * _animations[id],
                    ESP.ThemeColor.W
                ) :
                new Vector4(0.15f, 0.15f, 0.15f, ESP.ThemeColor.W * 0.5f);

            Vector4 borderColor = enabled ?
                new Vector4(
                    ESP.ThemeColor.X * (0.8f + 0.2f * _animations[id]),
                    ESP.ThemeColor.Y * (0.8f + 0.2f * _animations[id]),
                    ESP.ThemeColor.Z * (0.8f + 0.2f * _animations[id]),
                    ESP.ThemeColor.W
                ) :
                new Vector4(0.3f, 0.3f, 0.3f, ESP.ThemeColor.W * 0.5f);

            draw.AddRectFilled(pos, pos + size, ImGui.GetColorU32(bgColor), rounding);
            draw.AddRect(pos, pos + size, ImGui.GetColorU32(borderColor), rounding, ImDrawFlags.None, 2f);

            Vector2 textSize = ImGui.CalcTextSize(label);
            Vector2 textPos = pos + (size - textSize) * 0.5f;
            Vector4 textColor = enabled ?
                new Vector4(1f, 1f, 1f, ESP.ThemeColor.W) :
                new Vector4(0.6f, 0.6f, 0.6f, ESP.ThemeColor.W * 0.5f);

            draw.AddText(textPos, ImGui.GetColorU32(textColor), label);

            ImGui.SetCursorPos(originalCursorPos);
            ImGui.InvisibleButton(id, size);
            bool hovered = ImGui.IsItemHovered() && enabled;
            bool clicked = ImGui.IsItemClicked() && enabled;

            float targetAnim = hovered ? 1f : 0f;
            _animations[id] += (targetAnim - _animations[id]) * ImGui.GetIO().DeltaTime * 10f;
            _animations[id] = System.Math.Clamp(_animations[id], 0f, 1f);

            if (clicked)
            {
                draw.AddRectFilled(pos, pos + size, ImGui.GetColorU32(new Vector4(1, 1, 1, 0.1f)), rounding);
            }

            ImGui.SetCursorPos(originalCursorPos + new Vector2(0, size.Y + 4f));

            return clicked;
        }
    }
}
