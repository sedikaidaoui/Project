#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <cmath>
#include "imgui.h"
#include "imgui_internal.h"

namespace font { extern ImFont* lexend_bold; }
namespace image { extern ImTextureID logo_texture; }

struct Notification {
    int id;
    std::string message;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;

    // أنيميشن ناعمة للدخول والخروج
    float currentX = -300.0f;
    float velocityX = 0.0f;
    float currentAlpha = 0.0f;
    float scanline_offset = 0.0f;
};

class NotificationSystem {
public:
    NotificationSystem() : notificationIdCounter(0) {}

    void AddNotification(const std::string& message, int durationMs) {
        auto now = std::chrono::steady_clock::now();
        Notification noti;
        noti.id = notificationIdCounter++;
        noti.message = message;
        noti.startTime = now;
        noti.endTime = now + std::chrono::milliseconds(durationMs);
        noti.currentX = -300.0f;
        noti.velocityX = 0.0f;
        noti.currentAlpha = 0.0f;
        noti.scanline_offset = 0.0f;

        notifications.push_back(noti);
    }

    void DrawNotifications() {
        if (notifications.empty()) return;
        auto now = std::chrono::steady_clock::now();

        notifications.erase(std::remove_if(notifications.begin(), notifications.end(),
            [now](const Notification& n) { return now >= n.endTime && n.currentAlpha <= 0.01f; }), notifications.end());

        for (int i = 0; i < (int)notifications.size(); i++) {
            float duration = (float)std::chrono::duration_cast<std::chrono::milliseconds>(notifications[i].endTime - notifications[i].startTime).count();
            float elapsed = (float)std::chrono::duration_cast<std::chrono::milliseconds>(now - notifications[i].startTime).count();
            float percentage = (duration > 0) ? ((duration - elapsed) / duration) * 100.0f : 0.0f;


            float display_pct = (now >= notifications[i].endTime) ? 0.0f : percentage;

            ShowNotification(i, notifications[i], display_pct, (now >= notifications[i].endTime));
        }
    }

private:
    std::vector<Notification> notifications;
    int notificationIdCounter;

    void ShowNotification(int position, Notification& noti, float percentage, bool time_expired) {
        ImGuiIO& io = ImGui::GetIO();
        float deltaTime = io.DeltaTime;
        float time_seconds = (float)ImGui::GetTime();


        float targetX = 15.0f;
        float targetAlpha = 1.0f;

        if (time_expired) {
            targetX = -320.0f;
            targetAlpha = 0.0f;
        }

 
        noti.currentX += (targetX - noti.currentX) * 10.0f * deltaTime;
        noti.currentAlpha += (targetAlpha - noti.currentAlpha) * 12.0f * deltaTime;
        noti.currentAlpha = ImClamp(noti.currentAlpha, 0.0f, 1.0f);

        std::string title = noti.message, status = "";
        size_t delim = noti.message.find(':');
        if (delim != std::string::npos) {
            title = noti.message.substr(0, delim);
            status = noti.message.substr(delim + 1);
        }

        float titleWidth = 0.0f;
        if (font::lexend_bold) {
            ImGui::PushFont(font::lexend_bold);
            titleWidth = ImGui::CalcTextSize(title.c_str()).x;
            ImGui::PopFont();
        }
        else {
            titleWidth = ImGui::CalcTextSize(title.c_str()).x;
        }
        float statusWidth = ImGui::CalcTextSize(status.c_str()).x * 0.85f;
        float maxTextWidth = (titleWidth > statusWidth) ? titleWidth : statusWidth;

        float w = 62.0f + maxTextWidth + 25.0f;
        if (w < 200.0f) w = 200.0f;

        float h = 44.0f;
        float x = noti.currentX;
        float y = 20.0f + (position * (h + 10.0f));

        ImGui::SetNextWindowPos(ImVec2(x, y));
        ImGui::SetNextWindowSize(ImVec2(w, h));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

        if (ImGui::Begin(("##NOTI_" + std::to_string(noti.id)).c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs))
        {
            ImDrawList* draw = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetWindowPos();
            ImVec2 s = ImGui::GetWindowSize();


            bool isEnabled = (noti.message.find("Welcom") != std::string::npos || noti.message.find("Success") != std::string::npos);
            bool isError = (noti.message.find("ERROR") != std::string::npos || noti.message.find("Failed") != std::string::npos);

            ImColor themeColor = ImColor(255, 255, 255, (int)(255 * noti.currentAlpha));  // blanc (plus de bleu)
            if (isEnabled) themeColor = ImColor(0, 240, 140, (int)(255 * noti.currentAlpha));
            if (isError)   themeColor = ImColor(255, 35, 80, (int)(255 * noti.currentAlpha));

   
            draw->AddRectFilled(p, ImVec2(p.x + s.x, p.y + s.y), ImColor(10, 12, 18, (int)(225 * noti.currentAlpha)), 4.0f);

            // 2. الـ Scanlines الهولوغرافية المتحركة
            noti.scanline_offset += deltaTime * 20.0f;
            if (noti.scanline_offset > 6.0f) noti.scanline_offset = 0.0f;

            for (float scr_y = noti.scanline_offset; scr_y < s.y; scr_y += 5.0f) {
                draw->AddLine(ImVec2(p.x + 4.0f, p.y + scr_y), ImVec2(p.x + s.x - 2.0f, p.y + scr_y),
                    ImColor(255, 255, 255, (int)(5 * noti.currentAlpha)), 1.0f);
            }

        
            float block_w = 3.5f;
            draw->AddRectFilled(p, ImVec2(p.x + block_w, p.y + s.y), themeColor, 4.0f, ImDrawFlags_RoundCornersLeft);

            float pulse_y = fmodf(time_seconds * 70.0f, s.y + 15.0f) - 7.0f;
            draw->AddRectFilled(ImVec2(p.x, p.y + ImMax(0.0f, pulse_y)),
                ImVec2(p.x + block_w, p.y + ImMin(s.y, pulse_y + 10.0f)),
                ImColor(255, 255, 255, (int)(160 * noti.currentAlpha)));

  
            draw->AddRect(p, ImVec2(p.x + s.x, p.y + s.y), ImColor(themeColor.Value.x, themeColor.Value.y, themeColor.Value.z, noti.currentAlpha * 0.12f), 4.0f, ImDrawFlags_RoundCornersAll, 1.0f);


            float icon_size = 22.0f;
            float icon_x = p.x + block_w + 10.0f;
            float icon_y = p.y + (s.y - icon_size) / 2.0f;

            if (image::logo_texture != 0) {
                draw->AddImage(image::logo_texture, ImVec2(icon_x, icon_y), ImVec2(icon_x + icon_size, icon_y + icon_size), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, (int)(255 * noti.currentAlpha)));
            }

      
            float text_x_pos = (image::logo_texture != 0) ? (block_w + 40.0f) : (block_w + 14.0f);

            if (font::lexend_bold) ImGui::PushFont(font::lexend_bold);
            draw->AddText(ImVec2(p.x + text_x_pos, p.y + 6), ImColor(255, 255, 255, (int)(255 * noti.currentAlpha)), title.c_str());
            if (font::lexend_bold) ImGui::PopFont();

            draw->AddText(ImVec2(p.x + text_x_pos, p.y + 22), ImColor(135, 140, 150, (int)(190 * noti.currentAlpha)), status.c_str());

            float bar_w = 35.0f;
            float bar_h = 1.5f;
            float bar_x = p.x + s.x - bar_w - 10.0f;
            float bar_y = p.y + (s.y / 2.0f);

            ImVec2 b_start = ImVec2(bar_x, bar_y);
            ImVec2 b_end = ImVec2(bar_x + (bar_w * (percentage / 100.0f)), bar_y);

            if (percentage > 0.0f) {
                draw->AddLine(b_start, ImVec2(bar_x + bar_w, bar_y), ImColor(255, 255, 255, (int)(15 * noti.currentAlpha)), bar_h);
                draw->AddLine(b_start, b_end, themeColor, bar_h);
                draw->AddCircleFilled(b_end, 1.8f, ImColor(themeColor.Value.x, themeColor.Value.y, themeColor.Value.z, noti.currentAlpha * 0.5f), 8);
            }
        }
        ImGui::End();
        ImGui::PopStyleColor(1);
        ImGui::PopStyleVar(3);
    }
};

inline NotificationSystem g_Notify;
#endif
