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
#include <src/Fonts/FontAwesome.hpp>   // ICON_FA_USER
#include <cstdio>                      // snprintf
#include "EspLines\Math\Vector\Vector3.hpp"
#include "NameGun.h"
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/RankEsp.hpp>              // score de rang -> index d'icone
#include <EspLines/RankIcons.hpp>            // badges de rang embarques dans la DLL
#include <EspLines/Features/Visuals/ArabicShape.hpp>   // noms arabes (shaping + RTL)

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

// ---------------------------------------------------------------------------
// TELEKILL LAG — halqa (da2ira) f blasset l'activation
//
//   * Kat-tser7 f l'ard f la position dyal joueur f moment d'activation.
//   * Kat-bqa F NAFS LBLASSA (ma kat-tbeddelch m3a lmouvement) — la couleur
//     levha bssbha l'3douwwa men l'point :
//         0 m   -> KHDR (green, melli t-activi hya f rdjlek)
//         45 m  -> sfra (limouni)
//         90 m+ -> 7mar kamel   (90 m = l'distance li telekill kay-t7amed)
//   * Kat-t7edder automatiquement melli telekill kay-tfina.
//
//   APPEL : ESP::Players(), l'EntitiesLock DÉJÀ TENU (kay-qra men Entities
//           w MainCamera mnine ma kaynch race m3a Data::Work).
// ---------------------------------------------------------------------------
static void DrawTelekillRing()
{
    static bool    s_on    = false;
    static bool    s_valid = false;
    static Vector3 s_anchor{};

    const bool teleOn =
        g_Globals.Exploits.FakeLag_Tele && g_Globals.EspConfig.InMatch;

    if (!teleOn) {
        s_on    = false;
        s_valid = false;
        return;
    }

    // Position "pieds" du joueur local (fallback : camera - 1.5 m)
    auto localFeet = [](Vector3& out) -> bool {
        const long lp = static_cast<long>(g_Globals.EspConfig.LocalPlayer);
        if (lp != 0) {
            auto it = g_Globals.EspConfig.Entities.find(lp);
            if (it != g_Globals.EspConfig.Entities.end() &&
                it->second.Root != Vector3::Zero()) {
                out = it->second.Root;
                return true;
            }
        }
        const Vector3 cam = g_Globals.EspConfig.MainCamera;
        if (cam != Vector3::Zero()) {
            out = Vector3(cam.X, cam.Y - 1.5f, cam.Z);
            return true;
        }
        return false;
    };

    Vector3 feet{};
    const bool haveFeet = localFeet(feet);

    if (!s_on) {                       // activation : on cale la halqa ici
        s_on    = true;
        s_valid = haveFeet;
        if (haveFeet) s_anchor = feet;
    }
    else if (!s_valid && haveFeet) {   // sinon on retente a la frame suivante
        s_anchor = feet;
        s_valid  = true;
    }

    if (!s_valid || !haveFeet) return;

    constexpr float kRadius  = 1.2f;   // rayon de la halqa (metres)
    constexpr float kMaxDist = 90.0f;  // 90 m -> 7mar kamel (distance telekill)

    // ---- couleur : KHDR (0 m) -> sfra (45 m) -> 7mar (90 m) -----------
    const float t = Vector3::Distance(feet, s_anchor) / kMaxDist;
    int cr, cg, cb;
    if (t < 0.5f) {
        const float u = t * 2.0f;              // khder -> sfra
        cr = (int)(255.0f * u);
        cg = 255;
        cb = 0;
    }
    else {
        const float u = (t - 0.5f) * 2.0f;     // sfra -> 7mar
        cr = 255;
        cg = (int)(255.0f * (1.0f - u));
        cb = 0;
    }
    if (cg < 0) cg = 0;
    if (cr < 0) cr = 0;

    const ImU32 ringCol = IM_COL32(cr, cg, cb, 255);

    // ---- projection au sol + trace segment par segment ----------------
    // (les points derriere la camara sont sautes : pas de trait qui part
    //  en travers l'ecran, on garde juste l'arc visible)
    constexpr int   kSegs = 64;
    constexpr float kStep = 6.2831853f / (float)kSegs;

    ImDrawList*       dl = ImGui::GetBackgroundDrawList();
    const Matrix4x4&  vm = g_Globals.EspConfig.ViewMatrix;
    const int         w  = g_Globals.EspConfig.Width;
    const int         h  = g_Globals.EspConfig.Height;

    ImVec2 prev{};
    bool   prevOk = false;

    for (int i = 0; i <= kSegs; ++i) {
        const float a = (float)i * kStep;
        const Vector3 p(s_anchor.X + std::cos(a) * kRadius,
                        s_anchor.Y,
                        s_anchor.Z + std::sin(a) * kRadius);

        const ImVec2 sp   = W2S::WorldToScreenImVec2(vm, p, w, h);
        const bool   ok   = (sp.x > -5000.0f && sp.y > -5000.0f &&
                             sp.x < 100000.0f && sp.y < 100000.0f);

        if (ok && prevOk) {
            dl->AddLine(prev, sp, IM_COL32(255, 255, 255, 60), 6.0f);  // halo
            dl->AddLine(prev, sp, ringCol, 2.6f);                       // halqa
        }
        prev   = sp;
        prevOk = ok;
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
            // Deteccion: estas apuntando al enemigo?
            // -----------------------------
            bool isTargeted = false;
            if (g_Globals.Visuals.Alvo) {
                ImGuiIO& io = ImGui::GetIO();
                float cx = io.DisplaySize.x * 0.5f;
                float cy = io.DisplaySize.y * 0.5f;

                float bx1 = headPos.x - (boxWidth / 2);
                float by1 = headPos.y;
                float bx2 = headPos.x + (boxWidth / 2);
                float by2 = headPos.y + boxHeight;
                float margin = 15.0f;

                if (cx >= bx1 - margin && cx <= bx2 + margin &&
                    cy >= by1 - margin && cy <= by2 + margin)
                {
                    isTargeted = true;
                }
            }

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

            ImColor alvoColor(
                g_Globals.Visuals.AlvoColor[0],
                g_Globals.Visuals.AlvoColor[1],
                g_Globals.Visuals.AlvoColor[2],
                g_Globals.Visuals.AlvoColor[3]
            );

            ImColor baseColor;

            // Linea VERDE cuando apuntas al enemigo (prioridad sobre rainbow)
            if (isTargeted)
            {
                baseColor = alvoColor;
            }
            // Rainbow overrides knocked-independent colors
            else if (g_Globals.Visuals.RainbowLines)
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

            // La ligne s'attache toujours au SOMMET DE LA BOX (au dessus de la
            // boite) — jamais au dessus du nom : le nom reste libre au dessus.
            ImVec2 attachPoint(newX + newW * 0.5f, newY);

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
                if (!gunName.empty()) {   // arme inconnue : on n'affiche RIEN (pas de "ID: xxx")
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
        }


        if (g_Globals.Visuals.Watermark) 
        {
            auto RenderMainTitle = [](ImDrawList* drawList, float yPos) 
                {
                ImGui::PushFont(FWork::Fonts::InterSemiBold);
                const char* text = "0x Void - Core";
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

        if (g_Globals.Visuals.Name || g_Globals.Visuals.Distance || g_Globals.Visuals.RankIcon || g_Globals.Visuals.moco)
        {
            ImDrawList* drawList = ImGui::GetForegroundDrawList();

            // ===== COLOR CONFIGURATION =====
            auto CreateColor = [](const float color[4]) -> ImColor {
                return ImColor(color[0], color[1], color[2], color[3]);
                };

            ImColor nameColor = CreateColor(g_Globals.Visuals.NameColor);
            ImColor distColor = CreateColor(g_Globals.Visuals.DistColor);
            const ImColor outlineColor = ImColor(0, 0, 0, 180);
            const float outlineSize = 1.0f;

            // RGB Effect
            if (g_Globals.Visuals.RGB) {
                float time = ImGui::GetTime();
                nameColor = ImColor::HSV(fmod(time * 2.0f, 1.0f), 1.0f, 1.0f);
                distColor = ImColor::HSV(fmod(time * 2.0f + 0.33f, 1.0f), 1.0f, 1.0f);
            }

            // ===== FONT CONFIGURATION =====
            ImFont* nameFont = FWork::Fonts::InterRegular;
            const float nameFontSize = nameFont->FontSize * 1.00f;   // ESP Name agrandi (etait 0.80)
            const float distFontSize = FWork::Fonts::InterBold->FontSize * 0.70f;
            // Ensure player has a name
            if (player.Name.empty()) {
                player.Name = "BOT";
            }

            // Noms arabes : formes contextuelles + sens RTL (ImGui n'a ni bidi
            // ni shaping — sans ca un nom arabe s'affiche lettre par lettre a l'envers)
            const std::string nameText = ArabicShape::Process(player.Name, nameFont);

            // ===== TEXT POSITIONING =====
            //  LE NOM EST COLLE AU-DESSUS DE LA BOITE ESP (haut de boite = headPos.y).
            //  Plus de decalage fixe de 50px : le bloc se pose juste au dessus.
            //  Ordre (du haut vers le bas) :  [ID]  [distance]  [NOM]  ->  BOITE
            ImVec2 textSize = nameFont->CalcTextSizeA(nameFontSize, FLT_MAX, 0.0f, nameText.c_str());

            const bool showDistLine = g_Globals.Visuals.Distance;
            const std::string distText = showDistLine
                ? std::to_string(static_cast<int>(std::round(dist))) + "m"
                : std::string();
            const ImVec2 distSize = showDistLine
                ? FWork::Fonts::InterBold->CalcTextSizeA(distFontSize, FLT_MAX, 0.0f, distText.c_str())
                : ImVec2(0.0f, 0.0f);

            const float gapToBox = 3.0f;                       // ecart nom <-> boite
            const float distBlock = showDistLine ? (distSize.y + 1.0f) : 0.0f;

            // bas du nom = juste au dessus de la boite
            ImVec2 textPos(headPos.x - (textSize.x / 2.0f),
                           headPos.y - gapToBox - distBlock - textSize.y);

            // distance AU DESSUS du nom -> elle ne recouvre jamais la boite
            const ImVec2 distPos(showDistLine ? headPos.x - (distSize.x / 2.0f) : 0.0f,
                                 showDistLine ? textPos.y - 1.0f - distSize.y   : 0.0f);

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

            // ===== VISUAL ELEMENTS =====

            // Player Name
            if (g_Globals.Visuals.Name) {
                DrawTextWithOutline(nameFont, nameFontSize, textPos, nameColor, nameText.c_str());
            }

            // Player Rank Icon — badge PNG embarque DANS la DLL (aucun fichier externe)
            if (g_Globals.Visuals.RankIcon && player.RankScore > 0) {
                if (ImTextureID iconTex = RankIcons::Get(RankEsp::IconIndexFromScore(player.RankScore))) {
                    const float iconSize = nameFontSize * 1.45f;
                    const ImVec2 iconMin(textPos.x + textSize.x + 6.0f,
                                         textPos.y + (textSize.y - iconSize) * 0.5f);
                    drawList->AddImage(iconTex, iconMin, ImVec2(iconMin.x + iconSize, iconMin.y + iconSize));
                }
            }

            // Distance Text — AU DESSUS du nom (le nom reste colle a la boite)
            if (showDistLine) {
                DrawTextWithOutline(FWork::Fonts::InterBold, distFontSize, distPos, distColor, distText.c_str());
            }

            // MOCO Triangle Indicator — sous la boite (jamais sur le texte)
            if (g_Globals.Visuals.moco) {
                const float triangleHeight = 6.0f;
                const float triangleBase = 10.0f;
                ImVec2 triangleCenter(headPos.x, headPos.y + boxHeight + 4.0f);

                ImVec2 trianglePoints[] = {
                    ImVec2(triangleCenter.x - triangleBase / 2.0f, triangleCenter.y),
                    ImVec2(triangleCenter.x + triangleBase / 2.0f, triangleCenter.y),
                    ImVec2(triangleCenter.x, triangleCenter.y + triangleHeight)
                };

                drawList->AddTriangleFilled(trianglePoints[0], trianglePoints[1], trianglePoints[2], IM_COL32(15, 252, 3, 255));
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

    // Silent Aim V2 : cercle de FOV dedie (pearl white)
    if (g_Globals.EspConfig.InMatch && g_Globals.AimBot.SilentAimV2 &&
        g_Globals.AimBot.SilentAimV2ShowFov && g_Globals.AimBot.SilentAimV2Fov > 0) {
        ImVec2 center2(g_Globals.EspConfig.Width / 2.0f, g_Globals.EspConfig.Height / 2.0f);
        ImGui::GetBackgroundDrawList()->AddCircle(center2, (float)g_Globals.AimBot.SilentAimV2Fov,
            IM_COL32(243, 244, 246, 220), 100, 1.5f);
    }

    // Telekill Lag : halqa f blasset l'activation (EntitiesLock deja tenu)
    DrawTelekillRing();

    GameTimer::Draw();
}

// ------------------------------------------------------------------
// COMPTEUR D'ENNEMIS (haut centre) — porte depuis BR MODE BY DEATH ENGINE
// MOSTATILA 56x24 : icone joueur + nombre d'ennemis cote a cote,
// padding aux deux cotes, sans halo et sans barre laterale.
// ------------------------------------------------------------------
void ESP::PlayerCounter()
{
    // 100% AUTO : pas de case a cocher, la carte s'affiche toute seule
    // des que le menu est ouvert (CurrentTab >= 2) et que le jeu est charge.

    // Data::Work tourne dans son propre thread -> lecture sous lock.
    int enemyCount = 0;
    {
        SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
        for (const auto& pair : g_Globals.EspConfig.Entities) {
            const Player& player = pair.second;
            if (!player.IsDead && player.Distance <= g_Globals.Visuals.DistanceEsp)
                enemyCount++;
        }
    }

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    // --- DESIGN : MOSTATILA plus large + plus allongee, avec de la place
    //     aux cotes du contenu (padding), pas de halo, pas de barre laterale ---
    const float w = 56.0f;                  // largeur (46 -> 56, plus oblong)
    const float h = 24.0f;                  // hauteur (22 -> 24)
    const float rad = 7.0f;
    const float x = (io.DisplaySize.x - w) * 0.5f;
    const float y = 10.0f;
    const ImVec2 rMin(x, y);
    const ImVec2 rMax(x + w, y + h);

    drawList->AddRectFilled(rMin, rMax, IM_COL32(9, 10, 14, 245), rad);
    drawList->AddRect(rMin, rMax, IM_COL32(243, 244, 246, 95), rad, 0, 1.0f);

    // --- contenu : icone joueur  +  nombre d'ennemis, cote a cote ---
    char countText[16];
    snprintf(countText, sizeof(countText), "%d", enemyCount);

    ImFont* counterFont = FWork::Fonts::InterBold
                        ? FWork::Fonts::InterBold
                        : (FWork::Fonts::InterRegular ? FWork::Fonts::InterRegular : ImGui::GetFont());
    const float counterFontSize = 14.0f;
    const ImVec2 countSize = counterFont->CalcTextSizeA(counterFontSize, 1000.0f, 0.0f, countText);

    ImFont* playerIconFont = FWork::Fonts::FontAwesomeSolid;
    const float iconSize = 12.0f;
    const ImVec2 isz = playerIconFont
                     ? playerIconFont->CalcTextSizeA(iconSize, 1000.0f, 0.0f, ICON_FA_USER)
                     : ImVec2(0, 0);

    const float gap = 7.0f;
    const float pad = 8.0f;                 // place min. gardee des deux cotes
    float contentW = isz.x + gap + countSize.x;
    float startX;
    if (contentW + pad * 2.0f <= w) {
        startX = x + (w - contentW) * 0.5f;             // centre si ca tient
    } else {
        startX = x + pad;                                // sinon on respecte le padding
    }

    if (playerIconFont) {
        drawList->AddText(playerIconFont, iconSize,
                          ImVec2(startX, y + (h - isz.y) * 0.5f),
                          IM_COL32(243, 244, 246, 225), ICON_FA_USER);
    }

    drawList->AddText(counterFont, counterFontSize,
                      ImVec2(startX + isz.x + gap, y + (h - countSize.y) * 0.5f),
                      IM_COL32(243, 244, 246, 255), countText);
}
