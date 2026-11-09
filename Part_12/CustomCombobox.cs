using ImGuiNET;
using System.Numerics;

namespace AotForms.Components
{
    public static class CustomCombobox
    {
        public static bool CaxCustomCombobox(string label, ref int selectedIndex, string[] items, float width = 180f)
        {
            float shiftRight = 10f;
            ImGui.BeginGroup();

            ImGui.PushStyleColor(ImGuiCol.FrameBg, new Vector4(0.10f, 0.10f, 0.10f, 1f));
            ImGui.PushStyleColor(ImGuiCol.FrameBgHovered, new Vector4(0.15f, 0.15f, 0.15f, 1f));
            ImGui.PushStyleColor(ImGuiCol.Border, new Vector4(0.35f, 0.35f, 0.35f, 1f));
            ImGui.PushStyleColor(ImGuiCol.HeaderHovered, new Vector4(ESP.ThemeColor.X, ESP.ThemeColor.Y, ESP.ThemeColor.Z, 0.5f));
            ImGui.PushStyleColor(ImGuiCol.HeaderActive, ESP.ThemeColor);
            ImGui.PushStyleColor(ImGuiCol.Text, new Vector4(1f, 1f, 1f, 1f));

            ImGui.PushStyleVar(ImGuiStyleVar.FrameRounding, 4f);
            ImGui.PushStyleVar(ImGuiStyleVar.FramePadding, new Vector2(10f, 4f));
            ImGui.PushStyleVar(ImGuiStyleVar.FrameBorderSize, 1.5f);

            ImGui.SetCursorPosX(ImGui.GetCursorPosX() + shiftRight);
            ImGui.SetNextItemWidth(width);
            bool changed = ImGui.Combo($"  {label}", ref selectedIndex, items, items.Length);

            ImGui.PopStyleVar(3);
            ImGui.PopStyleColor(6);
            ImGui.EndGroup();
            return changed;
        }
    }
}
