using ImGuiNET;
using System;
using System.Numerics;

namespace AotForms.Components
{
    public static class CustomColourPicker
    {
        public static bool CaxCustomColourPicker(string label, ref Vector4 color)
        {
            ImDrawListPtr drawList = ImGui.GetWindowDrawList();
            float buttonSize = 22f;
            Vector2 cursorPos = ImGui.GetCursorScreenPos();

            float shiftX = 10f;
            ImGui.SetCursorScreenPos(cursorPos + new Vector2(shiftX, 0));

            if (ImGui.ColorButton($"##{label}_color", color, ImGuiColorEditFlags.NoTooltip | ImGuiColorEditFlags.NoAlpha, new Vector2(buttonSize, buttonSize)))
            {
                ImGui.OpenPopup($"##{label}_picker_popup");
            }

            ImGui.SameLine();
            ImGui.Text(label);

            bool changed = false;

            if (ImGui.BeginPopup($"##{label}_picker_popup"))
            {
                Vector2 pickerPos = ImGui.GetCursorScreenPos();
                Vector2 pickerSize = new Vector2(240, 22);

                const int steps = 360;
                float stepWidth = pickerSize.X / steps;
                for (int i = 0; i < steps; i++)
                {
                    float t = i / (float)steps;
                    Vector4 c = GetColorFromHuePosition(t);
                    drawList.AddRectFilled(pickerPos + new Vector2(i * stepWidth, 0),
                                           pickerPos + new Vector2((i + 1) * stepWidth, pickerSize.Y),
                                           ImGui.ColorConvertFloat4ToU32(new Vector4(c.X, c.Y, c.Z, 1f)), 0f);
                }

                ImGui.SetCursorScreenPos(pickerPos);
                ImGui.InvisibleButton("##picker_interaction", pickerSize);

                if (ImGui.IsItemActive() && ImGui.IsMouseDown(ImGuiMouseButton.Left))
                {
                    Vector2 mouse = ImGui.GetMousePos();
                    float relativeX = Math.Clamp((mouse.X - pickerPos.X) / pickerSize.X, 0f, 1f);
                    color = GetColorFromHuePosition(relativeX);
                    changed = true;
                }

                ImGui.EndPopup();
            }

            return changed;
        }

        private static Vector4 GetColorFromHuePosition(float position)
        {
            float h = position * 360f;
            float s = 1f;
            float v = 1f;

            int hi = (int)(h / 60f) % 6;
            float f = (h / 60f) - hi;
            float p = v * (1f - s);
            float q = v * (1f - f * s);
            float t = v * (1f - (1f - f) * s);

            Vector4 rgb = hi switch
            {
                0 => new Vector4(v, t, p, 1f),
                1 => new Vector4(q, v, p, 1f),
                2 => new Vector4(p, v, t, 1f),
                3 => new Vector4(p, q, v, 1f),
                4 => new Vector4(t, p, v, 1f),
                _ => new Vector4(v, p, q, 1f),
            };

            rgb.W = 1f;
            return rgb;
        }
    }
}
