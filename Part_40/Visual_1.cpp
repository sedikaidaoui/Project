#include "Visual.hpp"
#include "GameTimer.hpp"
#include <src/Globals.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <Windows.h>
#include <imgui.h>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <imgui_internal.h>
#include <src/Fonts/Fonts.hpp>
#include "EspLines\Math\Vector\Vector3.hpp"
#include "NameGun.h"
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>

struct HealthBarState {
    float AnimatedHealth = 1.0f;
    float LastUpdateTime = 0.0f;
};

static std::unordered_map<int, HealthBarState> healthBarStates;

void DrawHorizontalHealthBarTop(short CurrentHealth, short MaxHealth, float x, float y, float w, int GlowRadius = 10)
{
    ImDrawList* DrawList = ImGui::GetForegroundDrawList();
    if (MaxHealth <= 0) return;

    float HealthPercentage = static_cast<float>(CurrentHealth) / MaxHealth;
    float FilledBarWidth = w * HealthPercentage;
    float BarHeight = 2.5f;

    ImVec4 GreenColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    ImVec4 YellowColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
    ImVec4 RedColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

    ImVec4 HealthColor = (HealthPercentage > 0.5f)
        ? ImLerp(GreenColor, YellowColor, (1.0f - HealthPercentage) * 2.0f)
        : ImLerp(YellowColor, RedColor, (0.5f - HealthPercentage) * 2.0f);

    ImVec4 GlowColor = (HealthPercentage > 0.5f) ? GreenColor :
        (HealthPercentage > 0.25f) ? YellowColor : RedColor;

    ImU32 BarColorU32 = ImGui::ColorConvertFloat4ToU32(HealthColor);

    // background
    DrawList->AddRectFilled(
        ImVec2(x, y),
        ImVec2(x + w, y + BarHeight),
        IM_COL32(0, 0, 0, 128)
    );

    // glow pass
    for (int i = GlowRadius; i > 0; --i) {
        float Alpha = 25.0f * (1.0f - ((float)i / GlowRadius));
        DrawList->AddRectFilled(
            ImVec2(x - i, y - i),
            ImVec2(x + FilledBarWidth + i, y + BarHeight + i),
            IM_COL32(
                int(GlowColor.x * 255),
                int(GlowColor.y * 255),
                int(GlowColor.z * 255),
                (int)Alpha
            )
        );
    }

    // filled bar
    DrawList->AddRectFilled(
        ImVec2(x, y),
        ImVec2(x + FilledBarWidth, y + BarHeight),
        BarColorU32
    );
}


void DrawVerticalHealthBar(short CurrentHealth, short MaxHealth, ImVec2 Position, float TotalHeight, int GlowRadius = 10) {
    ImDrawList* DrawList = ImGui::GetForegroundDrawList();

    float HealthPercentage = static_cast<float>(CurrentHealth) / MaxHealth;
    float FilledBarHeight = TotalHeight * HealthPercentage;
    float BarWidth = 2.5f;

    ImVec4 GreenColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    ImVec4 YellowColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
    ImVec4 RedColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

    ImVec4 HealthBarColor;
    if (HealthPercentage > 0.5f) {
        HealthBarColor = ImLerp(GreenColor, YellowColor, (1.0f - HealthPercentage) * 2.0f);
    }
    else {
        HealthBarColor = ImLerp(YellowColor, RedColor, (0.5f - HealthPercentage) * 2.0f);
    }

    ImU32 BarColorU32 = ImGui::ColorConvertFloat4ToU32(HealthBarColor);

    DrawList->AddRectFilled(
        ImVec2(Position.x, Position.y),
        ImVec2(Position.x + BarWidth, Position.y + TotalHeight),
        IM_COL32(0, 0, 0, 128)
    );

    ImVec4 GlowColor;
    if (HealthPercentage > 0.5f) {
        GlowColor = GreenColor;
    }
    else if (HealthPercentage > 0.25f) {
        GlowColor = YellowColor;
    }
    else {
        GlowColor = RedColor;
    }

    for (int i = GlowRadius; i > 0; --i) {
        float Alpha = 25.0f * (1.0f - ((float)i / GlowRadius));
        DrawList->AddRectFilled(
            ImVec2(Position.x - i, Position.y + (TotalHeight - FilledBarHeight) - i),
            ImVec2(Position.x + BarWidth + i, Position.y + TotalHeight + i),
            IM_COL32((int)(GlowColor.x * 255), (int)(GlowColor.y * 255), (int)(GlowColor.z * 255), (int)Alpha)
        );
    }

    DrawList->AddRectFilled(
        ImVec2(Position.x, Position.y + (TotalHeight - FilledBarHeight)),
        ImVec2(Position.x + BarWidth, Position.y + TotalHeight),
        BarColorU32
    );
}

void DrawHealthBarBelowFullBox(short CurrentHealth, short MaxHealth, float x, float y, float w, float h, int GlowRadius = 10) {
    ImDrawList* DrawList = ImGui::GetForegroundDrawList();

    // Prevent divide-by-zero
    if (MaxHealth <= 0) return;

    float HealthPercentage = static_cast<float>(CurrentHealth) / MaxHealth;
    float FilledBarWidth = w * HealthPercentage;
    float BarHeight = 2.5f;
    float offsetY = 5.0f;

    // Gradient colors
    ImVec4 GreenColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    ImVec4 YellowColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
    ImVec4 RedColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

    ImVec4 HealthBarColor = (HealthPercentage > 0.5f)
        ? ImLerp(GreenColor, YellowColor, (1.0f - HealthPercentage) * 2.0f)
        : ImLerp(YellowColor, RedColor, (0.5f - HealthPercentage) * 2.0f);

    ImVec4 GlowColor = (HealthPercentage > 0.5f) ? GreenColor :
        (HealthPercentage > 0.25f) ? YellowColor : RedColor;

    ImU32 BarColorU32 = ImGui::ColorConvertFloat4ToU32(HealthBarColor);

    // Background bar
    DrawList->AddRectFilled(
        ImVec2(x, y + h + offsetY),
        ImVec2(x + w, y + h + offsetY + BarHeight),
        IM_COL32(0, 0, 0, 128)
    );

    // Glow layers
    for (int i = GlowRadius; i > 0; --i) {
        float Alpha = 30.0f * (1.0f - ((float)i / GlowRadius));
        DrawList->AddRectFilled(
            ImVec2(x - i, y + h + offsetY - i),
            ImVec2(x + FilledBarWidth + i, y + h + offsetY + BarHeight + i),
            IM_COL32((int)(GlowColor.x * 255), (int)(GlowColor.y * 255), (int)(GlowColor.z * 255), (int)Alpha)
        );
    }

    // Filled portion of the health bar
    DrawList->AddRectFilled(
        ImVec2(x, y + h + offsetY),
        ImVec2(x + FilledBarWidth, y + h + offsetY + BarHeight),
        BarColorU32
    );

    // Draw health text (e.g., "135 HP")
    char healthText[12];
    snprintf(healthText, sizeof(healthText), "%d HP", CurrentHealth);

    ImVec2 textSize = ImGui::CalcTextSize(healthText);
    ImVec2 textPos = ImVec2(x + (w / 2.0f) - (textSize.x / 2.0f), y + h + offsetY + BarHeight + 2.0f); // Below the bar

    // Optional: Change text color based on health
    ImU32 textColor = IM_COL32(255, 255, 255, 230); // default white
    if (HealthPercentage <= 0.25f)
        textColor = IM_COL32(255, 60, 60, 230); // red-ish
    else if (HealthPercentage <= 0.5f)
        textColor = IM_COL32(255, 215, 0, 230); // yellow

    DrawList->AddText(textPos, textColor, healthText);
}


void DrawPlayerHealthBar(
    int entityId,
    short CurrentHealth,
    short MaxHealth,
    const ImVec2& headPos,
    float boxWidth,
    float boxHeight,
    int positionType)
{
    if (MaxHealth <= 0 || positionType == 0) return;

    float x = headPos.x - (boxWidth / 2.0f);
    float y = headPos.y;
    float w = boxWidth;
    float h = boxHeight;

    switch (positionType)
    {
    case 1: // Left
    {
        ImVec2 barPos(x - 8.0f, y);
        DrawVerticalHealthBar(CurrentHealth, MaxHealth, barPos, h);
        break;
    }

    case 2: // Right
    {
        ImVec2 barPos(x + w + 4.0f, y);
        DrawVerticalHealthBar(CurrentHealth, MaxHealth, barPos, h);
        break;
    }

    case 3: // Top
    {
        float barWidth = w;
        float barHeight = 4.0f;
        ImVec2 barPos(x, y - (barHeight + 6.0f));

        DrawHorizontalHealthBarTop(CurrentHealth, MaxHealth, barPos.x, barPos.y, barWidth);
        break;
    }

    case 4: // Bottom
    {
        DrawHealthBarBelowFullBox(CurrentHealth, MaxHealth, x, y, w, h);
        break;
    }
    }
}

void DrawGlowCorneredBox(float x, float y, float w, float h, ImColor color, float thickness, float glowRadius, float feather, bool fillEnabled) {
    auto drawList = ImGui::GetForegroundDrawList();

    float newWidth = w;
    float newHeight = h * 1.15f;
    float newY = y - (newHeight - h);

    if (g_Globals.Visuals.FilledBox) {
        drawList->AddRectFilled(
            ImVec2(x, newY),
            ImVec2(x + newWidth, newY + newHeight),
            ImColor(
                g_Globals.Visuals.FillColor[0],
                g_Globals.Visuals.FillColor[1],
                g_Globals.Visuals.FillColor[2],
                g_Globals.Visuals.FillColor[3]
            )
        );
    }

    for (int i = (int)glowRadius; i > 0; --i) {
        float alphaFactor = 25.0f * (1.0f - ((float)i / glowRadius));
        ImColor glowColor = ImColor(color.Value.x, color.Value.y, color.Value.z, alphaFactor / 255.0f);

        drawList->AddLine(ImVec2(x - i, newY - i), ImVec2(x + newWidth / 6 + i, newY - i), glowColor, thickness);
        drawList->AddLine(ImVec2(x - i, newY - i), ImVec2(x - i, newY + newHeight / 6 + i), glowColor, thickness);

        drawList->AddLine(ImVec2(x + newWidth + i, newY - i), ImVec2(x + newWidth - newWidth / 6 - i, newY - i), glowColor, thickness);
        drawList->AddLine(ImVec2(x + newWidth + i, newY - i), ImVec2(x + newWidth + i, newY + newHeight / 6 + i), glowColor, thickness);

        drawList->AddLine(ImVec2(x - i, newY + newHeight + i), ImVec2(x + newWidth / 3 + i, newY + newHeight + i), glowColor, thickness);
        drawList->AddLine(ImVec2(x - i, newY + newHeight + i), ImVec2(x - i, newY + newHeight - newHeight / 6 - i), glowColor, thickness);

        drawList->AddLine(ImVec2(x + newWidth + i, newY + newHeight + i), ImVec2(x + newWidth - newWidth / 6 - i, newY + newHeight + i), glowColor, thickness);
        drawList->AddLine(ImVec2(x + newWidth + i, newY + newHeight + i), ImVec2(x + newWidth + i, newY + newHeight - newHeight / 6 - i), glowColor, thickness);
    }

    drawList->AddLine(ImVec2(x, newY), ImVec2(x + newWidth / 6, newY), color, thickness);
    drawList->AddLine(ImVec2(x, newY), ImVec2(x, newY + newHeight / 6), color, thickness);

    drawList->AddLine(ImVec2(x + newWidth, newY), ImVec2(x + newWidth - newWidth / 6, newY), color, thickness);
    drawList->AddLine(ImVec2(x + newWidth, newY), ImVec2(x + newWidth, newY + newHeight / 6), color, thickness);

    drawList->AddLine(ImVec2(x, newY + newHeight), ImVec2(x + newWidth / 6, newY + newHeight), color, thickness);
    drawList->AddLine(ImVec2(x, newY + newHeight), ImVec2(x, newY + newHeight - newHeight / 6), color, thickness);

    drawList->AddLine(ImVec2(x + newWidth, newY + newHeight), ImVec2(x + newWidth - newWidth / 6, newY + newHeight), color, thickness);
    drawList->AddLine(ImVec2(x + newWidth, newY + newHeight), ImVec2(x + newWidth, newY + newHeight - newHeight / 6), color, thickness);
}

void DrawFullBox(float x, float y, float w, float h, ImColor color, float thickness) {
    auto drawList = ImGui::GetForegroundDrawList();

    float newWidth = w * 0.95f;
    float newHeight = h * 1.15f;
    float newX = x + (w - newWidth) / 2.0f;
    float newY = y - (newHeight - h);

    if (g_Globals.Visuals.FillColorBox) {
        drawList->AddRectFilled(
            ImVec2(newX, newY),
            ImVec2(newX + newWidth, newY + newHeight),
            ImColor(
                g_Globals.Visuals.FillColor[0],
                g_Globals.Visuals.FillColor[1],
                g_Globals.Visuals.FillColor[2],
                g_Globals.Visuals.FillColor[3]
            )
        );
    }

    drawList->AddRect(
        ImVec2(newX, newY),
        ImVec2(newX + newWidth, newY + newHeight),
        color,
        1.0f,
        ImDrawFlags_None,
        thickness * 0.5f
    );
}

void DrawGlowLine(const ImVec2& start, const ImVec2& end, ImU32 color, float thickness, float glowRadius, float feather)
{
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImVec4 colorVec = ImGui::ColorConvertU32ToFloat4(color);

    // Outer glow layers
    for (float i = glowRadius; i > 0; i -= feather)
    {
        // Calculate alpha for the outer layers based on current radius and feathering
        float alpha = colorVec.w * (i / glowRadius) * 0.02f; // Adjust glow strength

        ImU32 glowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(colorVec.x, colorVec.y, colorVec.z, alpha));

        // Draw each outer layer with increasing thickness to create a soft glow effect
        drawList->AddLine(start, end, glowColor, thickness + i);
    }

    // Main line at the center
    drawList->AddLine(start, end, color, thickness);
}

ImVec4 HSVtoRGB(float h, float s, float v)
{
    float r, g, b;

    int i = static_cast<int>(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (i % 6) {
    case 0: r = v, g = t, b = p; break;
    case 1: r = q, g = v, b = p; break;
    case 2: r = p, g = v, b = t; break;
    case 3: r = p, g = q, b = v; break;
    case 4: r = t, g = p, b = v; break;
    case 5: r = v, g = p, b = q; break;
    default: r = g = b = 0; break;
    }

    return ImVec4(r, g, b, 1.0f);
}


void DrawGlowLineGradient(const ImVec2& start, const ImVec2& end, float thickness, float glowRadius, float feather)
{
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    const int segments = 60;  // Number of small segments (higher = smoother gradient)

    // Calculate segment vector
    ImVec2 diff = ImVec2((end.x - start.x) / segments, (end.y - start.y) / segments);

    float time = static_cast<float>(ImGui::GetTime());
    // We'll animate hue over time for cycling effect
    float speed = 0.3f;

    for (int i = 0; i < segments; ++i) {
        ImVec2 segStart = ImVec2(start.x + diff.x * i, start.y + diff.y * i);
        ImVec2 segEnd = ImVec2(start.x + diff.x * (i + 1), start.y + diff.y * (i + 1));

        // Calculate hue based on vertical position + time to create moving gradient
        float t = (float)i / segments;
        float hue = fmodf(t + time * speed, 1.0f);

        // Convert HSV to RGB
        ImVec4 colorVec = HSVtoRGB(hue, 1.0f, 1.0f);

        // Draw glow layers per segment
        for (float glowStep = glowRadius; glowStep > 0; glowStep -= feather) {
            float alpha = (glowStep / glowRadius) * 0.02f;  // glow alpha scaling
            ImU32 glowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(colorVec.x, colorVec.y, colorVec.z, alpha));
            drawList->AddLine(segStart, segEnd, glowColor, thickness + glowStep);
        }

        // Draw main segment line
        ImU32 mainColor = ImGui::ColorConvertFloat4ToU32(ImVec4(colorVec.x, colorVec.y, colorVec.z, 1.0f));
        drawList->AddLine(segStart, segEnd, mainColor, thickness);
    }
}

void ESP::Players() 
{
    if (!g_Globals.EspConfig.Matrix) return;

    SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
    for (auto& [entityID, player] : g_Globals.EspConfig.Entities) 
    {
        if (player.IsDead || !player.IsKnown) 
        {
            continue;
        }

        // --- FINAL BOT FILTER ---
        bool nameIsBot = (player.Name.find("BOT") != std::string::npos || 
                          player.Name.find("Bot") != std::string::npos || 
                          player.Name.find("bot") != std::string::npos ||
                          player.Name == "BOT");

        if (g_Globals.AimBot.IgnoreBots && (player.IsBot || nameIsBot)) {
            continue;
        }

        if (!g_Globals.Visuals.Wukong)
        {
            if (!player.IsVisible)
                continue;
        }



        float dist = player.Distance;
        if (dist > g_Globals.Visuals.DistanceEsp) {
            continue;
        }

        auto IsOffScreen = [](const ImVec2& pos) { return pos.x < -1000 || pos.y < -1000; };
        const std::vector<ImVec2> Bones = {
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.Head, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.Neck, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.LeftShoulder, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.RightShoulder, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.LeftElbow, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.RightElbow, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.LeftWrist, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.RightWrist, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.Hip, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.LeftAnkle, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.RightAnkle, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.Root, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.Groin, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.LeftFoot, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.RightFoot, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height)
        };

        const ImVec2& headPos = Bones[0];
        const ImVec2& rootPos = Bones[11];

        if (rootPos.x == -99999.0f || headPos.x == -99999.0f) continue;

        const ImVec2& neckPos = Bones[1];
        const ImVec2& leftShoulderPos = Bones[2];
        const ImVec2& rightShoulderPos = Bones[3];
        const ImVec2& leftElbowPos = Bones[4];
        const ImVec2& rightElbowPos = Bones[5];
        const ImVec2& leftWristPos = Bones[6];
        const ImVec2& rightWristPos = Bones[7];
        const ImVec2& hipPos = Bones[8];
        const ImVec2& leftAnklePos = Bones[9];
        const ImVec2& rightAnklePos = Bones[10];
        const ImVec2& groinPos = Bones[12];
        const ImVec2& leftFootPos = Bones[13];
        const ImVec2& rightFootPos = Bones[14];

        Vector3 ankleMidPoint = (player.LeftAnkle + player.RightAnkle) * 0.5f;
        Vector3 hipDirection = (player.LeftShoulder - player.RightShoulder).Normalized(true);
        Vector3 leftHipDirection = hipDirection;
        Vector3 rightHipDirection = -hipDirection;
        float ankleDistance = (player.LeftAnkle - player.RightAnkle).Magnitude(true);
        float baseHipWidth = ankleDistance * g_Globals.Visuals.HipWidthScale;
        Vector3 verticalPositionLeft = Vector3::Lerp(player.LeftAnkle, player.Hip, g_Globals.Visuals.LeftHipHeightOffset);
        Vector3 verticalPositionRight = Vector3::Lerp(player.RightAnkle, player.Hip, g_Globals.Visuals.RightHipHeightOffset);
        Vector3 leftHip = verticalPositionLeft + (leftHipDirection * (baseHipWidth * g_Globals.Visuals.HipWidthOffset));
        Vector3 rightHip = verticalPositionRight + (rightHipDirection * (baseHipWidth * g_Globals.Visuals.HipWidthOffset));
        ImVec2 leftHipPosition = W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, leftHip, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
        ImVec2 rightHipPosition = W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, rightHip, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);

        float boxHeight;
        float boxWidth;

        if (headPos.x != 0 && headPos.y != 0 && rootPos.x != 0 && rootPos.y != 0) {
            boxHeight = fabsf(headPos.y - rootPos.y);
            boxWidth = boxHeight * 0.65f;
        }
        else {
            boxHeight = 50.0f;
            boxWidth = 30.0f;
        }

        if (g_Globals.Visuals.Lines)
        {
            auto drawList = ImGui::GetBackgroundDrawList();

            // -----------------------------
            // Base line color
            // -----------------------------
            ImColor snapLineColor(
                g_Globals.Visuals.LinesColor[0],
                g_Globals.Visuals.LinesColor[1],
                g_Globals.Visuals.LinesColor[2],
                g_Globals.Visuals.LinesColor[3]
            );

            ImColor knockedColor(
                g_Globals.Visuals.KnockedColor[0],
                g_Globals.Visuals.KnockedColor[1],
                g_Globals.Visuals.KnockedColor[2],
                g_Globals.Visuals.KnockedColor[3]
            );

            ImColor baseColor;

            // Rainbow overrides everything except knocked
            if (g_Globals.Visuals.RainbowLines)
            {
                float t = static_cast<float>(ImGui::GetTime());
                float hue = fmodf(t * 0.30f, 1.0f);
                ImVec4 rgb = HSVtoRGB(hue, 1.f, 1.f);
                baseColor = ImColor(rgb);
            }
            else
            {
                // Visibility override
                if (g_Globals.Visuals.Wukong && !player.IsVisible)
                    baseColor = ImColor(0.6f, 0.0f, 1.0f, 1.0f);
                else
                    baseColor = snapLineColor;
            }

            // ---------------------------------
            // KNOCKED OVERRIDE
            // ---------------------------------
            ImColor finalColor = player.IsKnocked ? knockedColor : baseColor;
            ImU32 finalColorU32 = finalColor;

            // -----------------------------
            // Attach point logic
            // -----------------------------
            ImVec2 screenTop(g_Globals.EspConfig.Width / 2.f, 0.f);
            ImVec2 screenBottom(g_Globals.EspConfig.Width / 2.f, (float)g_Globals.EspConfig.Height);

            float boxW = boxHeight * 0.65f;
            float boxX = headPos.x - (boxW / 2.f);
            float boxY = headPos.y;

            float newW = boxW * 0.95f;
            float newH = boxHeight * 1.15f;
            float newX = boxX + (boxW - newW) * 0.5f;
            float newY = boxY - (newH - boxHeight);

            ImVec2 attachPoint(newX + newW * 0.5f, newY);

            if (g_Globals.Visuals.Name)
                attachPoint.y = headPos.y - 57.f;

            float thickness = g_Globals.Visuals.LineThickness;
            float glowRadius = g_Globals.Visuals.GlowRadius;
            float glowFeather = g_Globals.Visuals.GlowFeather;

            // -----------------------------
            // Draw function with glow + knocked support
            // -----------------------------
            auto DrawLine = [&](const ImVec2& a, const ImVec2& b)
                {
                    // Glow enabled
                    if (g_Globals.Visuals.GlowLines)
                    {
                        // If knocked → use knocked glow color
                        if (player.IsKnocked)
                        {
                            DrawGlowLine(a, b,
                                (ImU32)knockedColor,
                                thickness, glowRadius, glowFeather
                            );

                        }
                        // Rainbow glow
                        else if (g_Globals.Visuals.RainbowLines)
                        {
                            DrawGlowLineGradient(a, b, thickness, glowRadius, glowFeather);
                        }
                        // Normal glow
                        else
                        {
                            DrawGlowLine(a, b, finalColorU32, thickness, glowRadius, glowFeather);
                        }
                    }
                    else
                    {
                        // No glow → simple line
                        drawList->AddLine(a, b, finalColorU32, thickness);
                    }
                };

            // -----------------------------
            // Draw line depending on mode
            // -----------------------------
            switch (g_Globals.Visuals.EspLines)
            {
            case 1: DrawLine(attachPoint, screenTop); break;
            case 2: DrawLine(screenBottom, attachPoint); break;
            }
        }



        if (g_Globals.Visuals.Skeleton) {
            ImColor SkeletonColor = ImColor(
                g_Globals.Visuals.SkeletonColor[0],
                g_Globals.Visuals.SkeletonColor[1],
                g_Globals.Visuals.SkeletonColor[2],
                g_Globals.Visuals.SkeletonColor[3]
            );

            struct SkeletonDrawer {
                ImDrawList* drawList;
                ImColor color;
                float thickness;

                void Bone(const ImVec2& from, const ImVec2& to) const {
                    drawList->AddLine(from, to, color, g_Globals.Visuals.Thickness);
                }

                void Head(const ImVec2& center, float radius) const {
                    drawList->AddCircle(center, radius, color, 0, g_Globals.Visuals.Thickness);
                }
            };

            ImColor IsKnockedColor = player.IsKnocked ? ImColor(1.f, 0.f, 0.f, 1.f) : SkeletonColor;

            float headRadius = std::clamp(100.0f / player.Distance, 0.5f, 1.5f) * 2.0f;

            SkeletonDrawer skeleton{
                ImGui::GetBackgroundDrawList(),
                IsKnockedColor,
                1.5f
            };

            // --- HEAD & UPPER BODY (same as before) ---
            skeleton.Head(headPos, headRadius);
            skeleton.Bone(headPos, neckPos);
            skeleton.Bone(neckPos, leftShoulderPos);
            skeleton.Bone(neckPos, rightShoulderPos);
            skeleton.Bone(leftShoulderPos, leftElbowPos);
            skeleton.Bone(rightShoulderPos, rightElbowPos);
            skeleton.Bone(leftElbowPos, leftWristPos);
            skeleton.Bone(rightElbowPos, rightWristPos);

            // --- SPINE: Neck → Hip ---
            skeleton.Bone(neckPos, hipPos);

            // --- LOWER BODY: Use real Groin bone like reference project ---
            // Hip → Groin (pelvis split point)
            skeleton.Bone(hipPos, groinPos);
            // Groin → Left leg → foot
            skeleton.Bone(groinPos, leftAnklePos);
            skeleton.Bone(leftAnklePos, leftFootPos);
            // Groin → Right leg → foot
            skeleton.Bone(groinPos, rightAnklePos);
            skeleton.Bone(rightAnklePos, rightFootPos);

            if (g_Globals.Visuals.Debug) {
                ImGui::GetBackgroundDrawList()->AddCircle(groinPos, 3.0f, IM_COL32(255, 0, 0, 255));
                ImGui::GetBackgroundDrawList()->AddCircle(leftFootPos, 3.0f, IM_COL32(0, 255, 0, 255));
                ImGui::GetBackgroundDrawList()->AddCircle(rightFootPos, 3.0f, IM_COL32(0, 0, 255, 255));
                ImGui::GetBackgroundDrawList()->AddCircle(hipPos, 3.0f, IM_COL32(255, 255, 0, 255));
            }
        }


        if (g_Globals.Visuals.ESPWeapon || g_Globals.Visuals.ESPWeaponIcon) {
            auto drawList = ImGui::GetForegroundDrawList();
            Namegun::Init();

            float verticalOffset = 15.0f; // Start offset below the player box/root

            ImColor weaponColor = ImColor(
                g_Globals.Visuals.GunColor[0],
                g_Globals.Visuals.GunColor[1],
                g_Globals.Visuals.GunColor[2],
                g_Globals.Visuals.GunColor[3]
            );

            if (g_Globals.Visuals.RGB) {
                float time = ImGui::GetTime();
                weaponColor = ImColor::HSV(fmod(time * 2.0f, 1.0f), 1.0f, 1.0f);
            }

            auto outlineColor = ImColor(0, 0, 0, 180);
            float outlineSize = 1.0f;

            if (g_Globals.Visuals.ESPWeaponIcon) {
                std::string iconStr = Namegun::GetGunIcon(player.WeaponID);
                if (!iconStr.empty()) {
                    ImFont* iconFont = FWork::Fonts::IconWeapon;
                    float iconFontScale = 0.65f; // Decreased size for compact view
                    float iconFontSize = iconFont->FontSize * iconFontScale;

                    ImVec2 iconSize = iconFont->CalcTextSizeA(iconFontSize, FLT_MAX, 0.0f, iconStr.c_str());
                    ImVec2 iconPos(rootPos.x - (iconSize.x / 2.0f), rootPos.y + verticalOffset);

                    // Draw outline for icon
                    for (float xOffset = -outlineSize; xOffset <= outlineSize; xOffset += outlineSize)
                        for (float yOffset = -outlineSize; yOffset <= outlineSize; yOffset += outlineSize)
                            if (xOffset != 0.0f || yOffset != 0.0f)
                                drawList->AddText(iconFont, iconFontSize, ImVec2(iconPos.x + xOffset, iconPos.y + yOffset), outlineColor, iconStr.c_str());

                    drawList->AddText(iconFont, iconFontSize, iconPos, weaponColor, iconStr.c_str());

                    verticalOffset += iconSize.y + 1.0f; // push next text down
                }
            }

            if (g_Globals.Visuals.ESPWeapon) {
                std::string gunName = Namegun::GetGunName(player.WeaponID);
                ImFont* textFont = FWork::Fonts::InterBold;
                float textFontScale = 0.65f; // Decreased size for compact view
                float textFontSize = textFont->FontSize * textFontScale;

                ImVec2 textSize = textFont->CalcTextSizeA(textFontSize, FLT_MAX, 0.0f, gunName.c_str());
                ImVec2 textPos(rootPos.x - (textSize.x / 2.0f), rootPos.y + verticalOffset);

                // Draw outline for text
                for (float xOffset = -outlineSize; xOffset <= outlineSize; xOffset += outlineSize)
                    for (float yOffset = -outlineSize; yOffset <= outlineSize; yOffset += outlineSize)
                        if (xOffset != 0.0f || yOffset != 0.0f)
                            drawList->AddText(textFont, textFontSize, ImVec2(textPos.x + xOffset, textPos.y + yOffset), outlineColor, gunName.c_str());

                drawList->AddText(textFont, textFontSize, textPos, weaponColor, gunName.c_str());
            }
        }


        if (g_Globals.Visuals.Watermark) 
        {
            auto RenderMainTitle = [](ImDrawList* drawList, float yPos) 
                {
                ImGui::PushFont(FWork::Fonts::InterSemiBold);
                const char* text = "HUB COMMUNITY | FUUL SIFE "; 
                float fontScale = g_Globals.Visuals.TextSize / 15.0f; 
                ImVec2 textSize = ImGui::CalcTextSize(text) * fontScale;
                ImVec2 screenSize = ImGui::GetIO().DisplaySize;
                ImVec2 textPos = ImVec2((screenSize.x - textSize.x) * 0.5f, yPos);

                ImColor textColor = ImColor(g_Globals.Visuals.WatermarkColor[0], g_Globals.Visuals.WatermarkColor[1], g_Globals.Visuals.WatermarkColor[2], g_Globals.Visuals.WatermarkColor[3]);
                ImU32 shadowColor = 0xFF000000;

                drawList->AddText(
                    ImGui::GetFont(),
                    ImGui::GetFontSize()* fontScale,
                    ImVec2(textPos.x + 1, textPos.y + 1),
                    shadowColor,
                    text
                );

                drawList->AddText(
                    ImGui::GetFont(),
                    ImGui::GetFontSize()* fontScale,
                    textPos,
                    textColor,
                    text
                );
                ImGui::PopFont();
                };

            RenderMainTitle(ImGui::GetBackgroundDrawList(), 80.0f);
        }

        if (g_Globals.Visuals.HealthBar)
        {
            float scaledBoxHeight = boxHeight * 1.15f;
            float boxTopY = headPos.y - 1.0f - (scaledBoxHeight - boxHeight);
            float boxLeftX = headPos.x - (boxWidth / 2);
            ImDrawList* DrawList = ImGui::GetForegroundDrawList();

            ImFont* font = FWork::Fonts::InterSemiBold;
            float fontSize = font->FontSize * 0.75f;

            switch (g_Globals.Visuals.players_healthbar)
            {
            case 0:
                break;

                // LEFT vertical bar
            case 1:
            {
                DrawVerticalHealthBar(player.Health, 200, ImVec2(boxLeftX - 5, boxTopY), scaledBoxHeight);

                char healthText[12];
                snprintf(healthText, sizeof(healthText), "%d", player.Health);
                ImVec2 textPos = ImVec2(boxLeftX - 0.5f, boxTopY + scaledBoxHeight + 1.0f);

                float hpPercent = (float)player.Health / 200;
                ImU32 textColor = IM_COL32(255, 255, 255, 200);
                if (hpPercent <= 0.25f) textColor = IM_COL32(255, 60, 60, 220);
                else if (hpPercent <= 0.5f) textColor = IM_COL32(255, 215, 0, 220);

                DrawList->AddText(font, fontSize, textPos, textColor, healthText);
            }
            break;

            // RIGHT vertical bar
            case 2:
            {
                DrawVerticalHealthBar(player.Health, 200, ImVec2(boxLeftX + boxWidth + 5, boxTopY), scaledBoxHeight);

                char healthText[11];
                snprintf(healthText, sizeof(healthText), "%d", player.Health);
                ImVec2 textPos = ImVec2(boxLeftX + boxWidth + 9.5f, boxTopY + scaledBoxHeight + 1.0f);

                float hpPercent = (float)player.Health / 200;
                ImU32 textColor = IM_COL32(255, 255, 255, 200);
                if (hpPercent <= 0.25f) textColor = IM_COL32(255, 60, 60, 220);
                else if (hpPercent <= 0.5f) textColor = IM_COL32(255, 215, 0, 220);

                DrawList->AddText(font, fontSize, textPos, textColor, healthText);
            }
            break;

            // TOP horizontal bar (new)
            case 3:
            {
                float barWidth = boxWidth;
                float barHeight = 4.0f;
                float barX = headPos.x - (boxWidth / 2);
                float barY = boxTopY - (barHeight + 6.0f);

                DrawHorizontalHealthBarTop(player.Health, 200, barX, barY, barWidth);
            }
            break;

            // FULL BOX BOTTOM bar
            case 4:
            {
                DrawHealthBarBelowFullBox(
                    player.Health,
                    200,
                    headPos.x - (boxWidth / 2),
                    headPos.y,
                    boxWidth,
                    boxHeight
                );
            }
            break;

            // SMALL text only (no bar)
            case 5:
            {
                char healthText[10];
                snprintf(healthText, sizeof(healthText), "%d HP", player.Health);

                ImVec2 textPos(boxLeftX + boxWidth + 7.0f, boxTopY + scaledBoxHeight + 1.0f);

                auto getHealthColor = [](float hpPercent) -> ImU32 {
                    if (hpPercent <= 0.25f) return IM_COL32(255, 60, 60, 250);
                    else if (hpPercent <= 0.5f) return IM_COL32(255, 215, 0, 250);
                    else return IM_COL32(3, 252, 32, 250);
                    };

                ImU32 mainColor = getHealthColor((float)player.Health / 200.0f);
                DrawList->AddText(font, fontSize, textPos, mainColor, healthText);
            }
            break;
            }
        }


        if (g_Globals.Visuals.FilledBox) {
            float padding = 1.0f;
            ImColor fillColor = ImColor(
                g_Globals.Visuals.Filledboxcolor[0],
                g_Globals.Visuals.Filledboxcolor[1],
                g_Globals.Visuals.Filledboxcolor[2],
                g_Globals.Visuals.Filledboxcolor[3]
            );

            float filledBoxX = headPos.x - (boxWidth / 2) + padding;
            float filledBoxY = headPos.y + padding;
            float filledBoxWidth = boxWidth - (2 * padding);
            float filledBoxHeight = boxHeight - (2 * padding);

            ImGui::GetBackgroundDrawList()->AddRectFilled(
                ImVec2(filledBoxX, filledBoxY),
                ImVec2(filledBoxX + filledBoxWidth, filledBoxY + filledBoxHeight),
                fillColor);
        }

        if (g_Globals.Visuals.Box) {

            ImColor boxColor;

            if (player.IsVisible) {
                boxColor = ImColor(g_Globals.Visuals.BoxColor[0],
                    g_Globals.Visuals.BoxColor[1],
                    g_Globals.Visuals.BoxColor[2],
                    g_Globals.Visuals.BoxColor[3]);
            }
            else {
                boxColor = ImColor(0.6f, 0.0f, 1.0f, 1.0f);
            }

            float glowRadius = 12.0f;
            float feather = 2.0f;

            switch (g_Globals.Visuals.players_box) {
            case 0:
                break;
            case 1:
                DrawFullBox(headPos.x - (boxWidth / 2), headPos.y - 1.0f, boxWidth, boxHeight, boxColor, 0.5f);
                break;
            case 2:
                DrawGlowCorneredBox(headPos.x - (boxWidth / 2), headPos.y, boxWidth, boxHeight, boxColor, 0.5f, glowRadius, feather, false);
                break;
            }
        }

        if (g_Globals.Visuals.Name || g_Globals.Visuals.Distance || g_Globals.Visuals.Level || g_Globals.Visuals.ID || g_Globals.Visuals.moco)
        {
            ImDrawList* drawList = ImGui::GetForegroundDrawList();

            // ===== COLOR CONFIGURATION =====
            auto CreateColor = [](const float color[4]) -> ImColor {
                return ImColor(color[0], color[1], color[2], color[3]);
                };

            ImColor nameColor = CreateColor(g_Globals.Visuals.NameColor);
            ImColor distColor = CreateColor(g_Globals.Visuals.DistColor);
            ImColor idColor = ImColor(255, 255, 255, 255);
            const ImColor outlineColor = ImColor(0, 0, 0, 180);
            const float outlineSize = 1.0f;

            // RGB Effect
            if (g_Globals.Visuals.RGB) {
                float time = ImGui::GetTime();
                nameColor = ImColor::HSV(fmod(time * 2.0f, 1.0f), 1.0f, 1.0f);
                distColor = ImColor::HSV(fmod(time * 2.0f + 0.33f, 1.0f), 1.0f, 1.0f);
                idColor = ImColor::HSV(fmod(time * 2.0f + 0.66f, 1.0f), 1.0f, 1.0f);
            }

            // ===== FONT CONFIGURATION =====
            ImFont* nameFont = FWork::Fonts::InterRegular;
            const float nameFontSize = nameFont->FontSize * 0.80f;
            const float distFontSize = FWork::Fonts::InterBold->FontSize * 0.70f;
            const float levelFontSize = FWork::Fonts::InterBold->FontSize * 0.75f;

            // Ensure player has a name
            if (player.Name.empty()) {
                player.Name = "BOT";
            }

            // ===== TEXT POSITIONING =====
            constexpr float offsetAboveHead = 50.0f;
            ImVec2 textSize = nameFont->CalcTextSizeA(nameFontSize, FLT_MAX, 0.0f, player.Name.c_str());
            ImVec2 textPos(headPos.x - (textSize.x / 2.0f), headPos.y - offsetAboveHead);

            // ===== DRAWING UTILITIES =====
            auto DrawTextWithOutline = [&](ImFont* font, float fontSize, ImVec2 position, ImColor color, const char* text) {
                const ImVec2 outlineOffsets[] = {
                    {-outlineSize, 0}, {outlineSize, 0},
                    {0, -outlineSize}, {0, outlineSize}
                };

                // Draw outline
                for (const auto& offset : outlineOffsets) {
                    drawList->AddText(font, fontSize,
                        ImVec2(position.x + offset.x, position.y + offset.y),
                        outlineColor, text);
                }

                // Draw main text
                drawList->AddText(font, fontSize, position, color, text);
                };

            auto GetLevelColor = [](int level) -> ImColor {
                if (level <= 10)      return ImColor(15, 200, 210, 255);
                else if (level <= 20) return ImColor(100, 200, 50, 255);
                else if (level <= 30) return ImColor(240, 180, 20, 255);
                else if (level <= 40) return ImColor(230, 140, 20, 255);
                else if (level <= 50) return ImColor(230, 90, 20, 255);
                else if (level <= 60) return ImColor(210, 70, 30, 255);
                else if (level <= 70) return ImColor(220, 30, 30, 255);
                else if (level <= 80) return ImColor(180, 20, 20, 255);
                else if (level <= 90) return ImColor(140, 10, 10, 255);
                else                  return ImColor(100, 0, 0, 255);
                };

            // ===== VISUAL ELEMENTS =====

            // Player Name
            if (g_Globals.Visuals.Name) {
                DrawTextWithOutline(nameFont, nameFontSize, textPos, nameColor, player.Name.c_str());
            }

            // Distance Text
            ImVec2 mocoBasePos;
            if (g_Globals.Visuals.Distance) {
                std::string distanceText = std::to_string(static_cast<int>(std::round(dist))) + "m";
                ImVec2 distSize = FWork::Fonts::InterBold->CalcTextSizeA(distFontSize, FLT_MAX, 0.0f, distanceText.c_str());
                ImVec2 distPos(textPos.x + (textSize.x - distSize.x) / 2.0f, textPos.y + textSize.y + 1.0f);

                DrawTextWithOutline(FWork::Fonts::InterBold, distFontSize, distPos, distColor, distanceText.c_str());
                mocoBasePos = ImVec2(distPos.x + distSize.x / 2.0f, distPos.y + distSize.y);
            }
            else {
                mocoBasePos = ImVec2(textPos.x + textSize.x / 2.0f, textPos.y + textSize.y + 5.0f);
            }

            // MOCO Triangle Indicator
            if (g_Globals.Visuals.moco) {
                const float triangleHeight = 6.0f;
                const float triangleBase = 10.0f;
                ImVec2 triangleCenter(mocoBasePos.x, mocoBasePos.y + 8.0f);

                ImVec2 trianglePoints[] = {
                    ImVec2(triangleCenter.x - triangleBase / 2.0f, triangleCenter.y),
                    ImVec2(triangleCenter.x + triangleBase / 2.0f, triangleCenter.y),
                    ImVec2(triangleCenter.x, triangleCenter.y + triangleHeight)
                };

                drawList->AddTriangleFilled(trianglePoints[0], trianglePoints[1], trianglePoints[2], IM_COL32(15, 252, 3, 255));
            }

            // Player Level Badge
            if (g_Globals.Visuals.Level) {
                std::string levelText = std::to_string(player.Level);
                ImVec2 levelTextSize = FWork::Fonts::InterBold->CalcTextSizeA(levelFontSize, FLT_MAX, 0.0f, levelText.c_str());

                const float paddingX = 6.0f;
                const float paddingY = 2.0f;
                const float spacing = 8.0f;

                ImVec2 boxSize(levelTextSize.x + 2 * paddingX, levelTextSize.y + 2 * paddingY);
                ImVec2 levelPos(textPos.x - boxSize.x - spacing, textPos.y - paddingY);

                // Draw level background
                drawList->AddRectFilled(levelPos,
                    ImVec2(levelPos.x + boxSize.x, levelPos.y + boxSize.y),
                    GetLevelColor(player.Level), 4.0f);

                // Draw level text
                drawList->AddText(FWork::Fonts::InterBold, levelFontSize,
                    ImVec2(levelPos.x + paddingX, levelPos.y + paddingY),
                    IM_COL32(255, 255, 255, 255), levelText.c_str());
            }

            // Player ID
            if (g_Globals.Visuals.ID) {
                std::string idText = "ID: " + std::to_string(player.ID);
                ImVec2 idSize = FWork::Fonts::InterBold->CalcTextSizeA(nameFontSize, FLT_MAX, 0.0f, idText.c_str());
                ImVec2 idPos(textPos.x + (textSize.x - idSize.x) / 2.0f, textPos.y - idSize.y - 5.0f);

                DrawTextWithOutline(FWork::Fonts::InterBold, nameFontSize, idPos, idColor, idText.c_str());
            }
        }

        if (g_Globals.Visuals.HeadDot)
        {
            ImDrawList* drawList = ImGui::GetForegroundDrawList();

            ImColor headDotColor = ImColor(
                g_Globals.Visuals.HeadDotColor[0],
                g_Globals.Visuals.HeadDotColor[1],
                g_Globals.Visuals.HeadDotColor[2],
                g_Globals.Visuals.HeadDotColor[3]
            );

            // Example: headPosition is your 2D screen pos of head bone
            drawList->AddCircleFilled(
                headPos,
                g_Globals.Visuals.HeadDotSize,
                headDotColor,
                32 // segments for smooth circle
            );
        }

        if (g_Globals.Visuals.Enemy) {
            int realCount = 0;
            int botCount = 0;
            float fontScale = g_Globals.Visuals.TextSize / 15.0f; // base 15

            for (const auto& pair : g_Globals.EspConfig.Entities) {
                const auto& entity = pair.second;
                if (entity.IsDead || !entity.IsKnown)
                    continue;

                bool isBot = Mem.Read<bool>(entity.Address + Offsets::IsClientBot);
                if (isBot)
                    botCount++;
                else
                    realCount++;
            }

            // Cache text for ImGui display
            static std::string _cachedEnemyText;
            _cachedEnemyText = "Players: " + std::to_string(realCount) + " | Bots: " + std::to_string(botCount);

            ImGui::PushFont(FWork::Fonts::InterSemiBold);
            ImVec2 textSize = ImGui::CalcTextSize(_cachedEnemyText.c_str()) * fontScale;
            float textPosY = 70 + ImGui::CalcTextSize("Velocira Internal").y + 10;
            Vector2 textPos((g_Globals.EspConfig.Width - textSize.x) / 2, textPosY);

            ImColor textColor(
                g_Globals.Visuals.EnemyColor[0],
                g_Globals.Visuals.EnemyColor[1],
                g_Globals.Visuals.EnemyColor[2],
                g_Globals.Visuals.EnemyColor[3]
            );

            ImU32 shadowColor = 0xFF000000;
            ImGui::GetBackgroundDrawList()->AddText(
                ImGui::GetFont(),
                ImGui::GetFontSize() * fontScale,
                ImVec2(textPos.X + 1, textPos.Y + 1),
                shadowColor,
                _cachedEnemyText.c_str()
            );

            ImGui::GetBackgroundDrawList()->AddText(
                ImGui::GetFont(),
                ImGui::GetFontSize() * fontScale,
                ImVec2(textPos.X, textPos.Y),
                textColor,
                _cachedEnemyText.c_str()
            );
            ImGui::PopFont();
        }

    };

    // Aimbot Mouse Line
    if (g_Globals.EspConfig.InMatch && g_Globals.AimBot.MouseAim && g_Globals.AimBot.TargetPos.X != 0) {
        ImVec2 screenCenter(g_Globals.EspConfig.Width / 2.0f, g_Globals.EspConfig.Height / 2.0f);
        ImVec2 target2D = W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, g_Globals.AimBot.TargetPos, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
        
        if (target2D.x > 0 && target2D.y > 0) {
            ImGui::GetBackgroundDrawList()->AddLine(screenCenter, target2D, IM_COL32(0, 255, 0, 255), 1.0f);
        }
    }

    if (g_Globals.EspConfig.InMatch && g_Globals.Misc.ShowAimbotFov && g_Globals.AimBot.Fov > 0) {
        ImVec2 center(g_Globals.EspConfig.Width / 2.0f, g_Globals.EspConfig.Height / 2.0f);
        auto& fovCol = g_Globals.Misc.AimbotFovColor;
        ImU32 circleColor = IM_COL32(
            (int)(fovCol[0] * 255.0f),
            (int)(fovCol[1] * 255.0f),
            (int)(fovCol[2] * 255.0f),
            (int)(fovCol[3] * 255.0f));
        ImGui::GetBackgroundDrawList()->AddCircle(center, (float)g_Globals.AimBot.Fov, circleColor, 100, 1.0f);
    }

    GameTimer::Draw();
}
