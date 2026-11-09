#include "GameTimer.hpp"
#include <src/Globals.hpp>
#include <Windows.h>
#include <imgui.h>
#include <cstdio>

namespace {
    constexpr int kDurationMs = 180000;

    bool s_countdownActive = false;
    bool s_wasInMatch = false;
    ULONGLONG s_startTick = 0;

    const char* UpdateCountdownText() {
        const bool inMatch = g_Globals.EspConfig.InMatch;

        if (!inMatch) {
            if (s_wasInMatch) {
                s_countdownActive = false;
                s_startTick = 0;
            }
            s_wasInMatch = false;
            return nullptr;
        }

        s_wasInMatch = true;

        if (!s_countdownActive) {
            s_countdownActive = true;
            s_startTick = GetTickCount64();
            return "New Match Started";
        }

        const long long elapsedMs = static_cast<long long>(GetTickCount64() - s_startTick);
        const long long remainingMs = static_cast<long long>(kDurationMs) - elapsedMs;

        if (remainingMs <= 0) {
            return "You Can Finish the Game Now";
        }

        static char timeBuffer[16];
        const int seconds = static_cast<int>(remainingMs / 1000);
        const int minutes = seconds / 60;
        const int secs = seconds % 60;
        snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d", minutes, secs);
        return timeBuffer;
    }
}

void GameTimer::Draw() {
    const char* timerText = UpdateCountdownText();
    if (!timerText || !timerText[0]) {
        return;
    }

    if (!g_Globals.Visuals.ShowGameTimer) {
        return;
    }

    if (g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0) {
        return;
    }

    char displayText[96];
    snprintf(displayText, sizeof(displayText), "Game Timer - %s", timerText);

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const ImVec2 textSize = ImGui::CalcTextSize(displayText);

    const float windowWidth = static_cast<float>(g_Globals.EspConfig.Width);
    const float textPosY = 80.0f;
    const float totalPlayersTextPosX = (windowWidth - textSize.x) * 0.5f;
    const float totalPlayersTextPosY = textPosY + textSize.y + 20.0f;

    constexpr float padding = 10.0f;
    const ImVec2 bgRectMin(totalPlayersTextPosX - padding, totalPlayersTextPosY - padding);
    const ImVec2 bgRectMax(totalPlayersTextPosX + textSize.x + padding, totalPlayersTextPosY + textSize.y + padding);

    const ImU32 bgColor = IM_COL32(0, 0, 0, 128);
    const ImU32 borderColor = IM_COL32(255, 255, 255, 204);
    const ImU32 textColor = IM_COL32(255, 255, 255, 255);

    drawList->AddRectFilled(bgRectMin, bgRectMax, bgColor, 5.0f);
    drawList->AddRect(bgRectMin, bgRectMax, borderColor, 5.0f);
    drawList->AddText(ImVec2(totalPlayersTextPosX, totalPlayersTextPosY), textColor, displayText);
}
