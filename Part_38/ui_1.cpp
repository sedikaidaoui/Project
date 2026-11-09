#include "ui.hpp"
#include <windows.h>
#include <shellapi.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <src/Overlay/Overlay.hpp>
#include <src/Globals.hpp>
#include <src/Fonts/FontInter.hpp>
#include <imgui_internal.h>
#include <src/Fonts/FontAwesome.hpp>
#include <src/Fonts/Fonts.hpp>
#include <thread>
#include <map>
#include <Auth/auth.hpp>
#include "src\Fonts\Fonts.hpp"
#include <FontAwesome6.hpp>
#include "Logo.hpp"
#include <lmcons.h>
#include <EspLines/Memory/NashMem.hpp>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <src/adb/adb_utils.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Features/FakeLag/FakeLag.hpp>
#include <EspLines/Data/MovementExploits.hpp>
#include <EspLines/Data/Data.hpp>
#include <vector>
#include <vector>
#include <string>
#include <atomic>
#include <algorithm>
#include <cctype>
#include <fstream>

struct ThreadGuard {
    ThreadGuard() { g_Globals.General.ActiveThreads++; }
    ~ThreadGuard() { g_Globals.General.ActiveThreads--; }
};

ImFont* InterBlack = nullptr;
ImFont* InterBold = nullptr;
ImFont* InterBold12 = nullptr;
ImFont* InterExtraBold = nullptr;
ImFont* InterExtraLight = nullptr;
ImFont* InterLight = nullptr;
ImFont* InterMedium = nullptr;
ImFont* InterRegular = nullptr;
ImFont* InterSemiBold = nullptr;
ImFont* InterThin = nullptr;

ImFont* FontAwesomeRegular = nullptr;
ImFont* FontAwesomeSolid = nullptr;
ImFont* FontAwesomeSolid14 = nullptr;
ImFont* FontAwesomeBrands = nullptr;

static bool isAuthenticated = false;

// ---------------------------------------------------------------------------
// License / session expiry handling
// ---------------------------------------------------------------------------
namespace HubAuth {
    static const char* kDiscordInvite = "https://discord.gg/6BNChkFvP";
    static const char* kExpiredMessage = "SESSION EXPIRED - renew on Discord.";

    inline void OpenDiscordInvite() {
        ShellExecuteA(nullptr, "open", kDiscordInvite, nullptr, nullptr, SW_SHOWNORMAL);
    }

    inline bool LooksExpired(const std::string& message) {
        std::string m = message;
        std::transform(m.begin(), m.end(), m.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return m.find("expir") != std::string::npos ||
               m.find("subscription") != std::string::npos;
    }

    // Local copy of the last known license expiry, so an expired user is
    // detected at DLL load (before anything else) even without a login.
    inline std::string LicenseFilePath() {
        char buf[MAX_PATH]{};
        DWORD n = GetEnvironmentVariableA("LOCALAPPDATA", buf, MAX_PATH);
        std::string dir = (n > 0 && n < MAX_PATH) ? std::string(buf) : std::string(".");
        dir += "\\HUB Community";
        CreateDirectoryA(dir.c_str(), nullptr);
        return dir + "\\license.dat";
    }

    inline void SaveExpiry(long long timestamp, const std::string& username) {
        std::ofstream f(LicenseFilePath(), std::ios::trunc);
        if (!f.is_open()) return;
        f << timestamp << "\n" << username << "\n";
    }

    inline long long LoadExpiry() {
        std::ifstream f(LicenseFilePath());
        long long ts = 0;
        if (f.is_open()) f >> ts;
        return ts;
    }

    inline long long ParseExpiry(const std::string& raw) {
        try {
            return raw.empty() ? 0 : static_cast<long long>(std::stoll(raw));
        }
        catch (...) {
            return 0;
        }
    }

    inline bool HasExpired(long long timestamp) {
        return timestamp > 0 && static_cast<long long>(std::time(nullptr)) >= timestamp;
    }

    // Native English error popup, then the Discord invite opens automatically.
    inline void NotifySessionExpiredAsync() {
        std::thread([] {
            MessageBoxA(nullptr,
                "SESSION EXPIRED\n\n"
                "Your session / subscription has ended.\n"
                "This license can no longer be used to run HUB COMMUNITY.\n\n"
                "Press OK to open our Discord server and renew your access.",
                "HUB COMMUNITY - Session Expired",
                MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
            OpenDiscordInvite();
        }).detach();
    }
}

static std::atomic<bool> g_SessionExpiredPopupPending{ false };
static std::atomic<bool> g_SessionExpiredTriggered{ false };
static std::atomic<bool> g_SessionBlocked{ false };
static std::atomic<bool> g_StartupLicenseExpired{ false };
static std::atomic<long long> g_SessionExpiryTime{ 0 };


namespace ImGui {
    inline bool ButtonStyled(const char* label, const ImVec2& size_arg, bool disabled = false)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        const ImVec2 label_size = CalcTextSize(label, NULL, true);

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size;
        if (size_arg.x == 0 && size_arg.y == 0)
            size = ImVec2(ImGui::GetWindowSize().x - 24, 30);
        else
            size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

        const ImRect bb(pos, pos + size);
        ItemSize(size, style.FramePadding.y);
        if (!ItemAdd(bb, id))
            return false;

        bool hovered = false, held = false;
        bool pressed = false;
        if (!disabled)
            pressed = ButtonBehavior(bb, id, &hovered, &held, 0);

        static std::unordered_map<ImGuiID, float> anim;
        auto it = anim.find(id);
        if (it == anim.end()) {
            anim.insert({ id, 0.0f });
            it = anim.find(id);
        }

        // Smooth frame-rate independent hover transition
        float target = (hovered && !disabled) ? 1.0f : 0.0f;
        it->second = ImLerp(it->second, target, 1.0f - expf(-16.0f * g.IO.DeltaTime));

        float alpha = style.Alpha;
        ImVec4 baseBg = disabled ? ImVec4(0.08f, 0.09f, 0.12f, 0.5f * alpha) : ImVec4(0.11f, 0.12f, 0.17f, alpha);
        ImVec4 hoverBg = ImVec4(0.94f, 0.26f, 0.26f, alpha); // Crimson Red
        ImVec4 activeBg = ImVec4(0.86f, 0.15f, 0.15f, alpha);

        ImVec4 bgCol = disabled ? baseBg : (held ? activeBg : ImLerp(baseBg, hoverBg, it->second * 0.75f));
        ImVec4 borderCol = disabled ? ImVec4(0.18f, 0.19f, 0.25f, 0.4f * alpha) : ImLerp(ImVec4(0.22f, 0.24f, 0.34f, 0.8f * alpha), ImVec4(0.97f, 0.45f, 1.00f, alpha), it->second);

        window->DrawList->AddRectFilled(bb.Min, bb.Max, ImColor(bgCol), 6.0f);
        window->DrawList->AddRect(bb.Min, bb.Max, ImColor(borderCol), 6.0f, 0, 1.0f);

        ImVec4 textCol = disabled ? ImVec4(0.45f, 0.45f, 0.50f, alpha) : (hovered ? ImVec4(1.0f, 1.0f, 1.0f, alpha) : ImVec4(0.92f, 0.93f, 0.96f, alpha));
        if (InterBold) PushFont(InterBold);
        RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, ImVec2(0.5f, 0.5f), &bb);
        if (InterBold) PopFont();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
        return pressed;
    }
}

static inline ImVec2 Size = ImVec2(640, 440);

int CurrentTab = 10;

char Username[255] = "";
char Password[255] = "";
char Gmail[255] = "";

//Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;

HWND hwnd = nullptr;

ID3D11ShaderResourceView* Logo = nullptr;

std::string GetUsername()
{
    char username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (GetUserNameA(username, &username_len))
        return std::string(username);
    return "Unknown";
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace FWork {
    void Interface::Initialize(HWND Window, HWND TargetWindow, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext) {
        if (!Window || !Device || !DeviceContext) return;
        hWindow = Window;
        IDevice = Device;
        g_pd3dDevice = Device;
        g_pd3dDeviceContext = DeviceContext;

        ImGui::CreateContext();
        if (!ImGui_ImplWin32_Init(hWindow)) return;
        if (!ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext)) return;

        ImGuiIO& io = ImGui::GetIO();
        
        // Font configuration for merging
        ImFontConfig font_cfg;
        font_cfg.FontDataOwnedByAtlas = false;
        
        ImFontConfig icon_cfg;
        icon_cfg.MergeMode = true;
        icon_cfg.PixelSnapH = true;
        icon_cfg.GlyphMinAdvanceX = 14.0f;
        static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

        // Load Inter Regular + FontAwesome
        InterRegular = io.Fonts->AddFontFromMemoryCompressedTTF(InterRegular_compressed_data, InterRegular_compressed_size, 16);
        io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesome6Solid_compressed_data, FontAwesome6Solid_compressed_size, 14.0f, &icon_cfg, icon_ranges);

        // Load Inter Bold + FontAwesome
        InterBold = io.Fonts->AddFontFromMemoryCompressedTTF(InterBold_compressed_data, InterBold_compressed_size, 16);
        io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesome6Solid_compressed_data, FontAwesome6Solid_compressed_size, 14.0f, &icon_cfg, icon_ranges);
        
        InterBold12 = io.Fonts->AddFontFromMemoryCompressedTTF(InterBold_compressed_data, InterBold_compressed_size, 12);
        InterMedium = io.Fonts->AddFontFromMemoryCompressedTTF(InterMedium_compressed_data, InterMedium_compressed_size, 16);
        InterExtraBold = io.Fonts->AddFontFromMemoryCompressedTTF(InterBold_compressed_data, InterBold_compressed_size, 24);
        
        // Load standalone FontAwesome for specific uses if needed
        FontAwesomeSolid = io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesome6Solid_compressed_data, FontAwesome6Solid_compressed_size, 17.f, &font_cfg, icon_ranges);

        Fonts::Initialize(IDevice);
        Logo = nullptr;
        if (g_pd3dDevice) {
            HRESULT hr = D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, LogoBytes, sizeof(LogoBytes), NULL, NULL, &Logo, NULL);
            if (FAILED(hr)) {
                Logo = nullptr;
            }
        }

        // Tenta iniciar a API assim que carrega
        //CShellApp.init();
        InitializeMenu();

        extern void BackgroundInit();
        extern std::thread g_InitThread;
        if (!g_InitThread.joinable()) {
            g_InitThread = std::thread(BackgroundInit);
        }
    }

    void Interface::InitializeMenu() {
        bIsMenuOpen = true;
        SetWindowLong(hWindow, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT);
        SetForegroundWindow(hWindow);
        SetWindowPos(hWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
    }

    void Interface::UpdateStyle() {
        IMGUI_CHECKVERSION();
        ImGuiStyle* Style = &ImGui::GetStyle();
        
        Style->WindowPadding = ImVec2(0, 0);
        Style->FramePadding = ImVec2(10, 5);
        Style->ItemSpacing = ImVec2(8, 7);
        Style->WindowRounding = 12.0f;
        Style->FrameRounding = 6.0f;
        Style->PopupRounding = 8.0f;
        Style->ChildRounding = 8.0f;
        Style->GrabRounding = 6.0f;
        Style->WindowBorderSize = 1.0f;
        Style->FrameBorderSize = 1.0f;
        Style->ChildBorderSize = 1.0f;
        Style->ScrollbarSize = 5.0f;
        Style->ScrollbarRounding = 8.0f;

        const ImVec4 accent = ImVec4(0.937f, 0.267f, 0.267f, 1.0f);       // #EF4444 Crimson Red
        const ImVec4 accentHover = ImVec4(0.973f, 0.443f, 0.443f, 1.0f);  // #F87171 Bright Rose
        const ImVec4 accentActive = ImVec4(0.863f, 0.149f, 0.149f, 1.0f); // #DC2626 Deep Red

        Style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.f, 0.f, 0.f, 0.f);
        Style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(accent.x, accent.y, accent.z, 0.50f);
        Style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(accent.x, accent.y, accent.z, 0.85f);
        Style->Colors[ImGuiCol_ScrollbarGrabActive] = accent;

        // --- Royal Crimson & Obsidian Glass Aesthetic Theme ---
        Style->Colors[ImGuiCol_WindowBg] = ImVec4(0.043f, 0.047f, 0.063f, 0.98f);     // #0B0C10 Obsidian base
        Style->Colors[ImGuiCol_ChildBg] = ImVec4(0.063f, 0.067f, 0.094f, 0.96f);      // #101118 Deep card acrylic
        Style->Colors[ImGuiCol_PopupBg] = ImVec4(0.071f, 0.075f, 0.110f, 0.98f);      // #12131C Frosted popup
        Style->Colors[ImGuiCol_Border] = ImVec4(0.133f, 0.141f, 0.212f, 0.85f);       // #222436 Subtle slate hairline
        Style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        Style->Colors[ImGuiCol_Text] = ImVec4(0.953f, 0.957f, 0.965f, 1.00f);         // #F3F4F6 Crisp pearlescent white
        Style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.420f, 0.447f, 0.502f, 1.00f); // #6B7280 Muted silver
        Style->Colors[ImGuiCol_FrameBg] = ImVec4(0.082f, 0.086f, 0.133f, 1.00f);      // #151622 Input frame
        Style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.110f, 0.118f, 0.180f, 1.00f);
        Style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.145f, 0.153f, 0.235f, 1.00f);
        Style->Colors[ImGuiCol_Button] = ImVec4(0.086f, 0.090f, 0.141f, 1.00f);
        Style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.137f, 0.145f, 0.227f, 1.00f);
        Style->Colors[ImGuiCol_ButtonActive] = accent;
        Style->Colors[ImGuiCol_CheckMark] = accent;
        Style->Colors[ImGuiCol_SliderGrab] = accent;
        Style->Colors[ImGuiCol_SliderGrabActive] = accentHover;
        Style->Colors[ImGuiCol_Header] = ImVec4(0.102f, 0.106f, 0.165f, 1.00f);
        Style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.145f, 0.153f, 0.235f, 1.00f);
        Style->Colors[ImGuiCol_HeaderActive] = accent;
        Style->Colors[ImGuiCol_Separator] = ImVec4(0.133f, 0.141f, 0.212f, 0.85f);
    }

    void LoadingSpinner(const char* label, float radius, float thickness, const ImU32& color) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size((radius) * 2, (radius) * 2);

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, id)) return;

        // Render
        window->DrawList->PathClear();

        int num_segments = 30;
        int start = (int)abs(ImGui::GetTime() * 1.8f * num_segments) % num_segments;

        const float a_min = IM_PI * 2.0f * ((float)start) / (float)num_segments;
        const float a_max = IM_PI * 2.0f * ((float)start + num_segments * 0.75f) / (float)num_segments;

        const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius);

        for (int i = 0; i < num_segments; i++) {
            const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
            window->DrawList->PathLineTo(ImVec2(centre.x + cosf(a) * radius, centre.y + sinf(a) * radius));
        }

        window->DrawList->PathStroke(color, false, thickness);
    }

    void Interface::RenderGui() {

        // --- License / session expiry ---------------------------------
        // When the subscription is over: native Windows error + Discord
        // invite, and the panel itself must never be shown/injected.
        static bool startupLicenseChecked = false;
        if (!startupLicenseChecked) {
            startupLicenseChecked = true;
            // Fires the moment the DLL is loaded/injected, before the login
            // window is even usable, if the last known expiry already passed.
            if (HubAuth::HasExpired(HubAuth::LoadExpiry())) {
                g_StartupLicenseExpired.store(true);
                g_SessionExpiredPopupPending.store(true);
            }
        }

        if (g_SessionExpiredPopupPending.exchange(false)) {
            HubAuth::NotifySessionExpiredAsync();
        }

        if (!g_SessionBlocked.load() && isAuthenticated) {
            if (g_SessionExpiryTime.load() <= 0 && !g_Api.client.sub_type.expire_date.empty()) {
                try {
                    g_SessionExpiryTime.store(static_cast<long long>(std::stoll(g_Api.client.sub_type.expire_date)));
                }
                catch (...) {
                    g_SessionExpiryTime.store(0);
                }
            }

            const long long expiresAt = g_SessionExpiryTime.load();
            if (expiresAt > 0 && static_cast<long long>(std::time(nullptr)) >= expiresAt) {
                g_SessionBlocked.store(true);
                g_SessionExpiredTriggered.store(true);
                g_SessionExpiredPopupPending.store(true);
            }
        }

        if (g_SessionBlocked.load()) {
            // Kill every feature so nothing keeps running with an expired license.
            g_Globals.AimBot.Enabled = false;
            g_Globals.Visuals.Enable = false;
            isAuthenticated = false;
            if (bIsMenuOpen) {
                bIsMenuOpen = false;
                bIsMainMinimized = false;
                UpdateClickThrough();
            }
            return;
        }

        if (bIsMenuOpen) {

            // Authentication variables
            static char username[64] = "";
            static char password[64] = "";
            static bool loginInProgress = false;
            static bool loginSuccess = false;
            static bool auth_error = false;
            static std::string error_msg = "";
            static std::string KeyAuth_message = "";

            // Local expiry already passed before this session: show the notice
            // in the login window as well (the popup already fired at load).
            static bool startupNoticeApplied = false;
            if (!startupNoticeApplied && g_StartupLicenseExpired.load() && !isAuthenticated) {
                startupNoticeApplied = true;
                auth_error = true;
                error_msg = "Session expired - renew on Discord.";
            }

            // If not authenticated, show login page
            if (!isAuthenticated) {
                // Login only (registration removed)

                // Match menu size dynamically
                float winHeight = 310.0f;
                ImGui::SetNextWindowSize(ImVec2(360, winHeight));
                ImGui::Begin("LoginWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                {
                    ImDrawList* DrawList = ImGui::GetWindowDrawList();
                    ImVec2 Pos = ImGui::GetWindowPos();
                    ImVec2 Size = ImGui::GetWindowSize();

                    // Main body background - Obsidian Glass
                    DrawList->AddRectFilled(Pos, Pos + Size, ImColor(11, 12, 16, 255), 12.0f);
                    DrawList->AddRect(Pos, Pos + Size, ImColor(38, 40, 56, 220), 12.0f, 0, 1.0f);
                    
                    // Top branding header bar
                    float headerHeight = 38.0f;
                    DrawList->AddRectFilled(Pos, Pos + ImVec2(Size.x, headerHeight), ImColor(14, 15, 22, 255), 12.0f, ImDrawFlags_RoundCornersTop);
                    DrawList->AddLine(Pos + ImVec2(0, headerHeight), Pos + ImVec2(Size.x, headerHeight), ImColor(38, 40, 56, 255));

                    // HUB badge logo + Title
                    if (Logo) {
                        DrawList->AddImage((ImTextureID)Logo, Pos + ImVec2(8, 6), Pos + ImVec2(32, 30));
                    } else {
                        DrawList->AddCircleFilled(Pos + ImVec2(20, 19), 4.0f, ImColor(239, 68, 68, 255), 12);
                    }
                    ImGui::SetCursorPos(ImVec2(36, 3));
                    ImGui::PushFont(InterBold);
                    ImGui::Text("HUB COMMUNITY");
                    ImGui::PopFont();

                    ImGui::SetCursorPos(ImVec2(36, 19));
                    ImGui::PushFont(InterBold12 ? InterBold12 : InterBold);
                    ImGui::TextColored(ImVec4(0.97f, 0.45f, 0.45f, 1.0f), "FUUL SIFE");
                    ImGui::PopFont();

                    // Close Button (top right)
                    ImGui::SetCursorPos(ImVec2(Size.x - 30, 8));
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.08f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,1,0.4f));
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
                    if (ImGui::Button("-", ImVec2(24, 24))) {
                        bIsMenuOpen = false;
                        SetWindowLong(hWindow, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE);
                        SetForegroundWindow(hTargetWindow);
                        SetWindowPos(hWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
                    }
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor(3);

                    // --- LOGIN header (REGISTER tab removed) ---
                    float tabY = 50.0f;
                    {
                        ImGui::PushFont(InterBold);
                        const char* title = "LOGIN";
                        ImVec2 titleSize = ImGui::CalcTextSize(title);
                        ImVec2 titlePos = Pos + ImVec2((Size.x - titleSize.x) * 0.5f, tabY + (26.0f - titleSize.y) * 0.5f);
                        DrawList->AddText(titlePos, ImColor(255, 255, 255, 255), title);
                        ImGui::PopFont();
                        DrawList->AddRectFilled(Pos + ImVec2(16, tabY + 27), Pos + ImVec2(Size.x - 16, tabY + 30), ImColor(239, 68, 68, 255), 1.5f);
                    }

                    // Separator under header
                    DrawList->AddLine(Pos + ImVec2(0, tabY + 30.0f), Pos + ImVec2(Size.x, tabY + 30.0f), ImColor(38, 40, 56, 255));

                    // Layout body
                    float startY = tabY + 45.0f;
                    float inputWidth = 300.0f;
                    float inputHeight = 32.0f;
                    float spacingY = 12.0f;
                    float centerX = (Size.x - inputWidth) * 0.5f;

                    // Push standard style variables for inputs
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 8));
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImColor(18, 19, 28, 255).Value);
                    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImColor(24, 25, 38, 255).Value);
                    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImColor(30, 32, 48, 255).Value);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImColor(240, 242, 248, 255).Value);
                    ImGui::PushStyleColor(ImGuiCol_Border, ImColor(38, 40, 56, 255).Value);

                    // --- Username Input ---
                    ImGui::SetCursorPos(ImVec2(centerX, startY));
                    ImGui::PushItemWidth(inputWidth);
                    ImGui::InputTextWithHint("##Username", "Username", username, IM_ARRAYSIZE(username));
                    {
                        bool active = ImGui::IsItemActive();
                        bool hovered = ImGui::IsItemHovered();
                        ImU32 borderColor = active ? ImColor(239, 68, 68, 255) : (hovered ? ImColor(239, 68, 68, 120) : ImColor(38, 40, 56, 255));
                        DrawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), borderColor, 6.0f, 0, 1.0f);
                    }
                    ImGui::PopItemWidth();

                    // --- Password Input ---
                    startY += inputHeight + spacingY;
                    ImGui::SetCursorPos(ImVec2(centerX, startY));
                    ImGui::PushItemWidth(inputWidth);
                    ImGui::InputTextWithHint("##Password", "Password", password, IM_ARRAYSIZE(password), ImGuiInputTextFlags_Password);
                    {
                        bool active = ImGui::IsItemActive();
                        bool hovered = ImGui::IsItemHovered();
                        ImU32 borderColor = active ? ImColor(239, 68, 68, 255) : (hovered ? ImColor(239, 68, 68, 120) : ImColor(38, 40, 56, 255));
                        DrawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), borderColor, 6.0f, 0, 1.0f);
                    }
                    ImGui::PopItemWidth();

                    ImGui::PopStyleColor(5);
                    ImGui::PopStyleVar(2);

                    // --- Error / Status Message ---
                    float msgY = startY + inputHeight + 8.0f;
                    if (auth_error && !error_msg.empty()) {
                        ImGui::PushFont(InterBold12);
                        float tw = ImGui::CalcTextSize(error_msg.c_str()).x;
                        ImGui::SetCursorPos(ImVec2((Size.x - tw) * 0.5f, msgY));
                        ImGui::TextColored(ImVec4(0.95f, 0.25f, 0.37f, 1.0f), "%s", error_msg.c_str());
                        ImGui::PopFont();
                    }

                    // --- Submit Button ---
                    float buttonWidth = 240.0f;
                    float buttonHeight = 36.0f;
                    float buttonY = winHeight - 55.0f;
                    float buttonX = (Size.x - buttonWidth) * 0.5f;

                    ImGui::SetCursorPos(ImVec2(buttonX, buttonY));
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

                    if (loginInProgress) {
                        float loadingPulse = (sinf((float)ImGui::GetTime() * 4.0f) + 1.0f) * 0.5f;
                        ImColor loadingColor = ImColor(30 + (int)(loadingPulse * 15), 30 + (int)(loadingPulse * 15), 45 + (int)(loadingPulse * 20));

                        ImGui::PushStyleColor(ImGuiCol_Button, loadingColor.Value);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, loadingColor.Value);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, loadingColor.Value);
                        ImGui::ButtonStyled("CONNECTING...", ImVec2(buttonWidth, buttonHeight), true);
                        ImGui::PopStyleColor(3);
                    }
                    else {
                        // Royal Crimson buttons
                        ImColor btnColor = ImColor(239, 68, 68);
                        ImColor btnHover = ImColor(248, 113, 113);
                        ImColor btnActive = ImColor(220, 38, 38);

                        ImGui::PushStyleColor(ImGuiCol_Button, btnColor.Value);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnHover.Value);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnActive.Value);

                        if (ImGui::ButtonStyled("SECURE LOGIN", ImVec2(buttonWidth, buttonHeight))) {
                            loginInProgress = true;
                            auth_error = false;
                            error_msg.clear();

                            std::thread([&]() {
                                std::string setupResult = g_Api.setup();
                                bool initOk = g_Api.response.success;
                                KeyAuth_message = g_Api.response.message;

                                if (initOk) {
                                    std::string authResult = g_Api.Login(std::string(username), std::string(password));

                                    loginSuccess = g_Api.response.success && (authResult == "success");
                                    KeyAuth_message = g_Api.response.message;

                                    if (!loginSuccess) {
                                        auth_error = true;
                                        if (HubAuth::LooksExpired(KeyAuth_message)) {
                                            // Expired license: native error + auto Discord, panel disabled
                                            error_msg = HubAuth::kExpiredMessage;
                                            g_SessionExpiredTriggered.store(true);
                                            g_SessionExpiredPopupPending.store(true);
                                            g_SessionBlocked.store(true);
                                        }
                                        else {
                                            error_msg = !KeyAuth_message.empty() ? KeyAuth_message : "Authentication failed.";
                                        }
                                    }
                                    else {
                                        // Fresh login: reload expiry countdown and persist it
                                        g_SessionExpiryTime.store(0);
                                        g_SessionExpiredTriggered.store(false);
                                        g_SessionBlocked.store(false);
                                        HubAuth::SaveExpiry(
                                            HubAuth::ParseExpiry(g_Api.client.sub_type.expire_date),
                                            std::string(username));
                                        isAuthenticated = true;
                                    }
                                }
                                else {
                                    loginSuccess = false;
                                    auth_error = true;
                                    error_msg = !KeyAuth_message.empty() ? KeyAuth_message : "Server handshake failed.";
                                }
                                loginInProgress = false;
                            }).detach();
                        }
                        ImGui::PopStyleColor(3);
                    }

                    ImGui::PopStyleVar();

                    // --- Footer copyright ---
                    ImGui::PushFont(InterBold12);
                    const char* copyrightText = "developed by @Ani";
                    float cpw = ImGui::CalcTextSize(copyrightText).x;
                    ImGui::SetCursorPos(ImVec2((Size.x - cpw) * 0.5f, winHeight - 16.0f));
                    ImGui::TextColored(ImColor(107, 114, 128, 140), "%s", copyrightText);
                    ImGui::PopFont();
                }
                ImGui::End();
                return;
            }

        if (bIsMainMinimized) {
            // Elegant circular glass orb with pulsing red aura
            const float orbRadius = 22.0f;
            ImGui::SetNextWindowSize(ImVec2(orbRadius * 2, orbRadius * 2));
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
            ImGuiWindowFlags iconFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, orbRadius);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.043f, 0.047f, 0.063f, 0.95f)); // Obsidian
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.937f, 0.267f, 0.267f, 0.70f));   // Crimson Red

            if (ImGui::Begin("FloatingIcon", nullptr, iconFlags)) {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 windowPos = ImGui::GetWindowPos();
                ImVec2 windowSize = ImGui::GetWindowSize();
                ImVec2 center = ImVec2(windowPos.x + windowSize.x * 0.5f, windowPos.y + windowSize.y * 0.5f);
                
                bool isHovered = ImGui::IsWindowHovered();

                // Breathing pulse aura
                float t = (float)ImGui::GetTime();
                float pulse = sinf(t * 3.0f) * 0.5f + 0.5f;
                drawList->AddCircle(center, orbRadius + 1.0f + pulse * 2.0f, ImColor(239, 68, 68, (int)(40 + pulse * 60)), 32, 1.5f);

                // Hover glow
                if (isHovered) {
                    drawList->AddCircleFilled(center, orbRadius - 1.0f, ImColor(239, 68, 68, 25), 32);
                }

                // Centered logo (HUB badge) — click to open the panel
                if (Logo) {
                    float logoSz = orbRadius * 1.6f;
                    ImVec2 logoMin(center.x - logoSz * 0.5f, center.y - logoSz * 0.5f);
                    drawList->AddImage((ImTextureID)Logo, logoMin, ImVec2(logoMin.x + logoSz, logoMin.y + logoSz));
                } else {
                    ImGui::PushFont(InterExtraBold ? InterExtraBold : InterBold);
                    const char* text = "P";
                    ImVec2 textSize = ImGui::CalcTextSize(text);
                    ImVec2 textPos = ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f - 1.0f);
                    ImU32 textColor = isHovered ? ImColor(255, 255, 255, 255) : ImColor(240, 242, 248, 240);
                    drawList->AddText(textPos, textColor, text);
                    ImGui::PopFont();
                }

                // Click to restore
                if (isHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 9.0f) {
                    bIsMainMinimized = false;
                }
            }
            ImGui::End();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);
        }
        else
        {

        static float AnimaTab = 0.f;
        static int LastCurrentTab = 0;
        static float TabFadeAlpha = 1.0f; // EFFECT: Tab fade-in alpha

        if (LastCurrentTab != CurrentTab)
        {
            AnimaTab = (LastCurrentTab > CurrentTab) ? -460.f : 460.f;
            TabFadeAlpha = 0.0f; // Reset fade on tab change
            LastCurrentTab = CurrentTab;
        }
        AnimaTab = ImLerp(AnimaTab, 0.f, 6.f * ImGui::GetIO().DeltaTime);
        TabFadeAlpha = ImLerp(TabFadeAlpha, 1.0f, 4.5f * ImGui::GetIO().DeltaTime); // Smooth fade-in
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, TabFadeAlpha); // Apply fade to all tab content

        // Dynamically size window based on current tab for a clean, compact layout
        ImVec2 winSize;
        if (CurrentTab == 0 || CurrentTab == 1 || CurrentTab == 10) {
            winSize = ImVec2(370, 240);
        } else {
            winSize = ImVec2(620, 420);
        }
        ImGui::SetNextWindowSize(winSize);

        ImGui::Begin("Dashboard", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            ImDrawList* DrawList = ImGui::GetWindowDrawList();
            ImVec2 Pos = ImGui::GetWindowPos();
            ImVec2 Size = ImGui::GetWindowSize();

            static int LastCurrentSub = 0;
            static int CurrentSub = 0;
            static float Anima = 0.f;

            if (LastCurrentSub != CurrentSub)
            {
                Anima = (LastCurrentSub > CurrentSub) ? -460.f : 460.f;
                LastCurrentSub = CurrentSub;
            }
            Anima = ImLerp(Anima, 0.f, 6.f * ImGui::GetIO().DeltaTime);

            if (CurrentTab == 0) // --- LOGIN / REGISTRO ---
            {
                CurrentTab = 10;
            }
            else if (CurrentTab == 10) // --- NEW LOADING SPINNER TAB ---
            {
                DrawList->AddRectFilled(Pos, Pos + Size, ImColor(11, 12, 16, 255), 12.0f);
                DrawList->AddRect(Pos, Pos + Size, ImColor(38, 40, 56, 220), 12.0f, 0, 1.0f);

                // Header
                ImGui::SetCursorPos(ImVec2(15, 10));
                DrawList->AddCircleFilled(Pos + ImVec2(22, 19), 4.0f, ImColor(239, 68, 68, 255), 12);
                ImGui::SetCursorPos(ImVec2(35, 10));
                ImGui::PushFont(InterBold);
                ImGui::Text("System"); ImGui::SameLine();
                ImGui::TextColored(ImVec4(1, 1, 1, 0.45f), "/  Initializing");
                ImGui::PopFont();

                // Header divider
                DrawList->AddLine(Pos + ImVec2(0, 38), Pos + ImVec2(Size.x, 38), ImColor(38, 40, 56, 255));

                // Centered Spinner in Crimson Red
                ImGui::SetCursorPos(ImVec2(Size.x * 0.5f - 20, 85));
                LoadingSpinner("##InitSpinner", 20.0f, 3.0f, IM_COL32(239, 68, 68, 255));

                // Status Message
                std::string status = g_Globals.General.StatusMessage;
                if (status.empty()) status = "Working...";
                float tw = ImGui::CalcTextSize(status.c_str()).x;
                ImGui::SetCursorPos(ImVec2((Size.x - tw) * 0.5f, 145));
                ImGui::PushFont(InterBold12);
                ImGui::TextColored(ImVec4(0.85f, 0.86f, 0.90f, 0.85f), "%s", status.c_str());
                ImGui::PopFont();

                // Transition Logic
                if (g_Globals.General.State == Globals::InitState::ReadyToScan) {
                    CurrentTab = 1; // Go to Set Process
                } else if (g_Globals.General.State == Globals::InitState::Failed) {
                    CurrentTab = 1; // Go to Set Process to show error
                }
            }
            else if (CurrentTab == 1) // --- SET PROCESS SCREEN ---
            {
                // Automated transition if already completed
                if (g_Globals.General.State == Globals::InitState::Completed) {
                    CurrentTab = 2;
                }

                DrawList->AddRectFilled(Pos, Pos + Size, ImColor(11, 12, 16, 255), 12.0f);
                DrawList->AddRect(Pos, Pos + Size, ImColor(38, 40, 56, 220), 12.0f, 0, 1.0f);

                // Top header for Set Process
                ImGui::SetCursorPos(ImVec2(15, 10));
                ImU32 bulletColor = (g_Globals.General.State == Globals::InitState::ReadyToScan) ? ImColor(16, 185, 129, 255) : ImColor(244, 63, 94, 255);
                DrawList->AddCircleFilled(Pos + ImVec2(22, 19), 4.0f, bulletColor, 12); 
                ImGui::SetCursorPos(ImVec2(35, 10));
                ImGui::PushFont(InterBold12);
                ImGui::Text(g_Globals.General.State == Globals::InitState::ReadyToScan ? "READY" : "OFFLINE"); ImGui::SameLine();
                ImGui::TextColored(ImVec4(1, 1, 1, 0.35f), "-  HUB COMMUNITY   -  FPS %.0f", ImGui::GetIO().Framerate);
                ImGui::PopFont();

                // Close Button (top right)
                ImGui::SetCursorPos(ImVec2(Size.x - 30, 8));
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.08f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,1,0.4f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
                if (ImGui::Button("-", ImVec2(24, 24))) {
                    bIsMainMinimized = true;
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(3);

                // Separator Line below Header
                DrawList->AddLine(Pos + ImVec2(0, 38), Pos + ImVec2(Size.x, 38), ImColor(38, 40, 56, 255));

                // Left Sidebar (with greyed-out icons)
                float sidebar_width = 48.0f;
                DrawList->AddRectFilled(Pos + ImVec2(0, 38), Pos + ImVec2(sidebar_width, Size.y), ImColor(9, 10, 14, 255), 12.0f, ImDrawFlags_RoundCornersLeft);
                DrawList->AddLine(Pos + ImVec2(sidebar_width, 38), Pos + ImVec2(sidebar_width, Size.y), ImColor(34, 36, 50, 255));

                auto NavIconOffline = [&](const char* icon, float y_offset) {
                    ImGui::SetCursorPos(ImVec2((sidebar_width - 24) * 0.5f, 38 + y_offset));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.20f));
                    ImGui::PushFont(InterRegular);
                    ImGui::Text("%s", icon);
                    ImGui::PopFont();
                    ImGui::PopStyleColor();
                };
                NavIconOffline(ICON_FA_SHIELD_HALVED, 15);
                NavIconOffline(ICON_FA_EYE, 50);
                NavIconOffline(ICON_FA_MICROCHIP, 85);
                NavIconOffline(ICON_FA_GEAR, 150);

                // Main area (right of sidebar)
                float main_w = Size.x - sidebar_width;
                static std::string scanStatus = "";
                static bool isScanning = false;
                static ImVec4 statusColor = ImVec4(1, 1, 1, 0.4f);

                // Centered Set Process button
                ImGui::SetCursorPos(ImVec2(sidebar_width + (main_w - 210) * 0.5f, 85));
                
                if (isScanning || g_Globals.General.State == Globals::InitState::ScanningModule) {
                    ImGui::BeginGroup();
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    ImVec2 pMin = ImGui::GetCursorScreenPos();
                    ImVec2 pMax = ImVec2(pMin.x + 210, pMin.y + 42);
                    drawList->AddRectFilled(pMin, pMax, ImColor(16, 17, 24, 255), 8.0f);
                    drawList->AddRect(pMin, pMax, ImColor(239, 68, 68, 160), 8.0f, 0, 1.5f);
                    
                    // Spinner placement
                    ImGui::SetCursorPos(ImVec2(sidebar_width + (main_w - 210) * 0.5f + 16, 85 + 11));
                    LoadingSpinner("##AdbConnectSpinner", 10.0f, 2.5f, IM_COL32(239, 68, 68, 255));
                    
                    // Text
                    ImGui::SetCursorPos(ImVec2(sidebar_width + (main_w - 210) * 0.5f + 48, 85 + 13));
                    ImGui::PushFont(InterBold12 ? InterBold12 : InterBold);
                    ImGui::TextColored(ImVec4(0.97f, 0.45f, 0.45f, 1.0f), "CONNECTING...");
                    ImGui::PopFont();
                    
                    ImGui::EndGroup();
                } else {
                    const char* btnText = "Connect Free Fire";
                    if (g_Globals.General.State == Globals::InitState::Failed) btnText = "Retry System Init";

                    if (ImGui::ButtonStyled(btnText, ImVec2(210, 42))) {
                        if (g_Globals.General.State == Globals::InitState::Failed) {
                            // Manual retry of system background init
                            g_Globals.General.State = Globals::InitState::InitializingMemory;
                            CurrentTab = 10;
                            extern void BackgroundInit();
                            extern std::thread g_InitThread;
                            if (g_InitThread.joinable()) g_InitThread.join();
                            g_InitThread = std::thread(BackgroundInit);
                        } else {
                            // Standard Set Process (Scan module)
                            isScanning = true;
                            g_Globals.General.State = Globals::InitState::ScanningModule;
                            scanStatus = "Connecting to emulator...";
                            statusColor = ImVec4(0.66f, 0.45f, 1.0f, 1.0f);
                            
                            std::thread([&]() {
                                ThreadGuard guard;

                                // 1) Launch the emulator (if needed) and Free Fire itself
                                scanStatus = "Launching Free Fire...";
                                statusColor = ImVec4(0.97f, 0.45f, 0.45f, 1.0f);

                                if (!FWork::ADB::LaunchFreeFire(60000)) {
                                    scanStatus = "Emulator / Free Fire not found";
                                    statusColor = ImVec4(0.95f, 0.25f, 0.35f, 1.0f);
                                    g_Globals.General.State = Globals::InitState::ReadyToScan;
                                    isScanning = false;
                                    return;
                                }

                                // 2) Wait for libil2cpp and read the module base
                                scanStatus = "Connecting to Free Fire...";
                                statusColor = ImVec4(1, 1, 1, 0.55f);

                                bool success = false;
                                for (int attempt = 0; attempt < 8 && !success; ++attempt) {
                                    success = FWork::ADB::InitializeADB();
                                    if (!success) Sleep(2000);
                                }

                                if (success) {
                                    scanStatus = "Success!";
                                    statusColor = ImVec4(0.1f, 0.85f, 0.4f, 1.0f);
                                    Features::FakeLag::Initialize();
                                    g_Globals.General.State = Globals::InitState::Completed;
                                    std::this_thread::sleep_for(std::chrono::seconds(1));
                                    CurrentTab = 2; // Transition to Combat tab
                                    isScanning = false;
                                } else {
                                    const std::string& err = FWork::ADB::GetLastError();
                                    scanStatus = err.empty() ? "Failed, please retry" : err;
                                    statusColor = ImVec4(0.95f, 0.25f, 0.35f, 1.0f);
                                    g_Globals.General.State = Globals::InitState::ReadyToScan;
                                    isScanning = false;
                                }
                            }).detach();
                        }
                    }
                }

                // Status Message display
                if (!scanStatus.empty()) {
                    float tw = ImGui::CalcTextSize(scanStatus.c_str()).x;
                    ImGui::SetCursorPos(ImVec2(sidebar_width + (main_w - tw) * 0.5f, 146));
                    ImGui::PushFont(InterBold12);
                    ImGui::TextColored(statusColor, "%s", scanStatus.c_str());
                    ImGui::PopFont();
                }

                // Footer comment
                ImGui::SetCursorPos(ImVec2(sidebar_width + 12, Size.y - 28));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.22f));
                ImGui::PushFont(InterBold12);
                ImGui::Text("Free Fire emulator running");
                ImGui::PopFont();
                ImGui::PopStyleColor();
            }
            else // --- MAIN DASHBOARD ---
            {
                float sidebar_width = 58.0f; 
                float header_height = 40.0f;

                // --- Backgrounds ---
                DrawList->AddRectFilled(Pos, Pos + Size, ImColor(11, 12, 16, 252), 12.0f); // Main body background
                DrawList->AddRect(Pos, Pos + Size, ImColor(38, 40, 56, 220), 12.0f, 0, 1.0f); // Window outer hairline
                DrawList->AddRectFilled(Pos, Pos + ImVec2(sidebar_width, Size.y), ImColor(9, 10, 14, 255), 12.0f, ImDrawFlags_RoundCornersLeft); // Sidebar background
                DrawList->AddRectFilled(Pos + ImVec2(sidebar_width, 0), Pos + ImVec2(Size.x, header_height), ImColor(13, 14, 20, 255), 12.0f, ImDrawFlags_RoundCornersTopRight); // Header background

                // --- EFFECT: Animated Luxury Gradient Glow ---
                {
                    float t = (float)ImGui::GetTime();
                    float pulse = (sinf(t * 1.5f) * 0.5f + 0.5f);
                    int glowAlpha = (int)(10 + pulse * 14);
                    // Top-left crimson glow
                    ImVec2 glowTL = Pos + ImVec2(sidebar_width, 0);
                    ImVec2 glowBR = Pos + ImVec2(sidebar_width + Size.x * 0.45f, Size.y * 0.40f);
                    DrawList->AddRectFilledMultiColor(glowTL, glowBR,
                        ImColor(239, 68, 68, glowAlpha), ImColor(0, 0, 0, 0),
                        ImColor(0, 0, 0, 0), ImColor(120, 0, 0, glowAlpha / 2));
                    // Bottom-right Sky Blue glow
                    ImVec2 glow2TL = Pos + ImVec2(Size.x * 0.55f, Size.y * 0.55f);
                    ImVec2 glow2BR = Pos + Size;
                    int glow2Alpha = (int)(6 + pulse * 10);
                    DrawList->AddRectFilledMultiColor(glow2TL, glow2BR,
                        ImColor(0, 0, 0, 0), ImColor(239, 68, 68, glow2Alpha / 2),
                        ImColor(239, 68, 68, glow2Alpha), ImColor(0, 0, 0, 0));
                }
                
                // Borders
                DrawList->AddLine(Pos + ImVec2(sidebar_width, 0), Pos + ImVec2(sidebar_width, Size.y), ImColor(34, 36, 50, 255));
                DrawList->AddLine(Pos + ImVec2(sidebar_width, header_height), Pos + ImVec2(Size.x, header_height), ImColor(34, 36, 50, 255));

                // --- EFFECT: Header Accent Line Shimmer ---
                {
                    float t = (float)ImGui::GetTime();
                    float shimmer = fmodf(t * 0.25f, 1.0f);
                    float lineW = Size.x - sidebar_width;
                    float shimX = sidebar_width + shimmer * lineW;
                    float halfGlow = lineW * 0.2f;
                    ImVec2 lnStart = Pos + ImVec2(sidebar_width, header_height);
                    ImVec2 lnEnd = Pos + ImVec2(Size.x, header_height);
                    DrawList->AddLine(lnStart, lnEnd, ImColor(239, 68, 68, 50), 1.0f);
                    float sLeft = (std::max)(sidebar_width, shimX - halfGlow);
                    float sRight = (std::min)(Size.x, shimX + halfGlow);
                    if (sRight > sLeft) {
                        DrawList->AddRectFilledMultiColor(
                            Pos + ImVec2(sLeft, header_height - 1),
                            Pos + ImVec2(sRight, header_height + 1),
                            ImColor(239, 68, 68, 0), ImColor(248, 113, 113, 130),
                            ImColor(248, 113, 113, 130), ImColor(239, 68, 68, 0));
                    }
                }

                // --- Top Branding Header ---
                {
                    float pulse = sinf((float)ImGui::GetTime() * 3.0f) * 0.5f + 0.5f;

                    // Main name
                    ImGui::SetCursorPos(ImVec2(sidebar_width + 28, 4));
                    ImGui::PushFont(InterBold);
                    ImGui::Text("HUB COMMUNITY");
                    ImVec2 titleSize = ImGui::CalcTextSize("HUB COMMUNITY");
                    ImGui::PopFont();

                    // Sub name: FUUL SIFE
                    ImGui::SetCursorPos(ImVec2(sidebar_width + 28, 21));
                    ImGui::PushFont(InterBold12 ? InterBold12 : InterBold);
                    ImGui::TextColored(ImVec4(0.97f, 0.45f, 0.45f, 1.0f), "FUUL SIFE");
                    ImVec2 subSize = ImGui::CalcTextSize("FUUL SIFE");
                    ImGui::PopFont();

                    // Live dot after the sub name
                    ImVec2 dotPos = Pos + ImVec2(sidebar_width + 28 + subSize.x + 10, 27);
                    DrawList->AddCircleFilled(dotPos, 3.0f, ImColor(16, 185, 129, 255), 12);
                    DrawList->AddCircle(dotPos, 4.0f + pulse * 1.5f, ImColor(16, 185, 129, (int)(pulse * 80)), 12, 1.0f);

                    // Version badge on the title line
                    ImGui::SetCursorPos(ImVec2(sidebar_width + 28 + titleSize.x + 8, 5));
                    ImGui::PushFont(InterBold12 ? InterBold12 : InterBold);
                    ImVec2 badgeSize = ImGui::CalcTextSize("V.18 SAFE") + ImVec2(8, 2);
                    ImVec2 badgePos = Pos + ImGui::GetCursorPos();
                    DrawList->AddRectFilled(badgePos, badgePos + badgeSize, ImColor(239, 68, 68, 35), 4.0f);
                    DrawList->AddRect(badgePos, badgePos + badgeSize, ImColor(239, 68, 68, 100), 4.0f, 0, 1.0f);
                    ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(4, 1));
                    ImGui::TextColored(ImVec4(0.97f, 0.45f, 0.45f, 1.0f), "V.18 SAFE");
                    ImGui::PopFont();

                    // FPS Pill
                    char fpsBuf[32];
                    sprintf_s(fpsBuf, "%.0f FPS", ImGui::GetIO().Framerate);
                    ImVec2 fpsSize = ImGui::CalcTextSize(fpsBuf) + ImVec2(10, 4);
                    float fpsX = Size.x - 68.0f - fpsSize.x;
                    ImVec2 fpsPos = Pos + ImVec2(fpsX, (header_height - fpsSize.y) * 0.5f);
                    DrawList->AddRectFilled(fpsPos, fpsPos + fpsSize, ImColor(20, 22, 32, 220), 4.0f);
                    DrawList->AddRect(fpsPos, fpsPos + fpsSize, ImColor(38, 40, 56, 180), 4.0f, 0, 1.0f);
                    ImGui::SetCursorPos(ImVec2(fpsX + 5, (header_height - 14) * 0.5f));
                    ImGui::PushFont(InterBold12 ? InterBold12 : InterBold);
                    ImGui::TextColored(ImVec4(0.65f, 0.68f, 0.76f, 0.9f), "%s", fpsBuf);
                    ImGui::PopFont();
                }

                // --- Top-left HUB Badge (window logo) ---
                {
                    const float badgeSz = 28.0f;
                    ImVec2 badgeMin = Pos + ImVec2((sidebar_width - badgeSz) * 0.5f, (header_height - badgeSz) * 0.5f);
                    ImVec2 badgeMax = badgeMin + ImVec2(badgeSz, badgeSz);
                    if (Logo) {
                        DrawList->AddImage((ImTextureID)Logo, badgeMin, badgeMax);
                    } else {
                        DrawList->AddRectFilled(badgeMin, badgeMax, ImColor(239, 68, 68, 50), 8.0f);
                        DrawList->AddRect(badgeMin, badgeMax, ImColor(239, 68, 68, 190), 8.0f, 0, 1.0f);
                    }
                }

                // Window Controls (top right)
                ImGui::SetCursorPos(ImVec2(Size.x - 52, 9));
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.08f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,1,0.45f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
                if (ImGui::Button("-##min", ImVec2(20, 20))) {
                    bIsMainMinimized = true;
                }
                ImGui::SameLine(0, 4);
                if (ImGui::Button("x##close", ImVec2(20, 20))) {
                    bIsMenuOpen = false;
                    SetWindowLong(hWindow, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE);
                    SetForegroundWindow(hTargetWindow);
                    SetWindowPos(hWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(3);

                // --- Minimalist Navigation Sidebar with Gliding Pill ---
                ImGui::SetCursorPos(ImVec2(0, header_height));
                ImGui::BeginChild("Sidebar", ImVec2(sidebar_width, Size.y - header_height), false, ImGuiWindowFlags_NoScrollbar);
                {
                    // Gliding active indicator
                    static float g_IndicatorY = 10.0f;
                    float targetY = 10.0f;
                    if (CurrentTab == 2) targetY = 10.0f;
                    else if (CurrentTab == 3) targetY = 52.0f;
                    else if (CurrentTab == 5) targetY = 94.0f;
                    else if (CurrentTab == 6) targetY = Size.y - header_height - 50.0f;

                    g_IndicatorY = ImLerp(g_IndicatorY, targetY, 1.0f - expf(-18.0f * ImGui::GetIO().DeltaTime));

                    // Draw gliding pill
                    DrawList->AddRectFilled(Pos + ImVec2(6, header_height + g_IndicatorY), Pos + ImVec2(sidebar_width - 6, header_height + g_IndicatorY + 34.0f), ImColor(239, 68, 68, 35), 7.0f);
                    DrawList->AddRectFilled(Pos + ImVec2(3, header_height + g_IndicatorY + 7), Pos + ImVec2(6, header_height + g_IndicatorY + 27), ImColor(248, 113, 113, 255), 1.5f);
                    DrawList->AddRect(Pos + ImVec2(6, header_height + g_IndicatorY), Pos + ImVec2(sidebar_width - 6, header_height + g_IndicatorY + 34.0f), ImColor(239, 68, 68, 90), 7.0f, 0, 1.0f);

                    auto NavButton = [&](const char* icon, int tab_id, float y_pos) {
                        bool active = (CurrentTab == tab_id);
                        ImGui::SetCursorPos(ImVec2((sidebar_width - 34) * 0.5f, y_pos));

                        std::string id_str = std::string("##nav_") + std::to_string(tab_id);
                        ImGui::InvisibleButton(id_str.c_str(), ImVec2(34, 34));
                        bool hovered = ImGui::IsItemHovered();
                        if (ImGui::IsItemClicked()) {
                            CurrentTab = tab_id;
                        }

                        if (hovered && !active) {
                            DrawList->AddRectFilled(Pos + ImVec2(6, header_height + y_pos), Pos + ImVec2(sidebar_width - 6, header_height + y_pos + 34), ImColor(255, 255, 255, 10), 7.0f);
                        }

                        // Render centered icon
                        ImGui::PushFont(FontAwesomeSolid);
                        ImVec2 icon_size = ImGui::CalcTextSize(icon);
                        ImVec2 icon_pos = Pos + ImVec2(sidebar_width / 2.0f - icon_size.x / 2.0f, header_height + y_pos + 17.0f - icon_size.y / 2.0f);
                        DrawList->AddText(icon_pos, active ? ImColor(255, 255, 255, 255) : (hovered ? ImColor(220, 222, 230, 255) : ImColor(107, 114, 128, 190)), icon);
                        ImGui::PopFont();
                    };

                    NavButton(ICON_FA_SHIELD_HALVED, 2, 10);
                    NavButton(ICON_FA_EYE, 3, 52);
                    NavButton(ICON_FA_GEAR, 5, 94);
                    NavButton(ICON_FA_CIRCLE_INFO, 6, Size.y - header_height - 50.0f); // Info at bottom
                }
                ImGui::EndChild();

                // --- Dynamic Tab Content ---
                ImGui::SetCursorPos(ImVec2(sidebar_width + 10, header_height + 10));
                ImGui::BeginChild("MainContent", ImVec2(Size.x - sidebar_width - 20, Size.y - header_height - 20), false, ImGuiWindowFlags_NoScrollbar);
                {
                    float cw = ImGui::GetContentRegionAvail().x;
                    float ch = ImGui::GetContentRegionAvail().y;

                    if (CurrentTab == 2) // --- COMBAT ---
                    {
                        // Column 1
                        ImGui::SetCursorPos(ImVec2(0, 0));
                        ImGui::BeginChild("AimbotCol1", ImVec2(cw / 2 - 6, ch), false, ImGuiWindowFlags_NoScrollbar);
                        {
                            ImGui::CustomChild("Silent Aim", ImVec2(cw / 2 - 6, ch));
                            {
                                ImGui::Checkbox("SILENT AIM", &g_Globals.AimBot.SilentAimEnabled);
                                ImGui::KeyBind("Silent Toggle", &g_Globals.AimBot.SilentLegitToggleKeybind, 0);
                                /* ImGui::KeyBind("Silent Key", &g_Globals.AimBot.SilentAimBind, 0);*/
                                // Silent mode / targeting options trimmed:
                                // Standard (Closest Crosshair) only.
                                g_Globals.AimBot.SilentAimMode = 0;
                                g_Globals.AimBot.SilentTargetingMode = 0;
                                ImGui::SliderFloat("Silent Jitter", &g_Globals.AimBot.SilentJitter, 0.0f, 0.2f, "%.3f");


                            }
                            ImGui::EndCustomChild();
                        }
                        ImGui::EndChild();

                        // Column 2
                        ImGui::SetCursorPos(ImVec2(cw / 2 + 6, 0));
                        ImGui::BeginChild("AimbotCol2", ImVec2(cw / 2 - 6, ch), false, ImGuiWindowFlags_NoScrollbar);
                        {
                            ImGui::CustomChild("Weapon & Filter", ImVec2(cw / 2 - 6, ch));
                            {
                                ImGui::TextColored(ImVec4(1,1,1,0.45f), "Weapon Mods:");
                                ImGui::Checkbox("No Recoil", &g_Globals.AimBot.NoRecoil);

                                ImGui::Checkbox("Fast Reload", &g_Globals.AimBot.FastReload);
                                ImGui::KeyBind("Fast Reload Toggle", &g_Globals.AimBot.FastReloadToggleKeybind, 0);

                            }
                            ImGui::EndCustomChild();
                        }
                        ImGui::EndChild();
                    }
                    else if (CurrentTab == 3) // --- VISUALS ---
                    {
                        // Column 1
                        ImGui::SetCursorPos(ImVec2(0, 0));
                        ImGui::BeginChild("VisualsCol1", ImVec2(cw / 2 - 6, ch), false, ImGuiWindowFlags_NoScrollbar);
                        {
                            ImGui::CustomChild("Enemy ESP", ImVec2(cw / 2 - 6, ch));
                            {
                                ImGui::Checkbox("Master Enable", &g_Globals.Visuals.Enable);
                                ImGui::Checkbox("Lines", &g_Globals.Visuals.Lines);
                                ImGui::Checkbox("Box 2D", &g_Globals.Visuals.Box);
                                ImGui::Checkbox("Skeleton", &g_Globals.Visuals.Skeleton);
                                ImGui::Checkbox("Names", &g_Globals.Visuals.Name);
                                ImGui::Checkbox("Distance", &g_Globals.Visuals.Distance);
                                ImGui::Checkbox("Player Level", &g_Globals.Visuals.Level);
                                ImGui::Checkbox("Player ID", &g_Globals.Visuals.ID);
                                
                                if (ImGui::Checkbox("Health Text ESP", &g_Globals.Visuals.ESPHealthTEXT)) {
                                    g_Globals.Visuals.HealthBar = g_Globals.Visuals.ESPHealthTEXT;
                                }
                                ImGui::Checkbox("Health Bar ESP", &g_Globals.Visuals.HealthBar);
                            }
                            ImGui::EndCustomChild();
                        }
                        ImGui::EndChild();

                        // Column 2
                        ImGui::SetCursorPos(ImVec2(cw / 2 + 6, 0));
                        ImGui::BeginChild("VisualsCol2", ImVec2(cw / 2 - 6, ch), false, ImGuiWindowFlags_NoScrollbar);
                        {
                            ImGui::CustomChild("ESP Settings", ImVec2(cw / 2 - 6, ch));
                            {
                                ImGui::Combo("Lines Position", &g_Globals.Visuals.EspLines, "None\0Top\0Bottom\0");
                                ImGui::Combo("Box Style", &g_Globals.Visuals.players_box, "None\0Full\0Cornered\0");
                                ImGui::Combo("Health Style", &g_Globals.Visuals.players_healthbar, "None\0Left\0Right\0Top\0Bottom\0Text Only\0");
                                ImGui::SliderInt("ESP Distance", &g_Globals.Visuals.DistanceEsp, 0, 500, "%dm");

                                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                                ImGui::Checkbox("Glow Lines", &g_Globals.Visuals.GlowLines);
                                ImGui::Checkbox("Rainbow Lines", &g_Globals.Visuals.RainbowLines);
                                ImGui::SliderFloat("Glow Radius", &g_Globals.Visuals.GlowRadius, 1.0f, 30.0f, "%.1f");
                                ImGui::SliderFloat("Glow Feather", &g_Globals.Visuals.GlowFeather, 0.5f, 10.0f, "%.1f");
                                ImGui::SliderFloat("Line Thickness", &g_Globals.Visuals.LineThickness, 0.5f, 5.0f, "%.1f");
                                
                                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                                ImGui::Checkbox("Box Border Fill", &g_Globals.Visuals.FilledBox);
                                ImGui::Checkbox("Box Solid Fill", &g_Globals.Visuals.FillColorBox);
                                ImGui::ColorEdit4("Box Solid Color", g_Globals.Visuals.FillColor, ImGuiColorEditFlags_NoInputs);
                                
                                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                                if (ImGui::Checkbox("Refresh ESP", &g_Globals.Visuals.RefreshEsp)) {
                                    if (g_Globals.Visuals.RefreshEsp)
                                        FWork::Data::Refresh();
                                }
                                ImGui::KeyBind("Refresh Toggle", &g_Globals.Visuals.RefreshEspToggleKeybind, 0);
                                
                                ImGui::Spacing();
                                ImGui::Checkbox("Fix ESP", &g_Globals.Visuals.FixESP);
                                ImGui::KeyBind("Fix ESP Toggle", &g_Globals.Visuals.FixESPToggleKeybind, 0);
                                
                                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                                
                                ImGui::ColorEdit4("ESP Box Color", g_Globals.Visuals.BoxColor, ImGuiColorEditFlags_NoInputs);
                                ImGui::ColorEdit4("ESP Lines Color", g_Globals.Visuals.LinesColor, ImGuiColorEditFlags_NoInputs);
                                ImGui::ColorEdit4("Skeleton Color", g_Globals.Visuals.SkeletonColor, ImGuiColorEditFlags_NoInputs);
                                ImGui::ColorEdit4("Enemies Text Color", g_Globals.Visuals.EnemyColor, ImGuiColorEditFlags_NoInputs);

                                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                                ImGui::TextColored(ImVec4(1,1,1,0.4f), "Chams:");
                                ImGui::Checkbox("Through Walls", &g_Globals.Visuals.ChamsTransparente);
                                ImGui::Checkbox("White Body", &g_Globals.Visuals.Blanco);
                                ImGui::Checkbox("Black Body", &g_Globals.Visuals.Negro);
                                ImGui::Checkbox("Texture Walls", &g_Globals.Visuals.Celeste);
                                ImGui::Checkbox("Wireframes Texture", &g_Globals.Visuals.Frame);
                                ImGui::Checkbox("Glow Ouline", &g_Globals.Visuals.Glow);
                            }
                            ImGui::EndCustomChild();
                        }
                        ImGui::EndChild();
                    }
                    else if (CurrentTab == 5) // --- SETTINGS (MISC) ---
                    {
                        // Column 1
                        ImGui::SetCursorPos(ImVec2(0, 0));
                        ImGui::BeginChild("MiscCol1", ImVec2(cw / 2 - 6, ch), false, ImGuiWindowFlags_NoScrollbar);
                        {
                            ImGui::CustomChild("Global Settings", ImVec2(cw / 2 - 6, ch));
                            {
                                ImGui::Checkbox("Stream Mode", &g_Globals.General.Capture);
                                ImGui::SliderInt("Render Delay", &g_Globals.General.Delay, 0, 100, "%dms");
                                ImGui::KeyBind("Toggle Menu", &g_Globals.General.MenuKey, 0);

                                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                                static std::string cfgStatus = "";
                                if (ImGui::ButtonStyled("SAVE CONFIG", ImVec2(ImGui::GetContentRegionAvail().x, 32))) {
                                    if (SaveConfig()) cfgStatus = "Config Saved Successfully!"; else cfgStatus = "Save Failed!";
                                }
                                ImGui::Spacing();
                                if (ImGui::ButtonStyled("LOAD SAVED CONFIG", ImVec2(ImGui::GetContentRegionAvail().x, 32))) {
                                    if (LoadConfig()) cfgStatus = "Config Loaded Successfully!"; else cfgStatus = "Load Failed!";
                                }
                                if (!cfgStatus.empty()) {
                                    ImGui::PushFont(InterBold12 ? InterBold12 : InterBold);
                                    ImGui::TextColored(ImVec4(0.1f, 0.85f, 0.4f, 1.0f), "%s", cfgStatus.c_str());
                                    ImGui::PopFont();
                                }
                            }
                            ImGui::EndCustomChild();
                        }
                        ImGui::EndChild();

                        // Column 2
                        ImGui::SetCursorPos(ImVec2(cw / 2 + 6, 0));
                        ImGui::BeginChild("MiscCol2", ImVec2(cw / 2 - 6, ch), false, ImGuiWindowFlags_NoScrollbar);
                        {
                            ImGui::CustomChild("System Actions", ImVec2(cw / 2 - 6, ch));
                            {
                                ImGui::TextColored(ImVec4(1, 1, 1, 0.45f), "System Maintenance:");
                                static std::string scanStatus = "";
                                if (ImGui::ButtonStyled("CONNECT FREE FIRE AGAIN", ImVec2(ImGui::GetContentRegionAvail().x, 34))) {
                                    Offsets::Il2Cpp = 0; 
                                    scanStatus = "Launching Free Fire...";
                                    std::thread([&]() { 
                                        ThreadGuard guard;
                                        if (!FWork::ADB::LaunchFreeFire(60000)) {
                                            scanStatus = "Emulator / Free Fire not found";
                                            return;
                                        }
                                        scanStatus = "Connecting...";
                                        bool ok = false;
                                        for (int attempt = 0; attempt < 8 && !ok; ++attempt) {
                                            ok = FWork::ADB::InitializeADB();
                                            if (!ok) Sleep(2000);
                                        }
                                        if (ok) scanStatus = "Connected!";
                                        else {
                                            const std::string& err = FWork::ADB::GetLastError();
                                            scanStatus = err.empty() ? "Failed!" : ("Failed: " + err);
                                        }
                                        if (ok) Features::FakeLag::Initialize(); 
                                    }).detach();
                                }
                                if (!scanStatus.empty()) {
                                    ImGui::PushFont(InterBold12 ? InterBold12 : InterBold);
                                    ImGui::TextColored(ImVec4(0.1f, 0.85f, 0.4f, 1.0f), "Result: %s", scanStatus.c_str());
                                    ImGui::PopFont();
                                }
                                ImGui::Spacing();
                                if (ImGui::ButtonStyled("UNLOAD PANEL", ImVec2(ImGui::GetContentRegionAvail().x, 34))) {
                                    g_Globals.General.ShutDown = true;
                                }
                            }
                            ImGui::EndCustomChild();
                        }
                        ImGui::EndChild();
                    }
                    else if (CurrentTab == 6) // --- INFO ---
                    {
                        ImGui::CustomChild("Account & System", ImVec2(cw, ch));
                        {
                            ImGui::Text("STATUS:"); ImGui::SameLine();
                            ImGui::TextColored(ImVec4(0.1f, 0.85f, 0.4f, 1.0f), "ACTIVE / SECURE");

                            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                            ImGui::Text("VERSION:     HUB COMMUNITY  V.18 Safe");
                            ImGui::Text("EDITION:     FUUL SIFE");
                            ImGui::Text("THEME:       Crimson Red & Obsidian Glass");
                            ImGui::Text("BUILD DATE:  %s", __DATE__);
                            ImGui::Text("BUILD TIME:  %s", __TIME__);

                            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                            ImGui::TextColored(ImVec4(0.97f, 0.45f, 0.45f, 1.0f), "DEVELOPER:   @HUB COMMUNITY");

                            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                            ImGui::TextColored(ImVec4(1, 1, 1, 0.45f), "Official Community Server:");
                            ImGui::Spacing();
                            if (ImGui::ButtonStyled("CONNECTING TO SERVER", ImVec2(ImGui::GetContentRegionAvail().x, 36))) {
                                ShellExecuteA(nullptr, "open", "https://discord.gg/6BNChkFvP", nullptr, nullptr, SW_SHOWNORMAL);
                            }
                        }
                        ImGui::EndCustomChild();
                    }
                }
                ImGui::EndChild();
            }
            ImGui::End();
        }

        } 

        } 

        if (CurrentTab >= 2 && g_Globals.Misc.StatusPanelVisible) {
            RenderStatusPanel();
        }
    }

    void Interface::RenderStatusPanel()
    {
        if (CurrentTab < 2 || !g_Globals.Misc.StatusPanelVisible)
            return;

        auto VKName = [](int vk) -> std::string {
            if (vk == 0)           return "None";
            if (vk == VK_LBUTTON)  return "M1";
            if (vk == VK_RBUTTON)  return "M2";
            if (vk == VK_MBUTTON)  return "M3";
            if (vk == VK_XBUTTON1) return "XB1";
            if (vk == VK_XBUTTON2) return "XB2";
            if (vk == VK_TAB)      return "Tab";
            if (vk == VK_SHIFT)    return "Shift";
            if (vk == VK_CONTROL)  return "Ctrl";
            if (vk == VK_MENU)     return "Alt";
            if (vk == VK_SPACE)    return "Space";
            if (vk == VK_INSERT)   return "Ins";
            if (vk == VK_DELETE)   return "Del";
            if (vk == VK_HOME)     return "Home";
            if (vk == VK_END)      return "End";
            if (vk == VK_PRIOR)    return "PgUp";
            if (vk == VK_NEXT)     return "PgDn";
            if (vk >= VK_F1 && vk <= VK_F12) return "F" + std::to_string(vk - VK_F1 + 1);
            if (vk >= '0' && vk <= '9') return std::string(1, (char)vk);
            if (vk >= 'A' && vk <= 'Z') return std::string(1, (char)vk);
            return "VK" + std::to_string(vk);
        };

        struct KeyItem { std::string name; int bind; bool featureOn; };
        std::vector<KeyItem> items;

        auto TryAdd = [&](const char* name, int bind, bool featureOn) {
            if (bind == 0) return;
            items.push_back({ name, bind, featureOn });
        };

        TryAdd("Aimbot", g_Globals.AimBot.AimbotToggleKeybind, g_Globals.AimBot.Enabled);
        TryAdd("Fast Reload", g_Globals.AimBot.FastReloadToggleKeybind, g_Globals.AimBot.FastReload);
        TryAdd("Silent Aim", g_Globals.AimBot.SilentLegitToggleKeybind, g_Globals.AimBot.SilentAimEnabled);
       // TryAdd("Force Aim", g_Globals.General.ForceAimToggleKeybind, g_Globals.General.ForceonAim);
       // TryAdd("Mouse Aim", g_Globals.AimBot.MouseAimToggleKeybind, g_Globals.AimBot.MouseAim);
        TryAdd("Teleport Kill", g_Globals.Exploits.FakeLag_TeleToggleKeybind, g_Globals.Exploits.FakeLag_Tele);
        TryAdd("Freeze", g_Globals.Exploits.FakeLag_FreezeToggleKeybind, g_Globals.Exploits.FakeLag_Freeze);
        TryAdd("Ghost", g_Globals.Exploits.FakeLag_GhostToggleKeybind, g_Globals.Exploits.FakeLag_Ghost);
       // TryAdd("Infinite Skill", g_Globals.Exploits.InfinitySkillToggleKeybind, g_Globals.Exploits.InfinitySkill);
        TryAdd("Teleport Mark", g_Globals.Exploits.TeleportMarkToggleKeybind, g_Globals.Exploits.TeleportMarkEnabled);
       // TryAdd("Enemy Pull", g_Globals.Exploits.EnemyPullToggleKeybind, g_Globals.Exploits.EnemyPullEnabled);

        int enemyCount = 0;
        {
            SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
            for (auto& [id, p] : g_Globals.EspConfig.Entities) {
                (void)id;
                if (!p.IsDead) enemyCount++;
            }
        }

        auto& ac = g_Globals.Misc.AccentColor;
        auto& sc = g_Globals.Misc.SecondaryColor;

        ImU32 bgColor   = IM_COL32(11, 12, 16, 240);
        ImU32 borderCol = IM_COL32(38, 40, 56, 230);
        ImU32 titleCol  = IM_COL32(248, 113, 113, 255); // Royal Crimson
        ImU32 sepCol    = IM_COL32(34, 36, 50, 220);

        const float panelW = 185.0f;
        const float rowH   = 24.0f;
        const float headH  = 38.0f;
        const float footH  = 30.0f;
        const float corner = 10.0f;
        const float margin = 12.0f;
        float panelH = headH + items.size() * rowH + 8.0f + footH;

        ImVec2 screen = ImGui::GetIO().DisplaySize;
        if (screen.x > 0 && screen.y > 0) {
            const float edgeMargin = 12.0f;
            int anchor = g_Globals.Misc.StatusPanelAnchor;
            if (anchor < 0) anchor = 0;
            if (anchor > 3) anchor = 3;

            switch (anchor) {
            case 0: // Top
                g_Globals.Misc.StatusPanelX = (screen.x - panelW) * 0.5f;
                g_Globals.Misc.StatusPanelY = edgeMargin;
                break;
            case 1: // Right
                g_Globals.Misc.StatusPanelX = screen.x - panelW - edgeMargin;
                g_Globals.Misc.StatusPanelY = (screen.y - panelH) * 0.5f;
                break;
            case 2: // Left
                g_Globals.Misc.StatusPanelX = edgeMargin;
                g_Globals.Misc.StatusPanelY = (screen.y - panelH) * 0.5f;
                break;
            case 3: // Bottom
            default:
                g_Globals.Misc.StatusPanelX = (screen.x - panelW) * 0.5f;
                g_Globals.Misc.StatusPanelY = screen.y - panelH - edgeMargin;
                break;
            }

            g_Globals.Misc.StatusPanelX = (std::max)(0.0f, (std::min)(g_Globals.Misc.StatusPanelX, screen.x - panelW));
            g_Globals.Misc.StatusPanelY = (std::max)(0.0f, (std::min)(g_Globals.Misc.StatusPanelY, screen.y - panelH));
        }

        ImVec2 pos(g_Globals.Misc.StatusPanelX, g_Globals.Misc.StatusPanelY);
        ImDrawList* dl = ImGui::GetForegroundDrawList();

        // Base frosted glass container
        dl->AddRectFilled(pos, ImVec2(pos.x + panelW, pos.y + panelH), bgColor, corner);
        dl->AddRect(pos, ImVec2(pos.x + panelW, pos.y + panelH), borderCol, corner, 0, 1.0f);

        // Header background
        dl->AddRectFilled(pos, ImVec2(pos.x + panelW, pos.y + headH), IM_COL32(14, 15, 22, 255), corner, ImDrawFlags_RoundCornersTop);
        dl->AddLine(ImVec2(pos.x, pos.y + headH), ImVec2(pos.x + panelW, pos.y + headH), sepCol, 1.0f);

        // Header Title + Icon
        ImGui::PushFont(FontAwesomeSolid);
        const char* kbIcon = ICON_FA_KEYBOARD;
        ImVec2 iconSz = ImGui::CalcTextSize(kbIcon);
        ImGui::PopFont();

        ImGui::PushFont(InterBold);
        const char* kbTitle = "KeyBinds";
        ImVec2 titleSz = ImGui::CalcTextSize(kbTitle);
        float totalTitleW = iconSz.x + 6.0f + titleSz.x;
        float titleStartX = pos.x + (panelW - totalTitleW) * 0.5f;
        float titleCenterY = pos.y + (headH - titleSz.y) * 0.5f;
        ImGui::PopFont();

        ImGui::PushFont(FontAwesomeSolid);
        dl->AddText(ImVec2(titleStartX, pos.y + (headH - iconSz.y) * 0.5f), titleCol, kbIcon);
        ImGui::PopFont();

        ImGui::PushFont(InterBold);
        dl->AddText(ImVec2(titleStartX + iconSz.x + 6.0f, titleCenterY), IM_COL32(240, 242, 248, 255), kbTitle);
        ImGui::PopFont();

        float itemY = pos.y + headH + 6.0f;
        for (const auto& item : items) {
            std::string keyStr = VKName(item.bind);
            
            // Status LED dot
            ImU32 ledCol = item.featureOn ? IM_COL32(16, 185, 129, 255) : IM_COL32(75, 85, 99, 180);
            dl->AddCircleFilled(ImVec2(pos.x + margin + 3.0f, itemY + rowH * 0.5f), 3.0f, ledCol, 12);
            if (item.featureOn) {
                dl->AddCircle(ImVec2(pos.x + margin + 3.0f, itemY + rowH * 0.5f), 4.5f, IM_COL32(16, 185, 129, 80), 12, 1.0f);
            }

            // Feature Name
            ImGui::PushFont(InterRegular);
            ImU32 nameColor = item.featureOn ? IM_COL32(240, 242, 248, 255) : IM_COL32(148, 152, 168, 200);
            dl->AddText(ImVec2(pos.x + margin + 12.0f, itemY + 3.0f), nameColor, item.name.c_str());
            ImGui::PopFont();

            // Keycap Badge on right
            ImGui::PushFont(InterBold12 ? InterBold12 : InterBold);
            ImVec2 keySz = ImGui::CalcTextSize(keyStr.c_str());
            ImVec2 capMin(pos.x + panelW - margin - keySz.x - 8.0f, itemY + 2.0f);
            ImVec2 capMax(pos.x + panelW - margin, itemY + rowH - 2.0f);
            
            dl->AddRectFilled(capMin, capMax, IM_COL32(18, 19, 28, 255), 4.0f);
            ImU32 capBorder = item.featureOn ? IM_COL32(239, 68, 68, 180) : IM_COL32(38, 40, 56, 200);
            dl->AddRect(capMin, capMax, capBorder, 4.0f, 0, 1.0f);
            
            ImU32 keyColor = item.featureOn ? titleCol : IM_COL32(156, 163, 175, 220);
            dl->AddText(ImVec2(capMin.x + 4.0f, itemY + (rowH - keySz.y) * 0.5f - 1.0f), keyColor, keyStr.c_str());
            ImGui::PopFont();

            itemY += rowH;
        }

        // Footer separator
        float footSepY = pos.y + panelH - footH;
        dl->AddLine(ImVec2(pos.x, footSepY), ImVec2(pos.x + panelW, footSepY), sepCol, 1.0f);

        // Footer Enemy Count Badge
        ImGui::PushFont(InterBold12 ? InterBold12 : InterBold);
        std::string numStr = std::to_string(enemyCount);
        std::string prefix = "Enemies : ";
        ImVec2 prefSz = ImGui::CalcTextSize(prefix.c_str());
        ImVec2 numSz  = ImGui::CalcTextSize(numStr.c_str());
        float footX = pos.x + (panelW - (prefSz.x + numSz.x)) * 0.5f;
        float footY = footSepY + (footH - prefSz.y) * 0.5f;
        dl->AddText(ImVec2(footX, footY), IM_COL32(156, 163, 175, 230), prefix.c_str());
        dl->AddText(ImVec2(footX + prefSz.x, footY), titleCol, numStr.c_str());
        ImGui::PopFont();
    }

    void Interface::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) {
                ResizeWidht = (UINT)LOWORD(lParam);
                ResizeHeight = (UINT)HIWORD(lParam);
            }
            break;
        }

        if (bIsMenuOpen) {
            ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
        }
    }

    void Interface::HandleMenuKey()
    {
        auto toggleBoolKey = [](int vk, bool& value, bool& keyDown) {
            if (vk == 0) return;
            if (GetAsyncKeyState(vk) & 0x8000) {
                if (!keyDown) value = !value;
                keyDown = true;
            } else {
                keyDown = false;
            }
        };

        static bool aimbotToggleKeyDown = false;
        toggleBoolKey(g_Globals.AimBot.AimbotToggleKeybind, g_Globals.AimBot.Enabled, aimbotToggleKeyDown);

        static bool rapidFireToggleKeyDown = false;
        toggleBoolKey(g_Globals.AimBot.RapidFireToggleKeybind, g_Globals.AimBot.RapidFire, rapidFireToggleKeyDown);

        static bool fastReloadToggleKeyDown = false;
        toggleBoolKey(g_Globals.AimBot.FastReloadToggleKeybind, g_Globals.AimBot.FastReload, fastReloadToggleKeyDown);

        static bool silentToggleKeyDown = false;
        toggleBoolKey(g_Globals.AimBot.SilentLegitToggleKeybind, g_Globals.AimBot.SilentAimEnabled, silentToggleKeyDown);

        static bool forceAimToggleKeyDown = false;
        toggleBoolKey(g_Globals.General.ForceAimToggleKeybind, g_Globals.General.ForceonAim, forceAimToggleKeyDown);

        static bool mouseToggleKeyDown = false;
        toggleBoolKey(g_Globals.AimBot.MouseAimToggleKeybind, g_Globals.AimBot.MouseAim, mouseToggleKeyDown);

        static bool teleToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.FakeLag_TeleToggleKeybind, g_Globals.Exploits.FakeLag_Tele, teleToggleKeyDown);
        if (g_Globals.Exploits.FakeLag_Tele)
            Features::FakeLag::EnsureHelper();

        static bool freezeToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.FakeLag_FreezeToggleKeybind, g_Globals.Exploits.FakeLag_Freeze, freezeToggleKeyDown);
        if (g_Globals.Exploits.FakeLag_Freeze)
            Features::FakeLag::EnsureHelper();

        static bool ghostToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.FakeLag_GhostToggleKeybind, g_Globals.Exploits.FakeLag_Ghost, ghostToggleKeyDown);
        if (g_Globals.Exploits.FakeLag_Ghost)
            Features::FakeLag::EnsureHelper();

        static bool infinitySkillToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.InfinitySkillToggleKeybind, g_Globals.Exploits.InfinitySkill, infinitySkillToggleKeyDown);

        static bool speedTimerToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.SpeedTimerHackToggleKeybind, g_Globals.Exploits.SpeedTimerHack, speedTimerToggleKeyDown);

        static bool teleportMarkToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.TeleportMarkToggleKeybind, g_Globals.Exploits.TeleportMarkEnabled, teleportMarkToggleKeyDown);

        static bool shakeKillToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.ShakeKillToggleKeybind, g_Globals.Exploits.ShakeKill, shakeKillToggleKeyDown);

        static bool enemyPullToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.EnemyPullToggleKeybind, g_Globals.Exploits.EnemyPullEnabled, enemyPullToggleKeyDown);

        static bool magnetEnemyToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.MagnetEnemyToggleKeybind, g_Globals.Exploits.MagnetEnemyEnabled, magnetEnemyToggleKeyDown);

        static bool divekillToggleKeyDown = false;
        toggleBoolKey(g_Globals.General.divekillToggleKeybind, g_Globals.General.divekill, divekillToggleKeyDown);

        static bool flyHackToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.FlyHackIntToggleKeybind, g_Globals.Exploits.FlyHackActive, flyHackToggleKeyDown);

        static bool kcFlyToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.KcNoGravityFlyToggleKeybind, g_Globals.Exploits.KcNoGravityFly, kcFlyToggleKeyDown);

        static bool refreshEspToggleKeyDown = false;
        static bool prevRefreshEsp = g_Globals.Visuals.RefreshEsp;
        toggleBoolKey(g_Globals.Visuals.RefreshEspToggleKeybind, g_Globals.Visuals.RefreshEsp, refreshEspToggleKeyDown);
        if (g_Globals.Visuals.RefreshEsp && !prevRefreshEsp)
            FWork::Data::Refresh();
        prevRefreshEsp = g_Globals.Visuals.RefreshEsp;

        static bool fixEspToggleKeyDown = false;
        toggleBoolKey(g_Globals.Visuals.FixESPToggleKeybind, g_Globals.Visuals.FixESP, fixEspToggleKeyDown);

        static bool aimbotTypeKeyDown = false;
        if (g_Globals.AimBot.AimbotTypeKeybind != 0 &&
            (GetAsyncKeyState(g_Globals.AimBot.AimbotTypeKeybind) & 0x8000))
        {
            if (!aimbotTypeKeyDown) {
                aimbotTypeKeyDown = true;
                g_Globals.AimBot.AimbotType = (g_Globals.AimBot.AimbotType + 1) % 4;
            }
        } else {
            aimbotTypeKeyDown = false;
        }

        static bool forcePartKeyDown = false;
        if (g_Globals.General.forcePartkeybind != 0 &&
            (GetAsyncKeyState(g_Globals.General.forcePartkeybind) & 0x8000))
        {
            if (!forcePartKeyDown) {
                forcePartKeyDown = true;
                g_Globals.General.FORCEPART = (g_Globals.General.FORCEPART == 0) ? 1 : 0;
            }
        } else {
            forcePartKeyDown = false;
        }

        static bool MenuKeyDown = false;
        if (g_SessionBlocked.load())
        {
            // Expired license: hotkey can never bring the panel back.
            MenuKeyDown = (GetAsyncKeyState(g_Globals.General.MenuKey) & 0x8000) != 0;
            bIsMenuOpen = false;
        }
        else if (GetAsyncKeyState(g_Globals.General.MenuKey) & 0x8000)
        {
            if (!MenuKeyDown)
            {
                MenuKeyDown = true;
                bIsMenuOpen = !bIsMenuOpen;
                if (bIsMenuOpen) {
                    bIsMainMinimized = false; // Reset to full dashboard when opening via hotkey
                }

                UpdateClickThrough();
            }
        }
        else {
            MenuKeyDown = false;
        }
    }

    void Interface::UpdateClickThrough()
    {
        if (bIsMenuOpen) {
            // Check active window based on minimization and authentication state
            const char* targetWindowName = !isAuthenticated ? "LoginWindow" : (bIsMainMinimized ? "FloatingIcon" : "Dashboard");
            ImGuiWindow* window = ImGui::FindWindowByName(targetWindowName);
            if (window && !window->Hidden) {
                POINT mousePos;
                GetCursorPos(&mousePos);
                ScreenToClient(hWindow, &mousePos);

                ImRect rect = window->Rect();
                if (mousePos.x >= rect.Min.x && mousePos.x <= rect.Max.x &&
                    mousePos.y >= rect.Min.y && mousePos.y <= rect.Max.y) {
                    // Inside GUI -> interactable (remove WS_EX_TRANSPARENT, ensure WS_EX_LAYERED)
                    LONG_PTR exStyle = GetWindowLongPtr(hWindow, GWL_EXSTYLE);
                    if (exStyle & WS_EX_TRANSPARENT) {
                        SetWindowLongPtr(hWindow, GWL_EXSTYLE, (exStyle & ~WS_EX_TRANSPARENT) | WS_EX_LAYERED);
                        SetWindowPos(hWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
                    }
                    return;
                }
            }

            // Outside GUI -> transparent/click-through (add WS_EX_TRANSPARENT, ensure WS_EX_LAYERED)
            LONG_PTR exStyle = GetWindowLongPtr(hWindow, GWL_EXSTYLE);
            if (!(exStyle & WS_EX_TRANSPARENT)) {
                SetWindowLongPtr(hWindow, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT | WS_EX_LAYERED);
                SetWindowPos(hWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
            }
        }
        else {
            // Menu closed -> fully transparent/click-through (ensure WS_EX_TRANSPARENT and WS_EX_LAYERED)
            LONG_PTR exStyle = GetWindowLongPtr(hWindow, GWL_EXSTYLE);
            if (!(exStyle & WS_EX_TRANSPARENT)) {
                SetWindowLongPtr(hWindow, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT | WS_EX_LAYERED);
                SetWindowPos(hWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
            }
        }
    }

    void Interface::ShutDown() {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        Overlay::ShutDown();
    }
}
