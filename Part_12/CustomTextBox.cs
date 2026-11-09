using System.Collections.Generic;
using System.Numerics;
using ImGuiNET;

namespace AotForms.Components
{
    public static class CustomTextBox
    {
        private static Dictionary<string, float> _animations = new Dictionary<string, float>();

        public static bool CaxCustomTextBox(string label, ref string text, float width = 0, bool password = false)
        {
            bool changed = false;
            string id = $"##{label}";

            if (!_animations.ContainsKey(id))
                _animations[id] = 0f;

            var draw = ImGui.GetWindowDrawList();
            Vector2 pos = ImGui.GetCursorScreenPos();
            Vector2 size = new Vector2(width > 0 ? width : ImGui.GetContentRegionAvail().X, 28f);

            Vector2 originalCursorPos = ImGui.GetCursorPos();

            Vector2 labelSize = ImGui.CalcTextSize(label);
            Vector2 labelPos = pos + new Vector2(8f, -labelSize.Y - 2f);
            draw.AddText(labelPos, ImGui.GetColorU32(new Vector4(0.8f, 0.8f, 0.8f, ESP.ThemeColor.W)), label);

            ImGui.SetCursorScreenPos(pos);
            ImGui.PushItemWidth(size.X);
            ImGui.PushStyleVar(ImGuiStyleVar.FramePadding, new Vector2(8f, 4f));
            ImGui.PushStyleVar(ImGuiStyleVar.FrameRounding, 4f);

            ImGui.PushStyleColor(ImGuiCol.FrameBg, Vector4.Zero);
            ImGui.PushStyleColor(ImGuiCol.Border, Vector4.Zero);
            ImGui.PushStyleColor(ImGuiCol.Text, new Vector4(1f, 1f, 1f, ESP.ThemeColor.W));

            if (password)
            {
                changed = ImGui.InputText($"##input_{label}", ref text, 256, ImGuiInputTextFlags.Password);
            }
            else
            {
                changed = ImGui.InputText($"##input_{label}", ref text, 256);
            }

            bool hovered = ImGui.IsItemHovered();
            bool active = ImGui.IsItemActive();

            float targetAnim = (active || hovered) ? 1f : 0f;
            _animations[id] += (targetAnim - _animations[id]) * ImGui.GetIO().DeltaTime * 10f;
            _animations[id] = System.Math.Clamp(_animations[id], 0f, 1f);

            ImGui.PopStyleColor(3);
            ImGui.PopStyleVar(2);
            ImGui.PopItemWidth();

            Vector4 bgColor = new Vector4(0.1f, 0.1f, 0.1f, ESP.ThemeColor.W * 0.8f);
            Vector4 borderColor = new Vector4(
                ESP.ThemeColor.X * 0.6f + 0.4f * _animations[id],
                ESP.ThemeColor.Y * 0.6f + 0.4f * _animations[id],
                ESP.ThemeColor.Z * 0.6f + 0.4f * _animations[id],
                ESP.ThemeColor.W
            );

            draw.AddRectFilled(pos, pos + size, ImGui.GetColorU32(bgColor), 4f);
            draw.AddRect(pos, pos + size, ImGui.GetColorU32(borderColor), 4f, ImDrawFlags.None, 1.5f);

            ImGui.SetCursorPos(originalCursorPos + new Vector2(0, size.Y + 4f));

            return changed;
        }
    }
}
