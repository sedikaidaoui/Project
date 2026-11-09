using ImGuiNET;
using System.Numerics;

namespace AotForms.Components
{
    public static class CustomSeparator
    {
        /// <summary>Section divider; default line ~#2A2A2A (subtle on dark panels).</summary>
        public static void Draw(string text, float offset = 15f, Vector4? color = null)
        {
            ImDrawListPtr drawList = ImGui.GetWindowDrawList();
            ImGuiStylePtr style = ImGui.GetStyle();
            Vector2 cursorPos = ImGui.GetCursorScreenPos();
            Vector2 windowPos = ImGui.GetWindowPos();
            Vector2 windowSize = ImGui.GetWindowSize();
            Vector2 textSize = ImGui.CalcTextSize(text);
            float verticalPadding = 4f;
            float startLineLength = 8f;
            float gapAfterStartLine = 6f;
            ImGui.Dummy(new Vector2(0, verticalPadding));
            cursorPos = ImGui.GetCursorScreenPos();
            float centerY = cursorPos.Y + textSize.Y * 0.5f;

            drawList.AddLine(
                new Vector2(cursorPos.X + offset, centerY),
                new Vector2(cursorPos.X + offset + startLineLength, centerY),
                ImGui.GetColorU32(color ?? new Vector4(0.18f, 0.18f, 0.18f, 1f)),
                3.0f
            );

            drawList.AddText(
                new Vector2(cursorPos.X + offset + startLineLength + gapAfterStartLine, cursorPos.Y),
                ImGui.GetColorU32(new Vector4(0.85f, 0.85f, 0.85f, 1f)),
                text
            );

            float lineStartX = cursorPos.X + offset + startLineLength + gapAfterStartLine + textSize.X + 6f;
            float lineEndX = windowPos.X + windowSize.X - style.WindowPadding.X;

            drawList.AddLine(
                new Vector2(lineStartX, centerY),
                new Vector2(lineEndX, centerY),
                ImGui.GetColorU32(color ?? new Vector4(0.18f, 0.18f, 0.18f, 1f)),
                3.0f
            );

            ImGui.Dummy(new Vector2(0, textSize.Y + verticalPadding * 1 + style.ItemSpacing.Y));
        }
    }
}
