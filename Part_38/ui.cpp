#include "ui.hpp"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <src/Overlay/Overlay.hpp>
#include <src/Globals.hpp>
#include <src/AuthGuard.hpp>
#include <src/CmdServer/CmdServer.hpp>
#include <src/Fonts/FontInter.hpp>
#include <imgui_internal.h>
#include <src/Fonts/FontAwesome.hpp>
#include <src/Fonts/Fonts.hpp>
#include <thread>
#include <map>
#include <Auth/auth.hpp>
#include "src\Fonts\Fonts.hpp"
#include "src\ui\SidebarLogo.hpp"   // logo sidebar embarque dans la DLL
#include "src\ui\BadgeAvatar.hpp"    // badge ONLINE (tswira AM.png) embarque dans la DLL
#include <FontAwesome6.hpp>
#include "Logo.hpp"
#include <lmcons.h>
#include <EspLines/Memory/NashMem.hpp>
#include <EspLines/RankIcons.hpp>        // badges de rang embarques dans la DLL
#include <sstream>
#include <cstdio>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <src/adb/adb_utils.hpp>
#include <src/aimkill/AimKillLink.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Data/MovementExploits.hpp>
#include <EspLines/Data/Data.hpp>
#include <EspLines/Exploits/Exploit/FakeLagFreeze.hpp>
#include <vector>
#include <vector>
#include <string>

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

// Font des libelles des fonctions/parametres (OurJourney calligraphie)
ImFont* FunctionFont = nullptr;

// Font des TITRES des cartes ("l3anawin dyal lbita9a") : NOYH Heavy,
// c'est-à-dire la police QUI ETAIT dans les fonctions AVANT le passage en Poppins.
ImFont* TitleFont = nullptr;

static bool isAuthenticated = false;

namespace ImGui {
    inline bool ButtonStyled(const char* label, const ImVec2& size_arg, bool disabled = false)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);

        // Font du libelle : la MESURE et le rendu doivent utiliser la meme fonte,
        // sinon le texte n'est PAS centre dans le bouton (label_size mesure en 16
        // alors que le texte est dessine en 12).
        // -> NOYH Heavy 18px (FunctionFont) : boutons plus lisibles.
        ImFont* btnFont = FunctionFont ? FunctionFont : (InterBold12 ? InterBold12 : InterBold);
        if (btnFont) PushFont(btnFont);
        const ImVec2 label_size = CalcTextSize(label, NULL, true);

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size;
        if (size_arg.x == 0 && size_arg.y == 0)
            size = ImVec2(ImGui::GetWindowSize().x - 24, 36);   // bouton plus haut (etait 30)
        else
            size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

        const ImRect bb(pos, pos + size);
        ItemSize(size, style.FramePadding.y);
        if (!ItemAdd(bb, id))
        {
            if (btnFont) PopFont();
            return false;
        }

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
        ImVec4 hoverBg = ImVec4(0.953f, 0.957f, 0.965f, alpha); // Pearl White
        ImVec4 activeBg = ImVec4(0.820f, 0.835f, 0.870f, alpha); // Pearl white dim

        ImVec4 bgCol = disabled ? baseBg : (held ? activeBg : ImLerp(baseBg, hoverBg, it->second * 0.75f));
        ImVec4 borderCol = disabled ? ImVec4(0.18f, 0.19f, 0.25f, 0.4f * alpha) : ImLerp(ImVec4(0.22f, 0.24f, 0.34f, 0.8f * alpha), ImVec4(0.953f, 0.957f, 0.965f, alpha), it->second);

        window->DrawList->AddRectFilled(bb.Min, bb.Max, ImColor(bgCol), 6.0f);
        window->DrawList->AddRect(bb.Min, bb.Max, ImColor(borderCol), 6.0f, 0, 1.0f);

        ImVec4 textCol = disabled ? ImVec4(0.45f, 0.45f, 0.50f, alpha) : (hovered ? ImVec4(1.0f, 1.0f, 1.0f, alpha) : ImVec4(0.92f, 0.93f, 0.96f, alpha));
        // btnFont est deja pousse au debut de la fonction -> mesure et rendu identiques
        RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, ImVec2(0.5f, 0.5f), &bb);
        if (btnFont) PopFont();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
        return pressed;
    }
}

static inline ImVec2 Size = ImVec2(710, 500);   // +50px l3ard (cartes plus larges), 500 ftol

int CurrentTab = 10;

char Username[255] = "";
char Password[255] = "";
char Gmail[255] = "";

//Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;

HWND hwnd = nullptr;

ID3D11ShaderResourceView* Logo = nullptr;
ID3D11ShaderResourceView* SidebarImage = nullptr;
ID3D11ShaderResourceView* BadgeImage = nullptr;

extern HMODULE g_hModule;

// Charge la photo du haut de la sidebar.
// 1) image EMBARQUEE dans la DLL -> marche chez tout le monde.
// 2) secours : fichier image.jpg a cote de la DLL (si present).
static void LoadSidebarImage(ID3D11Device* dev) {
    SidebarImage = nullptr;
    BadgeImage = nullptr;
    if (!dev) return;

    // Badge "ONLINE" : tswira embarquee (pas de fichier necessaire)
    D3DX11CreateShaderResourceViewFromMemory(dev, BadgeAvatarBytes, sizeof(BadgeAvatarBytes), nullptr, nullptr, &BadgeImage, nullptr);

    // 1) Memoire (pas de fichier necessaire -> logo dispo pour tout le monde)
    if (SUCCEEDED(D3DX11CreateShaderResourceViewFromMemory(dev, SidebarLogoBytes, sizeof(SidebarLogoBytes), nullptr, nullptr, &SidebarImage, nullptr)) && SidebarImage)
        return;
    SidebarImage = nullptr;

    // 2) Secours : fichier a cote de la DLL
    std::vector<std::wstring> candidates;

    wchar_t mod[MAX_PATH] = {};
    if (g_hModule && GetModuleFileNameW(g_hModule, mod, MAX_PATH)) {
        std::wstring p(mod);
        size_t slash = p.find_last_of(L"\\");
        if (slash != std::wstring::npos)
            candidates.push_back(p.substr(0, slash + 1) + L"image.jpg");
    }

    for (const std::wstring& p : candidates) {
        if (GetFileAttributesW(p.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
        if (SUCCEEDED(D3DX11CreateShaderResourceViewFromFileW(dev, p.c_str(), nullptr, nullptr, &SidebarImage, nullptr)))
            return;
        SidebarImage = nullptr;
    }
}

// ------------------------------------------------------------------
// PARTICULES DE FOND (Login + ecran Connect)
// Positions normalisees 0..1 -> la fenetre peut changer de taille.
// ------------------------------------------------------------------
namespace {
    struct UiParticle {
        ImVec2 pos;     // 0..1 dans la zone
        ImVec2 vel;     // vitesse (unitees normalees / seconde)
        float  radius;  // rayon en px
        float  alpha;   // alpha de base (0..1)
        float  phase;   // decalage du scintillement
        float  speed;   // vitesse du scintillement
    };
    std::vector<UiParticle> g_UiParticles;
    unsigned s_PartSeed = 20260926u;

    float PartRand() {
        s_PartSeed = s_PartSeed * 1664525u + 1013904223u;
        return (float)((s_PartSeed >> 8) & 0xFFFFFF) / 16777216.0f;
    }
}

// Dessine les particules dans [areaPos .. areaPos + areaSize]
static void DrawBgParticles(ImDrawList* dl, const ImVec2& areaPos, const ImVec2& areaSize, int want)
{
    if (!dl || want <= 0 || areaSize.x <= 1.0f || areaSize.y <= 1.0f) return;

    float dt = ImGui::GetIO().DeltaTime;
    if (dt > 0.05f) dt = 0.05f;
    const float t = (float)ImGui::GetTime();

    while ((int)g_UiParticles.size() < want) {
        UiParticle p;
        p.pos    = ImVec2(PartRand(), PartRand());
        // NEIGE : chute vers le bas, derivee laterale tres lente
        p.vel    = ImVec2((PartRand() - 0.5f) * 0.008f, 0.030f + PartRand() * PartRand() * 0.090f);
        p.radius = 1.2f + PartRand() * PartRand() * 3.0f;   // gros flocons = rares (profondeur)
        p.alpha  = 0.30f + PartRand() * 0.60f;
        p.phase  = PartRand() * 6.2831853f;
        p.speed  = 0.30f + PartRand() * 0.70f;
        g_UiParticles.push_back(p);
    }
    if ((int)g_UiParticles.size() > want)
        g_UiParticles.resize(want);

    for (UiParticle& p : g_UiParticles) {
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;

        if (p.pos.y > 1.02f) { p.pos.y = -0.02f; p.pos.x = PartRand(); }  // repart du haut
        if (p.pos.x < -0.05f)  p.pos.x = 1.05f;
        if (p.pos.x >  1.05f)  p.pos.x = -0.05f;

        // Chaque flocon tombe DROIT, seul, sans se collideur
        float nx = p.pos.x + sinf(t * p.speed + p.phase) * 0.028f;
        if (nx < -0.05f) nx += 1.10f;
        if (nx >  1.05f) nx -= 1.10f;

        // Fade doux pres du haut et du bas de la zone
        float edge = 1.0f;
        if (p.pos.y < 0.15f)       edge = p.pos.y / 0.15f;
        else if (p.pos.y > 0.85f)  edge = (1.0f - p.pos.y) / 0.15f;
        if (edge < 0.0f) edge = 0.0f;
        if (edge > 1.0f) edge = 1.0f;

        // Scintillement tres doux : un flocon = une lumiere
        const float tw = 0.60f + 0.40f * (sinf(t * p.speed + p.phase) * 0.5f + 0.5f);
        const int   a  = (int)(p.alpha * tw * edge * 255.0f);
        if (a <= 0) continue;

        const ImVec2 c(areaPos.x + nx * areaSize.x, areaPos.y + p.pos.y * areaSize.y);

        // gros flocon -> halo + coeur (effet de profondeur)
        if (p.radius > 2.0f)
            dl->AddCircleFilled(c, p.radius * 2.0f, ImColor(255, 255, 255, a / 4), 16);
        dl->AddCircleFilled(c, p.radius, ImColor(255, 255, 255, a), 12);
    }
}

// Point + tangente sur le perimetre d'un rectangle arrondi (s = distance, sens horaire)
static void RoundRectPerimeter(const ImVec2& mn, const ImVec2& mx, float r, float s, ImVec2& pt, ImVec2& tg)
{
    const float w = mx.x - mn.x, h = mx.y - mn.y;
    if (r < 0.0f) r = 0.0f;
    if (r * 2.0f > w) r = w * 0.5f;
    if (r * 2.0f > h) r = h * 0.5f;

    const float L = w - 2.0f * r;    // arete horizontale
    const float V = h - 2.0f * r;    // arete verticale
    const float A = 1.5707963f * r;  // quart de cercle
    const float P = 2.0f * L + 2.0f * V + 4.0f * A;

    if (P <= 0.001f) { pt = mn; tg = ImVec2(1, 0); return; }

    s = fmodf(s, P);
    if (s < 0.0f) s += P;

    if (s < L) { pt = ImVec2(mn.x + r + s, mn.y); tg = ImVec2(1.0f, 0.0f); return; }
    s -= L;

    if (s < A) {                                        // coin haut-droit
        const float th = -1.5707963f + (s / A) * 1.5707963f;
        const float cx = mx.x - r, cy = mn.y + r;
        pt = ImVec2(cx + r * cosf(th), cy + r * sinf(th));
        tg = ImVec2(-sinf(th), cosf(th)); return;
    }
    s -= A;

    if (s < V) { pt = ImVec2(mx.x, mn.y + r + s); tg = ImVec2(0.0f, 1.0f); return; }
    s -= V;

    if (s < A) {                                        // coin bas-droit
        const float th = (s / A) * 1.5707963f;
        const float cx = mx.x - r, cy = mx.y - r;
        pt = ImVec2(cx + r * cosf(th), cy + r * sinf(th));
        tg = ImVec2(-sinf(th), cosf(th)); return;
    }
    s -= A;

    if (s < L) { pt = ImVec2(mx.x - r - s, mx.y); tg = ImVec2(-1.0f, 0.0f); return; }
    s -= L;

    if (s < A) {                                        // coin bas-gauche
        const float th = 1.5707963f + (s / A) * 1.5707963f;
        const float cx = mn.x + r, cy = mx.y - r;
        pt = ImVec2(cx + r * cosf(th), cy + r * sinf(th));
        tg = ImVec2(-sinf(th), cosf(th)); return;
    }
    s -= A;

    if (s < V) { pt = ImVec2(mn.x, mx.y - r - s); tg = ImVec2(0.0f, -1.0f); return; }
    s -= V;

    const float th = 3.1415927f + (s / A) * 1.5707963f;  // coin haut-gauche
    const float cx = mn.x + r, cy = mn.y + r;
    pt = ImVec2(cx + r * cosf(th), cy + r * sinf(th));
    tg = ImVec2(-sinf(th), cosf(th));
}

// Filet blanc qui CIRCULE autour du menu : base + comete lumineuse qui tourne
static void DrawMovingBorder(ImDrawList* dl, const ImVec2& mn, const ImVec2& mx, float rad)
{
    if (!dl) return;

    // base : filet blanc clair (le tour reste bien visible en permanence)
    dl->AddRect(mn, mx, ImColor(255, 255, 255, 85), rad, 0, 1.6f);

    const float w = mx.x - mn.x, h = mx.y - mn.y;
    float r = rad;
    if (r * 2.0f > w) r = w * 0.5f;
    if (r * 2.0f > h) r = h * 0.5f;

    const float L = w - 2.0f * r, V = h - 2.0f * r, A = 1.5707963f * r;
    const float P = 2.0f * L + 2.0f * V + 4.0f * A;
    if (P <= 1.0f) return;

    const float cometLen = P * 0.30f;                          // longueur du trait
    const float head     = fmodf((float)ImGui::GetTime() * (P * 0.30f), P);  // vitesse
    const int   segs     = 40;

    for (int i = 0; i < segs; ++i)
    {
        const float u0 = (float)i / segs;
        const float u1 = (float)(i + 1) / segs;

        ImVec2 p0, t0, p1, t1;
        RoundRectPerimeter(mn, mx, r, head - cometLen * u0, p0, t0);
        RoundRectPerimeter(mn, mx, r, head - cometLen * u1, p1, t1);

        const float k = 1.0f - u0;            // 1 = tete, 0 = queue
        const int   a = (int)(k * k * 245.0f);
        if (a <= 3) continue;

        dl->AddLine(p0, p1, ImColor(255, 255, 255, a), 2.2f + k * 2.6f);
    }

    // pointe eclatante en tete
    ImVec2 hp, ht;
    RoundRectPerimeter(mn, mx, r, head, hp, ht);
    dl->AddCircleFilled(hp, 8.0f, ImColor(255, 255, 255, 60), 20);
    dl->AddCircleFilled(hp, 3.6f, ImColor(255, 255, 255, 255), 16);
}

// ------------------------------------------------------------------
// TRACE SOURIS : un filet blanc qui SUIVI le curseur dans le menu
// ------------------------------------------------------------------
static std::vector<ImVec2> g_MouseTrail;

static void DrawMouseTrail(ImDrawList* dl, const ImVec2& mn, const ImVec2& mx)
{
    if (!dl) return;

    const ImVec2 m = ImGui::GetIO().MousePos;
    const bool inside = (m.x > mn.x && m.x < mx.x && m.y > mn.y && m.y < mx.y);
    if (!inside) { g_MouseTrail.clear(); return; }

    // un point toutes les ~2px de deplacement
    if (g_MouseTrail.empty()) {
        g_MouseTrail.push_back(m);
    } else {
        const float dx = m.x - g_MouseTrail.back().x;
        const float dy = m.y - g_MouseTrail.back().y;
        if (fabsf(dx) + fabsf(dy) > 2.0f) {
            g_MouseTrail.push_back(m);
            if (g_MouseTrail.size() > 48)
                g_MouseTrail.erase(g_MouseTrail.begin());
        } else {
            g_MouseTrail.back() = m;   // reste colle au curseur
        }
    }

    dl->PushClipRect(mn, mx, true);

    // le filet : queue (fonce) -> tete (vif)
    for (size_t i = 1; i < g_MouseTrail.size(); ++i) {
        const float k = (float)i / (float)g_MouseTrail.size();  // 0 = queue, 1 = tete
        const int   a = (int)(k * k * 190.0f);
        if (a <= 4) continue;
        dl->AddLine(g_MouseTrail[i - 1], g_MouseTrail[i], ImColor(255, 255, 255, a), 1.0f + k * 2.2f);
    }

    // POINT MUS SUPPRIME (le curseur reste celui de Windows)

    dl->PopClipRect();
}

// ------------------------------------------------------------------
// PARTICULES "MGADIN" : de tout petits points, SEULS et calmes, au
// milieu du menu. Pas de halo, pas d etoile : juste des points nets
// qui bougent a peine et scintillent doucement.
// ------------------------------------------------------------------
struct LdDot { ImVec2 pos, vel; float radius, alpha, phase, speed; };
static std::vector<LdDot> g_LdDots;

static void DrawMenuDust(ImDrawList* dl, const ImVec2& areaPos, const ImVec2& areaSize, int want)
{
    if (!dl || want <= 0 || areaSize.x < 60.0f || areaSize.y < 60.0f) return;

    float dt = ImGui::GetIO().DeltaTime;
    if (dt > 0.05f) dt = 0.05f;
    const float t = (float)ImGui::GetTime();

    while ((int)g_LdDots.size() < want) {
        LdDot d;
        d.pos    = ImVec2(0.03f + PartRand() * 0.94f, 0.03f + PartRand() * 0.94f);
        d.vel    = ImVec2((PartRand() - 0.5f) * 0.006f, (PartRand() - 0.5f) * 0.005f);
        d.radius = 0.65f + PartRand() * 0.95f;
        d.alpha  = 0.14f + PartRand() * 0.30f;
        d.phase  = PartRand() * 6.2831853f;
        d.speed  = 0.22f + PartRand() * 0.55f;
        g_LdDots.push_back(d);
    }
    if ((int)g_LdDots.size() > want) g_LdDots.resize(want);

    dl->PushClipRect(areaPos, ImVec2(areaPos.x + areaSize.x, areaPos.y + areaSize.y), true);

    for (LdDot& d : g_LdDots) {
        d.pos.x += d.vel.x * dt;
        d.pos.y += d.vel.y * dt;

        if (d.pos.x < 0.03f) { d.pos.x = 0.03f; d.vel.x =  fabsf(d.vel.x); }
        if (d.pos.x > 0.97f) { d.pos.x = 0.97f; d.vel.x = -fabsf(d.vel.x); }
        if (d.pos.y < 0.03f) { d.pos.y = 0.03f; d.vel.y =  fabsf(d.vel.y); }
        if (d.pos.y > 0.97f) { d.pos.y = 0.97f; d.vel.y = -fabsf(d.vel.y); }

        // elle bouge a peine : elle reste en place
        const float nx = d.pos.x + cosf(t * d.speed * 0.55f + d.phase) * 0.006f;
        const float ny = d.pos.y + sinf(t * d.speed * 0.45f + d.phase) * 0.008f;

        const float tw = 0.55f + 0.45f * sinf(t * d.speed + d.phase);
        const int   a  = (int)(d.alpha * tw * 255.0f);
        if (a <= 5) continue;

        const ImVec2 c(areaPos.x + nx * areaSize.x, areaPos.y + ny * areaSize.y);

        // UN SEUL point, net : pas de halo, pas d etoile
        dl->AddCircleFilled(c, d.radius, ImColor(255, 255, 255, a), 10);
    }

    dl->PopClipRect();
}

// ------------------------------------------------------------------
// Slider compact : label + valeur au dessus, piste fine en dessous
// ------------------------------------------------------------------
static bool DrawCompactSliderFloat(const char* label, float* value, float minValue, float maxValue)
{
    bool changed = false;
    ImGui::PushID(label);
    char valueText[32];
    snprintf(valueText, sizeof(valueText), "%.2f", *value);
    for (char* p = valueText; *p; ++p)
        if (*p == '.') *p = ',';
    ImGui::Text("%s: %s", label, valueText);
    const float width = ImGui::GetContentRegionAvail().x;
    const ImVec2 size(width, 14.0f);
    const ImVec2 itemMin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##compact_slider", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    const float left = itemMin.x + 10.0f;
    const float right = itemMin.x + size.x - 10.0f;
    const float centerY = itemMin.y + size.y * 0.5f;
    const float range = maxValue - minValue;
    float t = range > 0.0f ? (*value - minValue) / range : 0.0f;
    if ((hovered || active) && ImGui::GetIO().MouseDown[0])
    {
        t = ImClamp((ImGui::GetIO().MousePos.x - left) / (right - left), 0.0f, 1.0f);
        const float nextValue = minValue + t * range;
        if (nextValue != *value)
        {
            *value = nextValue;
            changed = true;
        }
    }
    t = range > 0.0f ? (*value - minValue) / range : 0.0f;
    const float knobX = left + (right - left) * ImClamp(t, 0.0f, 1.0f);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImColor track(28, 26, 34, static_cast<int>(255.0f * ImGui::GetStyle().Alpha));
    const ImColor fill(243, 244, 246, static_cast<int>(255.0f * ImGui::GetStyle().Alpha)); // pearl white
    drawList->AddLine(ImVec2(left, centerY), ImVec2(right, centerY), track, 2.0f);
    drawList->AddLine(ImVec2(left, centerY), ImVec2(knobX, centerY), fill, 2.0f);
    drawList->AddCircleFilled(ImVec2(knobX, centerY), active ? 4.0f : 3.0f, fill, 24);
    ImGui::PopID();
    return changed;
}

// ------------------------------------------------------------------
// Libelle de fonction : 1re lettre PLUS GROSSE, reste plus petit
// ------------------------------------------------------------------
static void DrawFunctionTitle(ImDrawList* dl, const ImVec2& pos, float boxH,
                              const char* text, float bigSize, float smallSize, ImU32 col)
{
    if (!dl || !text || !*text) return;
    ImFont* font = InterBold ? InterBold : ImGui::GetFont();
    if (!font) return;

    char first[2] = { text[0], 0 };
    const char* rest = text + 1;

    const ImVec2 fs = font->CalcTextSizeA(bigSize, FLT_MAX, 0.0f, first);
    const bool  hasRest = (rest && *rest);
    const ImVec2 rs = hasRest ? font->CalcTextSizeA(smallSize, FLT_MAX, 0.0f, rest) : ImVec2(0, 0);
    (void)rs;

    const float y0 = pos.y + (boxH - bigSize) * 0.5f;
    dl->AddText(font, bigSize, ImVec2(pos.x, y0), col, first);

    if (hasRest) {
        // meme ligne de base : le decalage est proportionnel a la taille du glyphe
        const float baseline = (font->FontSize > 0.0f) ? (font->Ascent / font->FontSize) : 0.8f;
        const float y1 = y0 + baseline * (bigSize - smallSize);
        dl->AddText(font, smallSize, ImVec2(pos.x + fs.x, y1), col, rest);
    }
}

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

        // Badges de rang : les 36 PNG sont embarques dans la DLL (aucun fichier externe)
        RankIcons::SetDevice(Device);

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

        // ------------------------------------------------------------------
        // Font des LIBELLES DE FONCTIONS : NOYH Heavy 20px
        //  = la meme police ET la meme taille que les TITRES DES CARTES
        //    (retour "kifma kan" : le retrecissement concerne la LIGNE).
        // Cree par Fonts::Initialize() (Fonts::NoyhFunctions) avec
        // GlyphExtraSpacing.x = 1.0px -> texte plus large ("zid fl3ard").
        // Filet de securite : NOYH Heavy 20px (Fonts::NoyhMenu).
        // ------------------------------------------------------------------
        FunctionFont = nullptr;

        Fonts::Initialize(IDevice);
        FunctionFont = Fonts::NoyhFunctions ? Fonts::NoyhFunctions
                     : (Fonts::NoyhMenu ? Fonts::NoyhMenu : FunctionFont);

        // Titres des cartes : NOYH Heavy = la police qui ETAIT dans les fonctions
        // avant le passage en Poppins (les lignes de fonctions restent en Poppins).
        TitleFont = Fonts::NoyhMenu ? Fonts::NoyhMenu : FunctionFont;
        Logo = nullptr;
        if (g_pd3dDevice) {
            HRESULT hr = D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, LogoBytes, sizeof(LogoBytes), NULL, NULL, &Logo, NULL);
            if (FAILED(hr)) {
                Logo = nullptr;
            }
        }

        LoadSidebarImage(g_pd3dDevice);

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
        Style->FramePadding = ImVec2(10, 7);    // champs (combos / inputs) plus hauts
        Style->ItemSpacing = ImVec2(8, 16);     // MASAFAT bayn function o function
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

        // --- THEME "Editorial X - Core" : BLANC + K7L uniquement (aucun bleu) ---
        const ImVec4 accent = ImVec4(1.000f, 1.000f, 1.000f, 1.0f);                 // blanc
        const ImVec4 accentHover = ImVec4(0.920f, 0.930f, 0.950f, 1.0f);            // blanc cassé
        const ImVec4 accentActive = ImVec4(0.740f, 0.760f, 0.800f, 1.0f);           // gris clair

        Style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.050f, 0.050f, 0.050f, 1.00f);        // piste noire
        Style->Colors[ImGuiCol_ScrollbarGrab] = accent;                              // barre blanche
        Style->Colors[ImGuiCol_ScrollbarGrabHovered] = accentHover;
        Style->Colors[ImGuiCol_ScrollbarGrabActive] = accentActive;

        // --- Obsidian & Pearl Theme (byd / k7l) ---
        Style->Colors[ImGuiCol_WindowBg] = ImVec4(0.043f, 0.043f, 0.043f, 0.98f);     // noir
        Style->Colors[ImGuiCol_ChildBg] = ImVec4(0.051f, 0.051f, 0.051f, 0.96f);      // carte noire
        Style->Colors[ImGuiCol_PopupBg] = ImVec4(0.070f, 0.070f, 0.070f, 0.98f);      // popup noire
        Style->Colors[ImGuiCol_Border] = ImVec4(0.900f, 0.920f, 0.950f, 0.16f);       // FILET BLANC discret
        Style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        Style->Colors[ImGuiCol_Text] = ImVec4(0.953f, 0.957f, 0.965f, 1.00f);         // blanc
        Style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.450f, 0.450f, 0.460f, 1.00f); // gris
        Style->Colors[ImGuiCol_FrameBg] = ImVec4(0.075f, 0.075f, 0.078f, 1.00f);      // champs (inputs / combos)
        Style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.115f, 0.115f, 0.118f, 1.00f);
        Style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.155f, 0.155f, 0.160f, 1.00f);
        Style->Colors[ImGuiCol_Button] = ImVec4(0.090f, 0.090f, 0.094f, 1.00f);       // boutons noirs
        Style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.170f, 0.170f, 0.176f, 1.00f);
        Style->Colors[ImGuiCol_ButtonActive] = accent;
        Style->Colors[ImGuiCol_CheckMark] = accent;
        Style->Colors[ImGuiCol_SliderGrab] = accent;
        Style->Colors[ImGuiCol_SliderGrabActive] = accentHover;
        Style->Colors[ImGuiCol_Header] = ImVec4(0.110f, 0.110f, 0.115f, 1.00f);       // selection
        Style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.160f, 0.160f, 0.166f, 1.00f);
        Style->Colors[ImGuiCol_HeaderActive] = accent;
        Style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.900f, 0.920f, 0.950f, 0.30f);     // selection blanche
        Style->Colors[ImGuiCol_Separator] = ImVec4(0.900f, 0.920f, 0.950f, 0.14f);          // FILETS BLANCS
        Style->Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.900f, 0.920f, 0.950f, 0.30f);
        Style->Colors[ImGuiCol_SeparatorActive] = ImVec4(0.900f, 0.920f, 0.950f, 0.50f);
        Style->Colors[ImGuiCol_DragDropTarget] = ImVec4(1.000f, 1.000f, 1.000f, 0.90f);
        Style->Colors[ImGuiCol_NavHighlight] = ImVec4(1.000f, 1.000f, 1.000f, 0.70f);
        Style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.100f, 0.100f, 0.100f, 0.85f);
        Style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.200f, 0.200f, 0.200f, 0.90f);
        Style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.350f, 0.350f, 0.350f, 1.00f);
        Style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.070f, 0.070f, 0.070f, 1.00f);
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

        if (bIsMenuOpen) {

            // Authentication variables
            static char username[64] = "";
            static char password[64] = "";
            static char licenseKey[64] = "";
            static bool loginInProgress = false;
            static bool loginSuccess = false;
            static bool auth_error = false;
            static std::string error_msg = "";
            static std::string KeyAuth_message = "";
            static double loginStartT = -1.0;   // heure du clic SECURE LOGIN (loading post-login)
            static float  loadProgress = 0.0f;  // barre de progression (lisse)
            static float  loadFullT = -1.0;     // instant ou la barre a atteint 100%

            // If not authenticated, show login page
            if (!isAuthenticated) {
                static bool showRegister = false;

                // LOADING SCREEN : login (user/pass) -> menu Connect
                const bool showLoadingScreen = loginInProgress || (loginStartT >= 0.0 && loginSuccess);

                // Match menu size dynamically
                float winHeight = showRegister ? 370.0f : 310.0f;
                if (showLoadingScreen) winHeight = 460.0f;
                ImGui::SetNextWindowSize(ImVec2(showLoadingScreen ? 520.0f : 360.0f, winHeight));
                ImGui::Begin("LoginWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                {
                    ImDrawList* DrawList = ImGui::GetWindowDrawList();
                    ImVec2 Pos = ImGui::GetWindowPos();
                    ImVec2 Size = ImGui::GetWindowSize();

                    // Main body background - Obsidian Glass
                    DrawList->AddRectFilled(Pos, Pos + Size, ImColor(11, 12, 16, 255), 12.0f);
                    DrawMovingBorder(DrawList, Pos, Pos + Size, 12.0f);

                    // Filet qui SUIVI le curseur dans le menu
                    DrawMouseTrail(ImGui::GetForegroundDrawList(), Pos, Pos + Size);

                    // Top branding header bar
                    float headerHeight = 38.0f;
                    DrawList->AddRectFilled(Pos, Pos + ImVec2(Size.x, headerHeight), ImColor(14, 15, 22, 255), 12.0f, ImDrawFlags_RoundCornersTop);
                    DrawList->AddLine(Pos + ImVec2(0, headerHeight), Pos + ImVec2(Size.x, headerHeight), ImColor(38, 40, 56, 255));

                    // Particules de fond
                    DrawBgParticles(DrawList, Pos + ImVec2(0, headerHeight), ImVec2(Size.x, Size.y - headerHeight), 0);   // 0 = particules retirees

                    // Pastille BLANCHE "Editorial X - Core" + "AUTHENTICATION" a cote
                    const float hdrMidY = Pos.y + headerHeight * 0.5f;
                    {
                        ImGui::PushFont(InterBold);
                        const char* brandTxt = "0x Void - Core";
                        ImVec2 brandSz = ImGui::CalcTextSize(brandTxt);
                        ImGui::PopFont();

                        const float dotR  = 3.6f;
                        const float padX  = 11.0f;
                        const float gap   = 7.0f;
                        const float pillH = 24.0f;
                        const float pillW = padX + dotR * 2.0f + gap + brandSz.x + padX;
                        const float pillX = Pos.x + 14.0f;
                        const float pillY = hdrMidY - pillH * 0.5f;

                        // pastille blanche (pearl white) + texte noir
                        DrawList->AddRectFilled(ImVec2(pillX, pillY), ImVec2(pillX + pillW, pillY + pillH),
                                                ImColor(255, 255, 255, 255), pillH * 0.5f);
                        DrawList->AddCircleFilled(ImVec2(pillX + padX + dotR, hdrMidY), dotR,
                                                  ImColor(16, 17, 22, 255), 12);
                        ImGui::PushFont(InterBold);
                        DrawList->AddText(ImVec2(pillX + padX + dotR * 2.0f + gap, hdrMidY - brandSz.y * 0.5f),
                                          ImColor(16, 17, 22, 255), brandTxt);
                        ImGui::PopFont();

                        // a cote : point blanc + AUTHENTICATION en blanc (jiha LIMNA)
                        const char* authTxt = showLoadingScreen ? "AUTHENTICATING" : "AUTHENTICATION";
                        ImGui::PushFont(InterBold);
                        ImVec2 authSz = ImGui::CalcTextSize(authTxt);
                        ImGui::PopFont();

                        const float authTextX = Pos.x + Size.x - 16.0f - authSz.x;  // mlah f limn
                        const float authX     = authTextX - 15.0f;                   // point 9bel lkhout
                        DrawList->AddCircleFilled(ImVec2(authX + 4.5f, hdrMidY), 4.5f, ImColor(255, 255, 255, 70), 16);
                        DrawList->AddCircleFilled(ImVec2(authX + 4.5f, hdrMidY), 3.2f, ImColor(255, 255, 255, 255), 12);
                        ImGui::PushFont(InterBold);
                        DrawList->AddText(ImVec2(authTextX, hdrMidY - authSz.y * 0.5f),
                                          ImColor(255, 255, 255, 255), authTxt);
                        ImGui::PopFont();
                    }

                    // ============================================================
                    //  LOADING SCREEN   LOGIN (user/pass)  ->  menu CONNECT
                    // ============================================================
                    if (showLoadingScreen)
                    {
                        const float t = (loginStartT >= 0.0) ? (float)(ImGui::GetTime() - loginStartT) : 0.0f;
                        const bool authDone = !loginInProgress && loginSuccess;
                        const Globals::InitState st = g_Globals.General.State.load();
                        const bool initReady = (st == Globals::InitState::ReadyToScan ||
                                                st == Globals::InitState::Completed ||
                                                st == Globals::InitState::Failed);

                        // duree totale du loading : la barre fait 0% -> 100% (a changer ici)
                        const float duration = 10.0f;
                        const float prog = ImSaturate(t / duration);      // 0 -> 1 lineairement

                        // --- les 4 etapes (calrees sur la barre) ---
                        const bool step0 = prog >= 0.20f;
                        const bool step1 = authDone && prog >= 0.40f;
                        const bool step2 = authDone && prog >= 0.60f;
                        const bool step3 = authDone && initReady && prog >= 0.85f;

                        // barre : 0% -> 99% sur "duration", puis 100% quand tout est pret
                        float target = ImMin(prog, 0.99f);
                        if (t >= duration && authDone && initReady) target = 1.0f;
                        loadProgress = ImLerp(loadProgress, target, 0.10f);

                        const float cx = Pos.x + Size.x * 0.5f;
                        const float cy = Pos.y + 126.0f;

                        auto Arc = [&](float r, float aStart, float aEnd, float thick, ImU32 col, int seg) {
                            DrawList->PathClear();
                            for (int i = 0; i <= seg; i++) {
                                float ang = aStart + (aEnd - aStart) * ((float)i / (float)seg);
                                DrawList->PathLineTo(ImVec2(cx + cosf(ang) * r, cy + sinf(ang) * r));
                            }
                            DrawList->PathStroke(col, false, thick);
                        };

                        // (fond circulaire derriere le logo SUPPRIME : transparent)
                        {
                            float pulse = fmodf(t, 1.6f) / 1.6f;
                            DrawList->AddCircle(ImVec2(cx, cy), 70.0f + pulse * 34.0f,
                                                ImColor(255, 255, 255, (int)(70.0f * (1.0f - pulse))), 64, 1.5f);
                        }

                        // piste
                        DrawList->AddCircle(ImVec2(cx, cy), 70.0f, ImColor(255, 255, 255, 45), 72, 1.2f);

                        // ================= LOGO Editorial X - Core (a la place de l'ancienne da2ira) =================
                        {
                            const float logoSize = 108.0f;
                            const ImVec2 lmin(cx - logoSize * 0.5f, cy - logoSize * 0.5f);
                            const ImVec2 lmax(cx + logoSize * 0.5f, cy + logoSize * 0.5f);

                            // (halo derriere le logo SUPPRIME : fond 100% transparent)

                            if (SidebarImage) {
                                // crop rond -> badge circulaire
                                DrawList->AddImageRounded(SidebarImage, lmin, lmax,
                                                          ImVec2(0, 0), ImVec2(1, 1),
                                                          IM_COL32_WHITE, logoSize * 0.5f,
                                                          ImDrawFlags_RoundCornersAll);
                            } else {
                                DrawList->AddCircleFilled(ImVec2(cx, cy), logoSize * 0.5f,
                                                          ImColor(255, 255, 255, 30), 64);
                            }
                        }

                        // arc rotatif + comete autour du logo
                        const float angMain = t * 2.2f;
                        Arc(70.0f, angMain + IM_PI * 1.55f, angMain + IM_PI * 2.0f, 4.0f, ImColor(255, 255, 255, 70), 24);
                        Arc(70.0f, angMain, angMain + IM_PI * 1.55f, 4.5f, ImColor(255, 255, 255, 255), 56);
                        {
                            ImVec2 head(cx + cosf(angMain) * 70.0f, cy + sinf(angMain) * 70.0f);
                            DrawList->AddCircleFilled(head, 11.0f, ImColor(255, 255, 255, 40), 24);
                            DrawList->AddCircleFilled(head, 5.0f, ImColor(255, 255, 255, 255), 24);
                        }

                        // titre
                        ImGui::PushFont(InterExtraBold ? InterExtraBold : InterBold);
                        {
                            const char* title = "0x Void - Core";
                            ImVec2 sz = ImGui::CalcTextSize(title);
                            DrawList->AddText(ImVec2(cx - sz.x * 0.5f, Pos.y + 210.0f), ImColor(255, 255, 255, 255), title);
                        }
                        ImGui::PopFont();

                        // statut + points animes
                        {
                            std::string status;
                            if (!authDone)       status = (prog < 0.20f) ? "Establishing secure channel" : "Verifying credentials";
                            else if (prog < 0.40f) status = "Verifying credentials";
                            else if (prog < 0.60f) status = "Account verified";
                            else if (prog < 0.85f) status = "Building session";
                            else if (!initReady) status = g_Globals.General.StatusMessage.empty() ? "Loading system modules" : g_Globals.General.StatusMessage;
                            else                 status = "System ready";
                            int dots = (int)(fmodf(t, 0.9f) / 0.9f * 4.0f) % 4 + 1;
                            status.append((size_t)dots, '.');

                            ImGui::PushFont(InterBold);
                            ImVec2 sz = ImGui::CalcTextSize(status.c_str());
                            DrawList->AddText(ImVec2(cx - sz.x * 0.5f, Pos.y + 244.0f), ImColor(198, 201, 212, 235), status.c_str());
                            ImGui::PopFont();
                        }

                        // liste des etapes
                        {
                            const char* labels[4] = { "Secure channel", "Account verification", "Session build", "System modules" };
                            const bool  stepDone[4] = { step0, step1, step2, step3 };
                            int current = -1;
                            for (int i = 0; i < 4; i++) { if (!stepDone[i]) { current = i; break; } }

                            ImGui::PushFont(InterBold);
                            const float rowH = 20.0f;
                            const float y0 = Pos.y + 274.0f;
                            for (int i = 0; i < 4; i++) {
                                ImVec2 tsz = ImGui::CalcTextSize(labels[i]);
                                const float rowW = 12.0f + 7.0f + tsz.x;
                                const float rx = cx - rowW * 0.5f;
                                const float ry = y0 + i * rowH + 10.0f;
                                const ImVec2 mk(rx + 6.0f, ry);

                                if (stepDone[i]) {
                                    DrawList->AddCircleFilled(mk, 6.0f, ImColor(16, 185, 129, 235), 16);
                                    DrawList->AddLine(ImVec2(mk.x - 3.0f, mk.y + 0.2f), ImVec2(mk.x - 1.0f, mk.y + 2.3f), ImColor(255, 255, 255, 255), 1.7f);
                                    DrawList->AddLine(ImVec2(mk.x - 1.0f, mk.y + 2.3f), ImVec2(mk.x + 3.3f, mk.y - 2.5f), ImColor(255, 255, 255, 255), 1.7f);
                                }
                                else if (i == current) {
                                    const float arcStart = t * 5.0f;
                                    DrawList->PathClear();
                                    for (int k = 0; k <= 16; k++) {
                                        float ang = arcStart + ((float)k / 16.0f) * (IM_PI * 1.4f);
                                        DrawList->PathLineTo(ImVec2(mk.x + cosf(ang) * 5.5f, mk.y + sinf(ang) * 5.5f));
                                    }
                                    DrawList->PathStroke(ImColor(255, 255, 255, 255), false, 1.8f);
                                }
                                else {
                                    DrawList->AddCircle(mk, 5.5f, ImColor(255, 255, 255, 55), 16, 1.2f);
                                }

                                const ImU32 col = stepDone[i] ? ImColor(233, 236, 243, 245)
                                                             : (i == current ? ImColor(255, 255, 255, 255)
                                                                            : ImColor(128, 131, 143, 190));
                                DrawList->AddText(ImVec2(rx + 19.0f, ry - tsz.y * 0.5f), col, labels[i]);
                            }
                            ImGui::PopFont();
                        }

                        // barre de progression + pourcentage
                        {
                            const float barW = Size.x - 170.0f;
                            const float barH = 8.0f;
                            const float bx = Pos.x + (Size.x - barW) * 0.5f;
                            const float by = Pos.y + 374.0f;
                            DrawList->AddRectFilled(ImVec2(bx, by), ImVec2(bx + barW, by + barH),
                                                    ImColor(255, 255, 255, 22), barH * 0.5f);
                            const float fw = barW * ImSaturate(loadProgress);
                            if (fw > 2.0f) {
                                DrawList->AddRectFilled(ImVec2(bx, by), ImVec2(bx + fw, by + barH),
                                                        ImColor(255, 255, 255, 255), barH * 0.5f);
                                const float sh = fmodf(t * 0.75f, 1.0f) * (barW + 32.0f) - 16.0f;
                                const float sh0 = bx + ImClamp(sh - 16.0f, 0.0f, fw);
                                const float sh1 = bx + ImClamp(sh, 0.0f, fw);
                                if (sh1 > sh0 + 0.5f)
                                    DrawList->AddRectFilled(ImVec2(sh0, by), ImVec2(sh1, by + barH),
                                                            ImColor(255, 255, 255, 110), barH * 0.5f);
                            }

                            char pct[8];
                            ImFormatString(pct, IM_ARRAYSIZE(pct), "%d%%", (int)(ImSaturate(loadProgress) * 100.0f + 0.5f));
                            ImGui::PushFont(InterBold);
                            const float fh = ImGui::GetFontSize();
                            DrawList->AddText(ImVec2(bx + barW + 12.0f, by + (barH - fh) * 0.5f),
                                              ImColor(255, 255, 255, 210), pct);
                            ImGui::PopFont();
                        }

                        // conseil
                        {
                            const char* hint = "Keep the game open while loading";
                            ImGui::PushFont(InterBold);
                            ImVec2 hsz = ImGui::CalcTextSize(hint);
                            DrawList->AddText(ImVec2(cx - hsz.x * 0.5f, Pos.y + 402.0f), ImColor(107, 114, 128, 150), hint);
                            ImGui::PopFont();
                        }

                        // pied de page
                        {
                            const char* cp = "Developed By @4slash";
                            ImGui::PushFont(InterBold12);
                            ImVec2 csz = ImGui::CalcTextSize(cp);
                            DrawList->AddText(ImVec2(cx - csz.x * 0.5f, Pos.y + Size.y - 26.0f),
                                              ImColor(107, 114, 128, 140), cp);
                            ImGui::PopFont();
                        }

                        // ---- passage vers le menu CONNECT ----
                        if (!loginInProgress && loginSuccess)
                        {
                            // la barre a atteint 100% -> on l'affiche 0.7s avant de passer au menu
                            if (loadProgress >= 0.995f && loadFullT < 0.0f)
                                loadFullT = (float)ImGui::GetTime();

                            const bool shown100 = (loadFullT >= 0.0f) &&
                                                  (((float)ImGui::GetTime() - loadFullT) >= 0.7f);

                            if (shown100 || t >= 30.0f)   // 30s = garde-fou si init bloque
                            {
                                CurrentTab = initReady ? 1 : 10;

                                // ====== ANTI-CRACK : session debloquee + heartbeat ======
                                AuthGuard::Unlock();
                                static bool hbStarted = false;
                                if (!hbStarted)
                                {
                                    hbStarted = true;
                                    const char* hbUser = username;
                                    const char* hbPass = password;
                                    std::thread([hbUser, hbPass]() {
                                        int fails = 0, cycle = 0;
                                        while (!g_Globals.General.ShutDown)
                                        {
                                            for (int s = 0; s < 90 && !g_Globals.General.ShutDown; ++s)
                                                std::this_thread::sleep_for(std::chrono::seconds(1));
                                            if (g_Globals.General.ShutDown) break;
                                            if (!isAuthenticated) break;

                                            // 1) la session KeyAuth doit rester valide
                                            g_Api.setup();
                                            if (!g_Api.response.success)
                                            {
                                                if (++fails >= 3) break;      // 3 echecs d'affilee
                                                continue;
                                            }
                                            fails = 0;

                                            // 2) ~toutes les 15 min : re-login complet (abo + HWID)
                                            if ((++cycle % 10) == 0)
                                            {
                                                g_Api.Login(std::string(hbUser), std::string(hbPass));
                                                if (!g_Api.response.success) break;
                                            }
                                        }
                                        // perdu -> plus de session, panel se decharge
                                        if (!g_Globals.General.ShutDown && isAuthenticated)
                                        {
                                            AuthGuard::Lock("server heartbeat");
                                            isAuthenticated = false;
                                            g_Globals.General.ShutDown = true;
                                        }
                                    }).detach();
                                }
                                isAuthenticated = true;
                            }
                        }

                        ImGui::End();
                        return;
                    }

                    // Bouton "-" supprime de tous les menus

                    // --- Clean Tab Selector ---
                    float tabY = 50.0f;
                    float tabWidth = Size.x;   // REGISTER retire : LOGIN seul, pleine largeur
                    
                    auto DrawTab = [&](const char* text, bool active, float x_offset) {
                        ImVec2 tabPos = Pos + ImVec2(x_offset, tabY);
                        ImRect tabRect(tabPos, tabPos + ImVec2(tabWidth, 30.0f));
                        bool hovered = false;
                        bool pressed = ImGui::ButtonBehavior(tabRect, ImGui::GetID(text), &hovered, nullptr);
                        
                        if (active) {
                            // Pearl white underline
                            DrawList->AddRectFilled(tabPos + ImVec2(16, 27), tabPos + ImVec2(tabWidth - 16, 30), ImColor(255, 255, 255, 255), 1.5f);
                        }
                        
                        ImGui::PushFont(InterBold);
                        ImVec2 textSize = ImGui::CalcTextSize(text);
                        ImVec2 textPos = tabPos + ImVec2((tabWidth - textSize.x) * 0.5f, (26.0f - textSize.y) * 0.5f);
                        DrawList->AddText(textPos, active ? ImColor(255, 255, 255, 255) : (hovered ? ImColor(200, 200, 205, 255) : ImColor(120, 120, 130, 180)), text);
                        ImGui::PopFont();
                        
                        return pressed;
                    };

                    // Onglet REGISTER supprime du menu
                    showRegister = false;
                    if (DrawTab("LOGIN", true, 0.0f)) {
                        auth_error = false;
                        error_msg.clear();
                    }

                    // Separator under tabs
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
                    ImGui::PushStyleColor(ImGuiCol_Text, ImColor(255, 255, 255, 255).Value);
                    ImGui::PushStyleColor(ImGuiCol_Border, ImColor(38, 40, 56, 255).Value);

                    // --- Username Input ---
                    ImGui::SetCursorPos(ImVec2(centerX, startY));
                    ImGui::PushItemWidth(inputWidth);
                    ImGui::InputTextWithHint("##Username", "Username", username, IM_ARRAYSIZE(username));
                    {
                        bool active = ImGui::IsItemActive();
                        bool hovered = ImGui::IsItemHovered();
                        ImU32 borderColor = active ? ImColor(255, 255, 255, 255) : (hovered ? ImColor(255, 255, 255, 120) : ImColor(38, 40, 56, 255));
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
                        ImU32 borderColor = active ? ImColor(255, 255, 255, 255) : (hovered ? ImColor(255, 255, 255, 120) : ImColor(38, 40, 56, 255));
                        DrawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), borderColor, 6.0f, 0, 1.0f);
                    }
                    ImGui::PopItemWidth();

                    // --- License Key Input (Register only) ---
                    if (showRegister) {
                        startY += inputHeight + spacingY;
                        ImGui::SetCursorPos(ImVec2(centerX, startY));
                        ImGui::PushItemWidth(inputWidth);
                        ImGui::InputTextWithHint("##LicenseKey", "License Key (KC-XXXXX)", licenseKey, IM_ARRAYSIZE(licenseKey));
                        {
                            bool active = ImGui::IsItemActive();
                            bool hovered = ImGui::IsItemHovered();
                            ImU32 borderColor = active ? ImColor(255, 255, 255, 255) : (hovered ? ImColor(255, 255, 255, 120) : ImColor(38, 40, 56, 255));
                            DrawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), borderColor, 6.0f, 0, 1.0f);
                        }
                        ImGui::PopItemWidth();
                    }

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
                    float buttonWidth = 200.0f;
                    float buttonHeight = 30.0f;
                    float buttonY = winHeight - 88.0f;   // remonte un peu par rapport au bas
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
                        // Pearl white buttons
                        ImColor btnColor = ImColor(255, 255, 255);
                        ImColor btnHover = ImColor(255, 255, 255);
                        ImColor btnActive = ImColor(209, 213, 219);

                        ImGui::PushStyleColor(ImGuiCol_Button, btnColor.Value);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnHover.Value);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnActive.Value);

                        const char* btnLabel = showRegister ? "REGISTER LICENSE" : "SECURE LOGIN";
                        if (ImGui::ButtonStyled(btnLabel, ImVec2(buttonWidth, buttonHeight))) {
                            loginInProgress = true;
                            auth_error = false;
                            error_msg.clear();
                            loginStartT = ImGui::GetTime();   // lance l'ecran de chargement
                            loadProgress = 0.0f;
                            loadFullT = -1.0;

                            std::thread([&]() {
                                std::string setupResult = g_Api.setup();
                                bool initOk = g_Api.response.success;
                                KeyAuth_message = g_Api.response.message;

                                if (initOk) {
                                    std::string authResult;
                                    if (showRegister) {
                                        authResult = g_Api.Register_key(std::string(username), std::string(password), std::string(licenseKey));
                                    } else {
                                        authResult = g_Api.Login(std::string(username), std::string(password));
                                    }

                                    loginSuccess = g_Api.response.success && (authResult == "success");
                                    KeyAuth_message = g_Api.response.message;

                                    if (!loginSuccess) {
                                        auth_error = true;
                                        error_msg = !KeyAuth_message.empty() ? KeyAuth_message : "Authentication failed.";
                                    }
                                    // SUCCES : ne PAS ouvrir le menu ici.
                                    // L'ecran de chargement (0% -> 100%) est le seul a faire
                                    // isAuthenticated = true quand la barre atteint 100%.
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
                    const char* copyrightText = "Developed By @4slash";
                    float cpw = ImGui::CalcTextSize(copyrightText).x;
                    ImGui::SetCursorPos(ImVec2((Size.x - cpw) * 0.5f, winHeight - 16.0f));
                    ImGui::TextColored(ImColor(107, 114, 128, 140), "%s", copyrightText);
                    ImGui::PopFont();
                }
                ImGui::End();
                return;
            }

        if (bIsMainMinimized) {
            // Elegant circular glass orb with pulsing violet aura
            const float orbRadius = 22.0f;
            ImGui::SetNextWindowSize(ImVec2(orbRadius * 2, orbRadius * 2));
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
            ImGuiWindowFlags iconFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, orbRadius);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.043f, 0.047f, 0.063f, 0.95f)); // Obsidian
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.953f, 0.957f, 0.965f, 0.70f));   // Pearl White

            if (ImGui::Begin("FloatingIcon", nullptr, iconFlags)) {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 windowPos = ImGui::GetWindowPos();
                ImVec2 windowSize = ImGui::GetWindowSize();
                ImVec2 center = ImVec2(windowPos.x + windowSize.x * 0.5f, windowPos.y + windowSize.y * 0.5f);
                
                bool isHovered = ImGui::IsWindowHovered();

                // Breathing pulse aura
                float t = (float)ImGui::GetTime();
                float pulse = sinf(t * 3.0f) * 0.5f + 0.5f;
                drawList->AddCircle(center, orbRadius + 1.0f + pulse * 2.0f, ImColor(255, 255, 255, (int)(40 + pulse * 60)), 32, 1.5f);

                // Hover glow
                if (isHovered) {
                    drawList->AddCircleFilled(center, orbRadius - 1.0f, ImColor(255, 255, 255, 25), 32);
                }

                // Centered icon/letter
                ImGui::PushFont(InterExtraBold ? InterExtraBold : InterBold);
                const char* text = "P";
                ImVec2 textSize = ImGui::CalcTextSize(text);
                ImVec2 textPos = ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f - 1.0f);
                ImU32 textColor = isHovered ? ImColor(255, 255, 255, 255) : ImColor(255, 255, 255, 240);
                drawList->AddText(textPos, textColor, text);
                ImGui::PopFont();

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
            winSize = ImVec2(480, 300);   // menu Connect agrandi
        } else {
            winSize = ImVec2(710, 500);   // menu principal : cartes plus larges (+50px l3ard)
        }
        ImGui::SetNextWindowSize(winSize);

        ImGui::Begin("Dashboard", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            ImDrawList* DrawList = ImGui::GetWindowDrawList();
            ImVec2 Pos = ImGui::GetWindowPos();
            ImVec2 Size = ImGui::GetWindowSize();

            // (Keybind Status Bar : INDEPENDANT - il ne suit PLUS le menu, on le deplace librement)

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
                DrawMovingBorder(DrawList, Pos, Pos + Size, 12.0f);

                // Header
                ImGui::SetCursorPos(ImVec2(15, 10));
                DrawList->AddCircleFilled(Pos + ImVec2(22, 19), 4.0f, ImColor(255, 255, 255, 255), 12);
                ImGui::SetCursorPos(ImVec2(35, 10));
                ImGui::PushFont(InterBold);
                ImGui::Text("System"); ImGui::SameLine();
                ImGui::TextColored(ImVec4(1, 1, 1, 0.45f), "/  Initializing");
                ImGui::PopFont();

                // Header divider
                DrawList->AddLine(Pos + ImVec2(0, 38), Pos + ImVec2(Size.x, 38), ImColor(38, 40, 56, 255));

                // Particules de fond (ecran Connect)
                DrawBgParticles(DrawList, Pos + ImVec2(0, 38), ImVec2(Size.x, Size.y - 38.0f), 0);   // 0 = particules retirees

                // Centered Spinner in Sky Blue
                ImGui::SetCursorPos(ImVec2(Size.x * 0.5f - 20, 85));
                LoadingSpinner("##InitSpinner", 20.0f, 3.0f, IM_COL32(243, 244, 246, 255));

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
                DrawMovingBorder(DrawList, Pos, Pos + Size, 12.0f);

                // Top header for Set Process
                ImGui::SetCursorPos(ImVec2(15, 10));
                ImU32 bulletColor = (g_Globals.General.State == Globals::InitState::ReadyToScan) ? ImColor(16, 185, 129, 255) : ImColor(244, 63, 94, 255);
                DrawList->AddCircleFilled(Pos + ImVec2(22, 19), 4.0f, bulletColor, 12); 
                ImGui::SetCursorPos(ImVec2(36, 11));
                ImGui::PushFont(InterBold);
                ImGui::Text(g_Globals.General.State == Globals::InitState::ReadyToScan ? "READY" : "OFFLINE");
                ImGui::PopFont();

                // Bouton "-" supprime de tous les menus

                // Separator Line below Header
                DrawList->AddLine(Pos + ImVec2(0, 38), Pos + ImVec2(Size.x, 38), ImColor(38, 40, 56, 255));

                // Particules de fond (ecran Connect)
                DrawBgParticles(DrawList, Pos + ImVec2(0, 38), ImVec2(Size.x, Size.y - 38.0f), 0);   // 0 = particules retirees

                // No sidebar on this screen — the button is centered on the full width
                float sidebar_width = 0.0f;

                // Main area (right of sidebar)
                float main_w = Size.x - sidebar_width;
                static std::string scanStatus = "";
                static bool isScanning = false;
                static ImVec4 statusColor = ImVec4(1, 1, 1, 0.4f);

                // Bouton "Connect Free Fire" : CENTRE exact (X et Y), largeur AUTO
                // (la taille suit le libelle en NOYH Heavy 20px + marge, sinon
                //  RenderTextClipped COUPE le texte comme avant)
                const char* btnText = "Connect Free Fire";
                if (g_Globals.General.State == Globals::InitState::Failed) btnText = "Retry System Init";

                float btnW = 220.0f;                        // largeur de secours
                const float btnH = 40.0f;                   // hauteur (etait 28 -> plus grande)
                {
                    ImFont* btnMeasureFont = FunctionFont ? FunctionFont
                                           : (InterBold12 ? InterBold12 : InterBold);
                    if (btnMeasureFont) ImGui::PushFont(btnMeasureFont);
                    const float labelW = ImGui::CalcTextSize(btnText).x;
                    if (btnMeasureFont) ImGui::PopFont();
                    if (labelW > 1.0f) btnW = labelW + 48.0f;   // +24px de marge de chaque cote
                }
                const float btnX = sidebar_width + (main_w - btnW) * 0.5f;
                const float btnY = (Size.y - btnH) * 0.5f; // centre vertical de la fenetre
                ImGui::SetCursorPos(ImVec2(btnX, btnY));
                
                if (isScanning || g_Globals.General.State == Globals::InitState::ScanningModule) {
                    ImGui::BeginGroup();
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    ImVec2 pMin = ImGui::GetCursorScreenPos();
                    ImVec2 pMax = ImVec2(pMin.x + btnW, pMin.y + btnH);
                    drawList->AddRectFilled(pMin, pMax, ImColor(16, 17, 24, 255), 8.0f);
                    drawList->AddRect(pMin, pMax, ImColor(255, 255, 255, 160), 8.0f, 0, 1.5f);

                    // Spinner + texte CENTRES dans le carre (btnW x btnH)
                    const char* connTxt = "CONNECTING...";
                    ImGui::PushFont(InterBold12 ? InterBold12 : InterBold);
                    const ImVec2 tsz = ImGui::CalcTextSize(connTxt);
                    ImGui::PopFont();

                    const float groupW = 20.0f + 8.0f + tsz.x;   // diam. spinner (r=10) + gap + texte
                    const float startX = (btnW - groupW) * 0.5f;

                    ImGui::SetCursorPos(ImVec2(btnX + startX, btnY + (btnH - 20.0f) * 0.5f));
                    LoadingSpinner("##AdbConnectSpinner", 10.0f, 2.5f, IM_COL32(243, 244, 246, 255));

                    ImGui::SetCursorPos(ImVec2(btnX + startX + 28.0f, btnY + (btnH - tsz.y) * 0.5f));
                    ImGui::PushFont(InterBold12 ? InterBold12 : InterBold);
                    ImGui::TextColored(ImVec4(0.953f, 0.957f, 0.965f, 1.0f), connTxt);
                    ImGui::PopFont();

                    ImGui::EndGroup();
                } else {
                    // btnText est calcule plus haut (largeur AUTO du bouton)
                    if (ImGui::ButtonStyled(btnText, ImVec2(btnW, btnH))) {
                        if (g_Globals.General.State == Globals::InitState::Failed) {
                            // Manual retry of system background init
                            g_Globals.General.State = Globals::InitState::InitializingMemory;
                            CurrentTab = 10;
                            extern void BackgroundInit();
                            extern std::thread g_InitThread;
                            if (g_InitThread.joinable()) g_InitThread.join();
                            g_InitThread = std::thread(BackgroundInit);
                        } else {
                            // Launch Free Fire -> wait 10s -> then the normal connect
                            isScanning = true;
                            g_Globals.General.State = Globals::InitState::ScanningModule;
                            scanStatus = "Launching Free Fire...";
                            statusColor = ImVec4(0.953f, 0.957f, 0.965f, 1.0f);

                            std::thread([&]() {
                                ThreadGuard guard;

                                // 1) open the game
                                FWork::ADB::LaunchFreeFire();
                                scanStatus = "Waiting for Free Fire (10s)...";

                                // 2) let it boot / load
                                for (int i = 0; i < 10; ++i) {
                                    if (g_Globals.General.ShutDown) break;
                                    std::this_thread::sleep_for(std::chrono::seconds(1));
                                }

                                // 3) then do exactly what it did before
                                scanStatus = "Connecting to emulator...";
                                bool success = FWork::ADB::InitializeADB();
                                if (success) {
                                    scanStatus = "Success!";
                                    statusColor = ImVec4(0.1f, 0.85f, 0.4f, 1.0f);
                                    g_Globals.General.State = Globals::InitState::Completed;
                                    std::this_thread::sleep_for(std::chrono::seconds(1));
                                    CurrentTab = 2; // Transition to Combat tab
                                    isScanning = false;
                                } else {
                                    scanStatus = "Failed, please retry";
                                    statusColor = ImVec4(0.95f, 0.25f, 0.35f, 1.0f);
                                    g_Globals.General.State = Globals::InitState::ReadyToScan;
                                    isScanning = false;
                                }
                            }).detach();
                        }
                    }
                }

                // Status Message display : CENTRE horizontalement, juste SOUS le carre CONNECTING
                if (!scanStatus.empty()) {
                    ImGui::PushFont(InterBold12);                                            // mesurer AVEC la meme fonte que le texte
                    float tw = ImGui::CalcTextSize(scanStatus.c_str()).x;
                    ImGui::SetCursorPos(ImVec2(sidebar_width + (main_w - tw) * 0.5f, btnY + btnH + 18.0f));  // juste sous le carre CONNECTING
                    ImGui::TextColored(statusColor, "%s", scanStatus.c_str());
                    ImGui::PopFont();
                }

                // --- FOOTER (f AKHIR l'ecran Connect) : Copyright Editorial X - Core (c) Devloped By Nizar ---
                {
                    ImFont* footFont = InterBold12 ? InterBold12 : InterBold;
                    const float footSz = 12.0f;
                    const char* cpTxt  = "Copyright 0x Void - Core";      // gauche
                    const char* midTxt = "\xC2\xA9";              // symbole (c) au centre
                    const char* devTxt = "Devloped By 4slash";     // droite

                    const ImVec2 s1 = (footFont ? footFont->CalcTextSizeA(footSz, FLT_MAX, 0.0f, cpTxt)
                                                : ImGui::CalcTextSize(cpTxt));
                    const ImVec2 sM = (footFont ? footFont->CalcTextSizeA(footSz, FLT_MAX, 0.0f, midTxt)
                                                : ImGui::CalcTextSize(midTxt));
                    const ImVec2 s2 = (footFont ? footFont->CalcTextSizeA(footSz, FLT_MAX, 0.0f, devTxt)
                                                : ImGui::CalcTextSize(devTxt));

                    const float gap  = 6.0f;      // space simple kif chi " " wa7d l'©
                    const float total = s1.x + gap + sM.x + gap + s2.x;
                    const float sx    = Pos.x + (Size.x - total) * 0.5f;
                    const float midY  = Pos.y + Size.y - 20.0f;

                    const ImU32 col  = ImColor(255, 255, 255, 135);
                    const ImU32 colM = ImColor(255, 255, 255, 215);   // (c) un peu plus vif

                    DrawList->AddText(footFont, footSz, ImVec2(sx, midY - s1.y * 0.5f), col, cpTxt);

                    const float midX = sx + s1.x + gap;
                    DrawList->AddText(footFont, footSz, ImVec2(midX, midY - sM.y * 0.5f), colM, midTxt);

                    DrawList->AddText(footFont, footSz, ImVec2(midX + sM.x + gap, midY - s2.y * 0.5f), col, devTxt);
                }
            }
            else // --- MAIN DASHBOARD ---
            {
                float sidebar_width = 74.0f;   
                float header_height = 40.0f;

                // --- Backgrounds ---
                DrawList->AddRectFilled(Pos, Pos + Size, ImColor(11, 12, 16, 252), 12.0f); // Main body background
                DrawList->AddRect(Pos, Pos + Size, ImColor(255, 255, 255, 85), 12.0f, 0, 1.6f); // Outline fixe (plus de rotation)

                // Filet qui SUIVI le curseur dans le menu
                DrawMouseTrail(ImGui::GetForegroundDrawList(), Pos, Pos + Size);
                DrawList->AddRectFilled(Pos, Pos + ImVec2(sidebar_width, Size.y), ImColor(9, 10, 14, 255), 12.0f, ImDrawFlags_RoundCornersLeft); // Sidebar background
                DrawList->AddRectFilled(Pos + ImVec2(sidebar_width, 0), Pos + ImVec2(Size.x, header_height), ImColor(13, 14, 20, 255), 12.0f, ImDrawFlags_RoundCornersTopRight); // Header background

                // PARTICULES "MGADIN" : CALMES, dessinees AU DESSUS des fonds
                // internes (cartes / panneaux) pour qu'elles restent visibles
                DrawMenuDust(ImGui::GetForegroundDrawList(), Pos, Size, 0);   // 0 = particules retirees

                // --- EFFECT: Animated Luxury Gradient Glow ---
                {
                    float t = (float)ImGui::GetTime();
                    float pulse = (sinf(t * 1.5f) * 0.5f + 0.5f);
                    int glowAlpha = (int)(10 + pulse * 14);
                    // Top-left pearl white glow
                    ImVec2 glowTL = Pos + ImVec2(sidebar_width, 0);
                    ImVec2 glowBR = Pos + ImVec2(sidebar_width + Size.x * 0.45f, Size.y * 0.40f);
                    DrawList->AddRectFilledMultiColor(glowTL, glowBR,
                        ImColor(255, 255, 255, glowAlpha), ImColor(0, 0, 0, 0),
                        ImColor(0, 0, 0, 0), ImColor(255, 255, 255, glowAlpha / 2));
                    // Bottom-right soft white glow
                    ImVec2 glow2TL = Pos + ImVec2(Size.x * 0.55f, Size.y * 0.55f);
                    ImVec2 glow2BR = Pos + Size;
                    int glow2Alpha = (int)(6 + pulse * 10);
                    DrawList->AddRectFilledMultiColor(glow2TL, glow2BR,
                        ImColor(0, 0, 0, 0), ImColor(255, 255, 255, glow2Alpha / 2),
                        ImColor(255, 255, 255, glow2Alpha), ImColor(0, 0, 0, 0));
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
                    DrawList->AddLine(lnStart, lnEnd, ImColor(255, 255, 255, 50), 1.0f);
                    float sLeft = (std::max)(sidebar_width, shimX - halfGlow);
                    float sRight = (std::min)(Size.x, shimX + halfGlow);
                    if (sRight > sLeft) {
                        DrawList->AddRectFilledMultiColor(
                            Pos + ImVec2(sLeft, header_height - 1),
                            Pos + ImVec2(sRight, header_height + 1),
                            ImColor(255, 255, 255, 0), ImColor(255, 255, 255, 130),
                            ImColor(255, 255, 255, 130), ImColor(255, 255, 255, 0));
                    }
                }

                // --- Top Branding Header : titre plus grand, sans point vert, sans badge "V.1 Safe" ---
                {
                    // --- TITRE (kbir chwiya) f lsar ---
                    const char* brand = "0x Void - Core - Internal";
                    const float brandSz = 20.0f;
                    const ImVec2 bs = InterBold ? InterBold->CalcTextSizeA(brandSz, FLT_MAX, 0.0f, brand)
                                                : ImGui::CalcTextSize(brand);
                    const float bx = Pos.x + sidebar_width + 10.0f;   // rj3 lisr chwiya
                    const float by = Pos.y + (header_height - bs.y) * 0.5f;
                    DrawList->AddText(InterBold, brandSz, ImVec2(bx + 1.0f, by + 1.0f),
                                      ImColor(255, 255, 255, 65), brand);   // halo
                    DrawList->AddText(InterBold, brandSz, ImVec2(bx, by),
                                      ImColor(255, 255, 255, 255), brand);

                    // --- "Devloped By Nizar" f AKHIR l'header (limn) b3id mn titr ---
                    const char* dev    = "Devloped By 4slash";
                    const float devSz  = 13.0f;
                    ImFont* devFont    = InterBold12 ? InterBold12 : InterBold;
                    const ImVec2 ds    = devFont->CalcTextSizeA(devSz, FLT_MAX, 0.0f, dev);
                    const float dx     = Pos.x + Size.x - 18.0f - ds.x;
                    const float dy     = Pos.y + (header_height - ds.y) * 0.5f;

                    DrawList->AddCircleFilled(ImVec2(dx - 13.0f, dy + ds.y * 0.5f), 2.5f,
                                              ImColor(255, 255, 255, 95), 12); // pastille de separation
                    DrawList->AddText(devFont, devSz, ImVec2(dx + 1.0f, dy + 1.0f),
                                      ImColor(255, 255, 255, 55), dev);       // halo
                    DrawList->AddText(devFont, devSz, ImVec2(dx, dy),
                                      ImColor(255, 255, 255, 170), dev);
                }

                // Boutons "-" et "x" supprimes de tous les menus

                // --- Minimalist Navigation Sidebar with Gliding Pill ---
                constexpr float kSideImgSize = 52.0f;   // image (carrée)
                constexpr float kSideImgPad  = 7.0f;    // marge: descendue de 5px (taille inchangee 52)
                constexpr float kSideTop     = 72.0f;   // onglets descendus de 8px (largeur inchangee)
                constexpr float kNavSize     = 40.0f;   // taille des onglets (retrecis)
                constexpr float kNavStep     = 50.0f;   // espacement vertical
                const float kPillX           = (sidebar_width - kNavSize) * 0.5f;  // recentrage carre

                // --- Avatar / logo : tout en haut de la sidebar, au-dessus du header ---
                if (SidebarImage) {
                    ImVec2 imgSize(kSideImgSize, kSideImgSize);
                    ImVec2 imgPos = Pos + ImVec2((sidebar_width - kSideImgSize) * 0.5f, kSideImgPad);
                    // (rectangle derriere le logo SUPPRIME : le PNG est transparent, le fond aussi)
                    DrawList->AddImage(SidebarImage, imgPos, imgPos + imgSize);
                }

                // --- BADGE "ONLINE" : TSWIRA JDIDA (AM.png), SOUS TOUS les onglets ---
                if (BadgeImage) {
                    const float avS = 60.0f;                       // AKBAR (48 -> 60)
                    const float avX = Pos.x + sidebar_width * 0.5f;
                    const float avY = Pos.y + Size.y - 44.0f;      // n7el-h ta7t
                    DrawList->AddImage(BadgeImage,
                                       ImVec2(avX - avS * 0.5f, avY - avS * 0.5f),
                                       ImVec2(avX + avS * 0.5f, avY + avS * 0.5f));
                }

                ImGui::SetCursorPos(ImVec2(0, header_height));
                // NoBackground: le ChildBg (96% opaque) rognait le bas du logo (y 40..54)
                // car ce child commence a header_height=40 et se dessine APRES l'image.
                // La sidebar a deja son propre fond opaque + (9,10,14) dessine plus haut.
                ImGui::BeginChild("Sidebar", ImVec2(sidebar_width, Size.y - header_height), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
                {
                    // Gliding active indicator
                    static float g_IndicatorY = -1.0f; // -1 = ماشي initialized
                    if (CurrentTab == 6) CurrentTab = 2;   // Tab "Info" (bas) supprime -> retour Security
                    float targetY = kSideTop;
                    if (CurrentTab == 2) targetY = kSideTop;
                    else if (CurrentTab == 3) targetY = kSideTop + kNavStep;
                    else if (CurrentTab == 4) targetY = kSideTop + kNavStep * 2.0f;
                    else if (CurrentTab == 5) targetY = kSideTop + kNavStep * 3.0f;
                    else if (CurrentTab == 7) targetY = kSideTop + kNavStep * 4.0f;  // AIMKILL

                    // أول مرة -> يقفز مباشرة للبلاصة الصحيحة (بلا أنيميشن)
                    if (g_IndicatorY < 0.0f) {
                        g_IndicatorY = targetY;
                    }
                    else {
                        g_IndicatorY = ImLerp(g_IndicatorY, targetY, 1.0f - expf(-18.0f * ImGui::GetIO().DeltaTime));
                    }

                    // Onglet actif : CARRE, recentre + barre d'accent a DROITE
                    const ImVec2 pillMin = Pos + ImVec2(kPillX, header_height + g_IndicatorY);
                    const ImVec2 pillMax = pillMin + ImVec2(kNavSize, kNavSize);
                    DrawList->AddRectFilled(pillMin, pillMax, ImColor(255, 255, 255, 45), 6.0f);
                    DrawList->AddRectFilled(ImVec2(pillMax.x + 3.0f, pillMin.y + 8.0f), ImVec2(pillMax.x + 6.0f, pillMax.y - 8.0f), ImColor(255, 255, 255, 255), 1.5f);
                    DrawList->AddRect(pillMin, pillMax, ImColor(255, 255, 255, 110), 6.0f, 0, 1.0f);

                    auto NavButton = [&](const char* icon, int tab_id, float y_pos) {
                        bool active = (CurrentTab == tab_id);
                        ImGui::SetCursorPos(ImVec2((sidebar_width - kNavSize) * 0.5f, y_pos));

                        std::string id_str = std::string("##nav_") + std::to_string(tab_id);
                        ImGui::InvisibleButton(id_str.c_str(), ImVec2(kNavSize, kNavSize));
                        bool hovered = ImGui::IsItemHovered();
                        if (ImGui::IsItemClicked()) {
                            CurrentTab = tab_id;
                        }

                        if (hovered && !active) {
                            DrawList->AddRectFilled(Pos + ImVec2(kPillX, header_height + y_pos), Pos + ImVec2(kPillX + kNavSize, header_height + y_pos + kNavSize), ImColor(255, 255, 255, 10), 6.0f);
                        }

                        // Render centered icon
                        ImGui::PushFont(FontAwesomeSolid);
                        ImVec2 icon_size = ImGui::CalcTextSize(icon);
                        ImVec2 icon_pos = Pos + ImVec2(sidebar_width / 2.0f - icon_size.x / 2.0f, header_height + y_pos + kNavSize * 0.5f - icon_size.y / 2.0f);
                        DrawList->AddText(icon_pos, active ? ImColor(255, 255, 255, 255) : (hovered ? ImColor(220, 222, 230, 255) : ImColor(107, 114, 128, 190)), icon);
                        ImGui::PopFont();
                    };

                    NavButton(ICON_FA_SHIELD_HALVED, 2, kSideTop);
                    NavButton(ICON_FA_EYE, 3, kSideTop + kNavStep);
                    NavButton(ICON_FA_BOLT, 4, kSideTop + kNavStep * 2.0f);
                    NavButton(ICON_FA_GEAR, 5, kSideTop + kNavStep * 3.0f);
                    NavButton(ICON_FA_CROSSHAIRS, 7, kSideTop + kNavStep * 4.0f);   // AIMKILL
                    // Tab "Info" en bas supprime (icône ? + theme)
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
                            ImGui::CustomChild("Aimbot", ImVec2(cw / 2 - 6, ch));
                            {
                                ImGui::Checkbox("Aimbot Visible", &g_Globals.AimBot.AimBotVisibleSafe);
                                if (g_Globals.AimBot.AimBotVisibleSafe) {
                                    ImGui::SliderInt("Head Rate", &g_Globals.AimBot.HeadRate, 0, 100);
                                    ImGui::SliderInt("Chest Rate", &g_Globals.AimBot.ChestRate, 0, 100);
                                    ImGui::SliderFloat("Strength", &g_Globals.AimBot.AimBotVisibleSafeStrength, 0.0f, 1.0f, "%.2f");
                                }

                                // AIMLOCK config inline when selected
                                if (g_Globals.AimBot.AimbotType == 3) {
                                    g_Globals.AimBot.AimLockToggle = true;
                                    const char* aimLockBones[] = { "Head", "Body", "Neck" };
                                    if (g_Globals.AimBot.AimLockBone < 0 || g_Globals.AimBot.AimLockBone >= IM_ARRAYSIZE(aimLockBones))
                                        g_Globals.AimBot.AimLockBone = 0;
                                    ImGui::Combo("Bone##AimLock", &g_Globals.AimBot.AimLockBone, aimLockBones, IM_ARRAYSIZE(aimLockBones));
                                    ImGui::SliderFloat("Speed##AimLock", &g_Globals.AimBot.AimLockSpeed, 1.0f, 20.0f, "%.1f");
                                    ImGui::SliderInt("Max Dist##AimLock", &g_Globals.AimBot.AimLockMaxDistance, 50, 500, "%dm");
                                    ImGui::Checkbox("FOV Check##AimLock", &g_Globals.AimBot.AimLockFovEnable);
                                    if (g_Globals.AimBot.AimLockFovEnable) {
                                        ImGui::SliderInt("FOV##AimLock", &g_Globals.AimBot.AimLockFov, 30, 500);
                                    }
                                    ImGui::Checkbox("Only Visible##AimLock", &g_Globals.AimBot.AimLockOnlyVisible);
                                } else {
                                    g_Globals.AimBot.AimLockToggle = false;
                                }
                              /*   ImGui::KeyBind("Aimbot Key", &g_Globals.AimBot.AimbotBind, 0);*/

                                // Trait entre "Aimbot Visible" et "Silent Aim" : k7l (lwel) -> byd (wsat) -> k7l (lakhr)
                                {
                                    const float sepW = ImGui::GetContentRegionAvail().x;
                                    const float sepH = 2.0f;
                                    const ImVec2 sepP = ImGui::GetCursorScreenPos();
                                    ImDrawList* sepDl = ImGui::GetWindowDrawList();
                                    const ImU32 cK7l = IM_COL32(0, 0, 0, 255);
                                    const ImU32 cByd = IM_COL32(255, 255, 255, 255);
                                    // moitie gauche : k7l -> byd
                                    sepDl->AddRectFilledMultiColor(ImVec2(sepP.x, sepP.y),
                                                                  ImVec2(sepP.x + sepW * 0.5f, sepP.y + sepH),
                                                                  cK7l, cByd, cByd, cK7l);
                                    // moitie droite : byd -> k7l
                                    sepDl->AddRectFilledMultiColor(ImVec2(sepP.x + sepW * 0.5f, sepP.y),
                                                                  ImVec2(sepP.x + sepW, sepP.y + sepH),
                                                                  cByd, cK7l, cK7l, cByd);
                                    ImGui::Dummy(ImVec2(sepW, 8.0f));
                                }

                                ImGui::Checkbox("Silent Aim", &g_Globals.AimBot.SilentAimV2);
                                ImGui::KeyBind("Silent Aim Key", &g_Globals.AimBot.SilentAimV2ToggleKeybind, 0);
                                if (g_Globals.AimBot.SilentAimV2) {
                                    ImGui::SliderInt("FOV##SAV2", &g_Globals.AimBot.SilentAimV2Fov, 50, 1000);
                                    ImGui::SliderInt("Max Dist##SAV2", &g_Globals.AimBot.SilentAimV2MaxDistance, 50, 600);
                                    ImGui::Checkbox("Full Headshot##SAV2", &g_Globals.AimBot.SilentAimV2FullHeadshot);
                                    ImGui::Checkbox("Kill in Chest##SAV2", &g_Globals.AimBot.SilentAimV2KillInChest);
                                    ImGui::Checkbox("Ignore Knocked##SAV2", &g_Globals.AimBot.SilentAimV2IgnoreKnocked);
                                    ImGui::SliderInt("Head Rate##SAV2", &g_Globals.AimBot.HeadRate, 0, 100);
                                    ImGui::SliderInt("Chest Rate##SAV2", &g_Globals.AimBot.ChestRate, 0, 100);
                                    ImGui::Checkbox("Draw FOV Circle##SAV2", &g_Globals.AimBot.SilentAimV2ShowFov);
                                    ImGui::TextColored(ImVec4(1, 1, 1, 0.4f), "Lead automatique + declenchement AUTO au tir");
                                }

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
                                ImGui::CheckboxTest("No Recoil", &g_Globals.AimBot.NoRecoil);

                                // Trait entre "No Recoil" et "Fast Reload" : k7l (lwel) -> byd (wsat) -> k7l (lakhr)
                                {
                                    const float sepW = ImGui::GetContentRegionAvail().x;
                                    const float sepH = 2.0f;
                                    const ImVec2 sepP = ImGui::GetCursorScreenPos();
                                    ImDrawList* sepDl = ImGui::GetWindowDrawList();
                                    const ImU32 cK7l = IM_COL32(0, 0, 0, 255);
                                    const ImU32 cByd = IM_COL32(255, 255, 255, 255);
                                    // moitie gauche : k7l -> byd
                                    sepDl->AddRectFilledMultiColor(ImVec2(sepP.x, sepP.y),
                                                                  ImVec2(sepP.x + sepW * 0.5f, sepP.y + sepH),
                                                                  cK7l, cByd, cByd, cK7l);
                                    // moitie droite : byd -> k7l
                                    sepDl->AddRectFilledMultiColor(ImVec2(sepP.x + sepW * 0.5f, sepP.y),
                                                                  ImVec2(sepP.x + sepW, sepP.y + sepH),
                                                                  cByd, cK7l, cK7l, cByd);
                                    ImGui::Dummy(ImVec2(sepW, 8.0f));
                                }

                                ImGui::CheckboxTest("Fast Reload", &g_Globals.AimBot.FastReload);
                                ImGui::KeyBind("Fast Reload Toggle", &g_Globals.AimBot.FastReloadToggleKeybind, 0);

                                //ImGui::Checkbox("Fast Switch", &g_Globals.Exploits.FastSwitch);
                              /*  ImGui::Checkbox("Auto Fire", &g_Globals.AimBot.AutoFire);
                                if (g_Globals.AimBot.AutoFire)
                                    ImGui::SliderFloat("Auto Fire Radius", &g_Globals.AimBot.AutoFireRadius, 50.0f, 300.0f, "%.0f px");*/

                             /*   ImVec2 pos4 = ImGui::GetCursorPos();
                                ImGui::SetCursorPosY(pos4.y + 3);*/
                              /*  ImGui::Text("Infinite Ammo"); ImGui::SameLine(0.0f, 0.0f);
                                ImGui::TextColored(ImVec4(0.91f, 0.30f, 0.24f, 1.0f), " (Risk)");
                                ImGui::SetCursorPos(pos4);
                                ImGui::Checkbox("##InfiniteAmmo", &g_Globals.Exploits.InfiniteAmmo);
                                */
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
                                ImGui::Checkbox("Rank Icon", &g_Globals.Visuals.RankIcon);
                                
                                if (ImGui::Checkbox("Health Text ESP", &g_Globals.Visuals.ESPHealthTEXT)) {
                                    g_Globals.Visuals.HealthBar = g_Globals.Visuals.ESPHealthTEXT;
                                }
                                ImGui::Checkbox("Health Bar ESP", &g_Globals.Visuals.HealthBar);
                                
                                
                                ImGui::Checkbox("Weapon Text", &g_Globals.Visuals.ESPWeapon);
                                ImGui::Checkbox("Weapon Icons", &g_Globals.Visuals.ESPWeaponIcon);
                                ImGui::Checkbox("RGB Color Mode", &g_Globals.Visuals.RGB);
                                
                                
                                // Trait entre "RGB Color Mode" et "Head Dot" : k7l (lwel) -> byd (wsat) -> k7l (lakhr)
                                {
                                    const float sepW = ImGui::GetContentRegionAvail().x;
                                    const float sepH = 2.0f;
                                    const ImVec2 sepP = ImGui::GetCursorScreenPos();
                                    ImDrawList* sepDl = ImGui::GetWindowDrawList();
                                    const ImU32 cK7l = IM_COL32(0, 0, 0, 255);
                                    const ImU32 cByd = IM_COL32(255, 255, 255, 255);
                                    // moitie gauche : k7l -> byd
                                    sepDl->AddRectFilledMultiColor(ImVec2(sepP.x, sepP.y),
                                                                  ImVec2(sepP.x + sepW * 0.5f, sepP.y + sepH),
                                                                  cK7l, cByd, cByd, cK7l);
                                    // moitie droite : byd -> k7l
                                    sepDl->AddRectFilledMultiColor(ImVec2(sepP.x + sepW * 0.5f, sepP.y),
                                                                  ImVec2(sepP.x + sepW, sepP.y + sepH),
                                                                  cByd, cK7l, cK7l, cByd);
                                    ImGui::Dummy(ImVec2(sepW, 8.0f));
                                }

                                ImGui::Checkbox("Head Dot", &g_Globals.Visuals.HeadDot);
                                ImGui::Checkbox("Moco Indicator", &g_Globals.Visuals.moco);
                                ImGui::Checkbox("Wukong Bypass", &g_Globals.Visuals.Wukong);
                                ImGui::Checkbox("Show Enemies Count", &g_Globals.Visuals.Enemy);
                                ImGui::Checkbox("Watermark Title", &g_Globals.Visuals.Watermark);
                                ImGui::Checkbox("Invlied Timer", &g_Globals.Visuals.ShowGameTimer);
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
                                ImGui::Checkbox("Green On Aim (Alvo)", &g_Globals.Visuals.Alvo);
                                if (g_Globals.Visuals.Alvo)
                                    ImGui::ColorEdit4("Green On Aim Color", g_Globals.Visuals.AlvoColor, ImGuiColorEditFlags_NoInputs);
                                ImGui::Combo("Box Style", &g_Globals.Visuals.players_box, "None\0Full\0Cornered\0");
                                ImGui::Combo("Health Style", &g_Globals.Visuals.players_healthbar, "None\0Left\0Right\0Top\0Bottom\0Text Only\0");
                                ImGui::SliderInt("ESP Distance", &g_Globals.Visuals.DistanceEsp, 0, 500, "%dm");

                                ImGui::Checkbox("Glow Lines", &g_Globals.Visuals.GlowLines);
                                ImGui::Checkbox("Rainbow Lines", &g_Globals.Visuals.RainbowLines);
                                ImGui::SliderFloat("Glow Radius", &g_Globals.Visuals.GlowRadius, 1.0f, 30.0f, "%.1f");
                                ImGui::SliderFloat("Glow Feather", &g_Globals.Visuals.GlowFeather, 0.5f, 10.0f, "%.1f");
                                ImGui::SliderFloat("Line Thickness", &g_Globals.Visuals.LineThickness, 0.5f, 5.0f, "%.1f");
                                
                                ImGui::Checkbox("Box Border Fill", &g_Globals.Visuals.FilledBox);
                                ImGui::Checkbox("Box Solid Fill", &g_Globals.Visuals.FillColorBox);
                                ImGui::ColorEdit4("Box Solid Color", g_Globals.Visuals.FillColor, ImGuiColorEditFlags_NoInputs);
                                
                                ImGui::SliderFloat("Head Dot Size", &g_Globals.Visuals.HeadDotSize, 1.0f, 10.0f, "%.1f");
                                ImGui::ColorEdit4("Head Dot Color", g_Globals.Visuals.HeadDotColor, ImGuiColorEditFlags_NoInputs);
                                
                                if (ImGui::Checkbox("Refresh ESP", &g_Globals.Visuals.RefreshEsp)) {
                                    if (g_Globals.Visuals.RefreshEsp)
                                        FWork::Data::Refresh();
                                }
                                ImGui::KeyBind("Refresh Toggle", &g_Globals.Visuals.RefreshEspToggleKeybind, 0);
                                
                                ImGui::Checkbox("Fix ESP", &g_Globals.Visuals.FixESP);
                                ImGui::KeyBind("Fix ESP Toggle", &g_Globals.Visuals.FixESPToggleKeybind, 0);
                                
                                
                                ImGui::ColorEdit4("ESP Box Color", g_Globals.Visuals.BoxColor, ImGuiColorEditFlags_NoInputs);
                                ImGui::ColorEdit4("ESP Lines Color", g_Globals.Visuals.LinesColor, ImGuiColorEditFlags_NoInputs);
                                ImGui::ColorEdit4("Skeleton Color", g_Globals.Visuals.SkeletonColor, ImGuiColorEditFlags_NoInputs);
                                ImGui::ColorEdit4("Weapon Text Color", g_Globals.Visuals.GunColor, ImGuiColorEditFlags_NoInputs);
                                ImGui::ColorEdit4("Watermark Color", g_Globals.Visuals.WatermarkColor, ImGuiColorEditFlags_NoInputs);
                                ImGui::ColorEdit4("Enemies Text Color", g_Globals.Visuals.EnemyColor, ImGuiColorEditFlags_NoInputs);

                                // Trait entre "Enemies Text Color" et "Through Walls" : k7l (lwel) -> byd (wsat) -> k7l (lakhr)
                                {
                                    const float sepW = ImGui::GetContentRegionAvail().x;
                                    const float sepH = 2.0f;
                                    const ImVec2 sepP = ImGui::GetCursorScreenPos();
                                    ImDrawList* sepDl = ImGui::GetWindowDrawList();
                                    const ImU32 cK7l = IM_COL32(0, 0, 0, 255);
                                    const ImU32 cByd = IM_COL32(255, 255, 255, 255);
                                    // moitie gauche : k7l -> byd
                                    sepDl->AddRectFilledMultiColor(ImVec2(sepP.x, sepP.y),
                                                                  ImVec2(sepP.x + sepW * 0.5f, sepP.y + sepH),
                                                                  cK7l, cByd, cByd, cK7l);
                                    // moitie droite : byd -> k7l
                                    sepDl->AddRectFilledMultiColor(ImVec2(sepP.x + sepW * 0.5f, sepP.y),
                                                                  ImVec2(sepP.x + sepW, sepP.y + sepH),
                                                                  cByd, cK7l, cK7l, cByd);
                                    ImGui::Dummy(ImVec2(sepW, 8.0f));
                                }

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
                    else if (CurrentTab == 4) // --- EXPLOITS ---
                    {
                        // Column 1
                        ImGui::SetCursorPos(ImVec2(0, 0));
                        ImGui::BeginChild("ExploitsCol1", ImVec2(cw / 2 - 6, ch), false, ImGuiWindowFlags_NoScrollbar);
                        {
                            ImGui::CustomChild("Network & Movement", ImVec2(cw / 2 - 6, ch));
                            {


                                ImGui::Checkbox("Freeze Enemy", &g_Globals.Exploits.FakeLag_Freeze);
                                ImGui::KeyBind("Freeze Toggle", &g_Globals.Exploits.FakeLag_FreezeToggleKeybind, 0);

                                {
                                    std::string freezeStatus = FakeLagFreeze::StatusText();
                                    if (!freezeStatus.empty()) {
                                        bool freezeError = (freezeStatus.find("error") != std::string::npos) ||
                                                           (freezeStatus.find("not found") != std::string::npos) ||
                                                           (freezeStatus.find("failed") != std::string::npos);
                                        ImGui::TextColored(freezeError
                                            ? ImVec4(1.0f, 0.35f, 0.35f, 0.95f)
                                            : ImVec4(1.0f, 0.72f, 0.25f, 0.95f),
                                            "%s", freezeStatus.c_str());
                                    }
                                }

                                // Trait : entre "Freeze Toggle" et "Ghost Lag" : k7l -> byd -> k7l
                                {
                                    const float sepW = ImGui::GetContentRegionAvail().x;
                                    const float sepH = 2.0f;
                                    const ImVec2 sepP = ImGui::GetCursorScreenPos();
                                    ImDrawList* sepDl = ImGui::GetWindowDrawList();
                                    const ImU32 cK7l = IM_COL32(0, 0, 0, 255);
                                    const ImU32 cByd = IM_COL32(255, 255, 255, 255);
                                    // moitie gauche : k7l -> byd
                                    sepDl->AddRectFilledMultiColor(ImVec2(sepP.x, sepP.y),
                                                                  ImVec2(sepP.x + sepW * 0.5f, sepP.y + sepH),
                                                                  cK7l, cByd, cByd, cK7l);
                                    // moitie droite : byd -> k7l
                                    sepDl->AddRectFilledMultiColor(ImVec2(sepP.x + sepW * 0.5f, sepP.y),
                                                                  ImVec2(sepP.x + sepW, sepP.y + sepH),
                                                                  cByd, cK7l, cK7l, cByd);
                                    ImGui::Dummy(ImVec2(sepW, 8.0f));
                                }


                                ImGui::Checkbox("Ghost Lag", &g_Globals.Exploits.FakeLag_Tele);
                                if (g_Globals.Exploits.FakeLag_Tele) {
                                    // Ping Safe : les petits paquets (keep-alive / mesure
                                    // de RTT) sont envoyes immediatement et jamais retenus,
                                    // seuls les paquets de mouvement le sont -> l'affichage
                                    // du ping reste NORMAL pendant l'effet ghost.
                                    ImGui::SliderInt("Ping Safe##GhostLag",
                                                     &g_Globals.Exploits.GhostPingPassBytes,
                                                     0, 200, "pass <= %d B");
                                    if (g_Globals.Exploits.GhostPingPassBytes > 0)
                                        ImGui::TextColored(ImVec4(0.10f, 0.85f, 0.40f, 1.0f),
                                                           "Ping stays normal - keep-alives bypass the hold");
                                    else
                                        ImGui::TextColored(ImVec4(1.00f, 0.72f, 0.25f, 0.95f),
                                                           "Ping will rise - every packet is held");
                                }
                                ImGui::KeyBind("Ghost Lag Toggle", &g_Globals.Exploits.FakeLag_TeleToggleKeybind, 0);

                                {
                                    std::string teleStatus = FakeLagFreeze::TelekillStatusText();
                                    if (!teleStatus.empty()) {
                                        bool teleError = (teleStatus.find("error") != std::string::npos) ||
                                                         (teleStatus.find("not found") != std::string::npos) ||
                                                         (teleStatus.find("failed") != std::string::npos);
                                        ImGui::TextColored(teleError
                                            ? ImVec4(1.0f, 0.35f, 0.35f, 0.95f)
                                            : ImVec4(1.0f, 0.72f, 0.25f, 0.95f),
                                            "%s", teleStatus.c_str());
                                    }
                                }


                               /* ImGui::Checkbox("Teleport Base", &g_Globals.Exploits.TeleportBase);
                                if (g_Globals.Exploits.TeleportBase)
                                    ImGui::SliderFloat("Base Max Dist", &g_Globals.Exploits.TeleportBaseDistance, 10.0f, 200.0f, "%.0f");*/

                                // Trait : entre "Ghost Lag Toggle" et "Back Jump" : k7l -> byd -> k7l
                                {
                                    const float sepW = ImGui::GetContentRegionAvail().x;
                                    const float sepH = 2.0f;
                                    const ImVec2 sepP = ImGui::GetCursorScreenPos();
                                    ImDrawList* sepDl = ImGui::GetWindowDrawList();
                                    const ImU32 cK7l = IM_COL32(0, 0, 0, 255);
                                    const ImU32 cByd = IM_COL32(255, 255, 255, 255);
                                    // moitie gauche : k7l -> byd
                                    sepDl->AddRectFilledMultiColor(ImVec2(sepP.x, sepP.y),
                                                                  ImVec2(sepP.x + sepW * 0.5f, sepP.y + sepH),
                                                                  cK7l, cByd, cByd, cK7l);
                                    // moitie droite : byd -> k7l
                                    sepDl->AddRectFilledMultiColor(ImVec2(sepP.x + sepW * 0.5f, sepP.y),
                                                                  ImVec2(sepP.x + sepW, sepP.y + sepH),
                                                                  cByd, cK7l, cK7l, cByd);
                                    ImGui::Dummy(ImVec2(sepW, 8.0f));
                                }


                                ImGui::Checkbox("Back Jump", &g_Globals.Exploits.BackJump);

                                // Trait : entre "Back Jump" et "Push Forward"
                                {
                                    const float sepW = ImGui::GetContentRegionAvail().x;
                                    const float sepH = 2.0f;
                                    const ImVec2 sepP = ImGui::GetCursorScreenPos();
                                    ImDrawList* sepDl = ImGui::GetWindowDrawList();
                                    const ImU32 cK7l = IM_COL32(0, 0, 0, 255);
                                    const ImU32 cByd = IM_COL32(255, 255, 255, 255);
                                    sepDl->AddRectFilledMultiColor(ImVec2(sepP.x, sepP.y),
                                                                  ImVec2(sepP.x + sepW * 0.5f, sepP.y + sepH),
                                                                  cK7l, cByd, cByd, cK7l);
                                    sepDl->AddRectFilledMultiColor(ImVec2(sepP.x + sepW * 0.5f, sepP.y),
                                                                  ImVec2(sepP.x + sepW, sepP.y + sepH),
                                                                  cByd, cK7l, cK7l, cByd);
                                    ImGui::Dummy(ImVec2(sepW, 8.0f));
                                }

                                ImGui::Checkbox("Push Forward", &g_Globals.Exploits.PushForward);
                                if (g_Globals.Exploits.PushForward) {
                                    ImGui::SliderFloat("Push Speed##PF", &g_Globals.Exploits.PushForwardSpeed, 0.50f, 20.00f, "%.2f");
                                    ImGui::SliderFloat("Push Ramp##PF", &g_Globals.Exploits.PushForwardRampMs, 50.0f, 1000.0f, "%.0f ms");

                                    // Diagnostic en direct : pourquoi ca bouge (ou pas)
                                    const auto& ps = ::FWork::MovementExploits::GetPushStatus();
                                    ImVec4 col(0.10f, 0.85f, 0.40f, 1.0f);
                                    const char* state = "RUN";
                                    if (!ps.running)             { state = "NOT RUNNING"; col = ImVec4(1.00f, 0.35f, 0.35f, 0.95f); }
                                    else if (!ps.inMatch)        { state = "NO MATCH";     col = ImVec4(1.00f, 0.35f, 0.35f, 0.95f); }
                                    else if (ps.blocked)         { state = "BLOCKED";      col = ImVec4(1.00f, 0.72f, 0.25f, 0.95f); }
                                    else if (!ps.dirOk)          { state = "NO DIR";       col = ImVec4(1.00f, 0.72f, 0.25f, 0.95f); }
                                    else if (ps.writes == 0)     { state = "NO WRITE";     col = ImVec4(1.00f, 0.35f, 0.35f, 0.95f); }
                                    else if (ps.factor < 0.999f) { state = "RAMP";         col = ImVec4(1.00f, 0.72f, 0.25f, 0.95f); }

                                    ImGui::TextColored(col, "Push: %s  writes=%ld  step=%.3f", state, ps.writes, (double)ps.step);
                                    ImGui::TextColored(col, "ctrl=%s root=%s  X=%.1f Z=%.1f",
                                                       ps.haveMoveComp ? "OK" : "--",
                                                       ps.haveMatrix   ? "OK" : "--",
                                                       (double)ps.posX, (double)ps.posZ);
                                }
                                ImGui::KeyBind("Push Forward Toggle", &g_Globals.Exploits.PushForwardToggleKeybind, 0);

                            }
                            ImGui::EndCustomChild();
                        }
                        ImGui::EndChild();

                        // Column 2
                        ImGui::SetCursorPos(ImVec2(cw / 2 + 6, 0));
                        ImGui::BeginChild("ExploitsCol2", ImVec2(cw / 2 - 6, ch), false, ImGuiWindowFlags_NoScrollbar);
                        {
                            ImGui::CustomChild("Misc Exploits", ImVec2(cw / 2 - 6, ch));
                            {
                                ImGui::CheckboxRisk("Fake Damage", &g_Globals.Exploits.FakeDame, "(Risk - Ban)");
                                if (g_Globals.Exploits.FakeDame) {
                                    if (FWork::Data::IsFakeDamageActive())
                                        ImGui::TextColored(ImVec4(0.10f, 0.85f, 0.40f, 1.0f),
                                                           "FakeDmg: APPLIED (BuffWeaponDamageScale)");
                                    else
                                        ImGui::TextColored(ImVec4(1.00f, 0.72f, 0.25f, 0.95f),
                                                           "FakeDmg: waiting for a match...");
                                }
                                ImGui::KeyBind("Fake Damage Toggle", &g_Globals.Exploits.FakeDameToggleKeybind, 0);

                                //ImGui::Checkbox("Fast Medkit", &g_Globals.Exploits.FastMedkit);


                                ImGui::CheckboxRisk("Pull Enemy", &g_Globals.Exploits.PullEnemy, "(Risk - Black List)");
                                if (g_Globals.Exploits.PullEnemy) {
                                    ImGui::SliderInt("Pull Enemy %", &g_Globals.Exploits.PullEnemyPercent, 10, 200, "%d%%");
                                    ImGui::SliderInt("Pull Enemy FOV", &g_Globals.Exploits.PullEnemyFov, 50, 500, "%d");
                                }
                                ImGui::KeyBind("Pull Enemy Toggle", &g_Globals.Exploits.PullEnemyBind, 0);

                                // Trait : entre "Pull Enemy Toggle" et "Speed Timer" : k7l -> byd -> k7l
                                {
                                    const float sepW = ImGui::GetContentRegionAvail().x;
                                    const float sepH = 2.0f;
                                    const ImVec2 sepP = ImGui::GetCursorScreenPos();
                                    ImDrawList* sepDl = ImGui::GetWindowDrawList();
                                    const ImU32 cK7l = IM_COL32(0, 0, 0, 255);
                                    const ImU32 cByd = IM_COL32(255, 255, 255, 255);
                                    // moitie gauche : k7l -> byd
                                    sepDl->AddRectFilledMultiColor(ImVec2(sepP.x, sepP.y),
                                                                  ImVec2(sepP.x + sepW * 0.5f, sepP.y + sepH),
                                                                  cK7l, cByd, cByd, cK7l);
                                    // moitie droite : byd -> k7l
                                    sepDl->AddRectFilledMultiColor(ImVec2(sepP.x + sepW * 0.5f, sepP.y),
                                                                  ImVec2(sepP.x + sepW, sepP.y + sepH),
                                                                  cByd, cK7l, cK7l, cByd);
                                    ImGui::Dummy(ImVec2(sepW, 8.0f));
                                }


                                ImGui::CheckboxRisk("Speed Timer", &g_Globals.Exploits.SpeedHack, "(Risk - Ban)");
                                if (g_Globals.Exploits.SpeedHack)
                                    ImGui::SliderInt("Speed Timer %", &g_Globals.Exploits.SpeedHackPercent, 1, 200, "%d%%");
                                ImGui::KeyBind("Speed Timer Toggle", &g_Globals.Exploits.SpeedHackToggleKeybind, 0);

                                ImGui::CheckboxRisk("Walk Speed", &g_Globals.Exploits.WalkSpeed, "(Risk - Ban)");
                                if (g_Globals.Exploits.WalkSpeed) {
                                    ImGui::SliderFloat("Walk Speed xN", &g_Globals.Exploits.WalkSpeedMult, 1.0f, 60.0f, "%.1fx");
                                    // lecture directe de l'etat d'ecriture : confirme que la
                                    // valeur est bien en memoire (PlayerAttributes+0x1B8/0x1BC)
                                    if (g_Globals.Exploits.WalkSpeedActive && g_Globals.Exploits.WalkSpeedApplied > 1.0f) {
                                        ImGui::TextColored(ImVec4(0.10f, 0.85f, 0.40f, 1.0f),
                                                           "Walk: x%.1f APPLIED",
                                                           (double)g_Globals.Exploits.WalkSpeedApplied);
                                    }
                                    else {
                                        ImGui::TextColored(ImVec4(1.00f, 0.72f, 0.25f, 0.95f),
                                                           "Walk: waiting for a match...");
                                    }
                                }
                                ImGui::KeyBind("Walk Speed Toggle", &g_Globals.Exploits.WalkSpeedToggleKeybind, 0);

                                // Trait : entre "Speed Timer Toggle" et "Magnet Lite" : k7l -> byd -> k7l
                                {
                                    const float sepW = ImGui::GetContentRegionAvail().x;
                                    const float sepH = 2.0f;
                                    const ImVec2 sepP = ImGui::GetCursorScreenPos();
                                    ImDrawList* sepDl = ImGui::GetWindowDrawList();
                                    const ImU32 cK7l = IM_COL32(0, 0, 0, 255);
                                    const ImU32 cByd = IM_COL32(255, 255, 255, 255);
                                    // moitie gauche : k7l -> byd
                                    sepDl->AddRectFilledMultiColor(ImVec2(sepP.x, sepP.y),
                                                                  ImVec2(sepP.x + sepW * 0.5f, sepP.y + sepH),
                                                                  cK7l, cByd, cByd, cK7l);
                                    // moitie droite : byd -> k7l
                                    sepDl->AddRectFilledMultiColor(ImVec2(sepP.x + sepW * 0.5f, sepP.y),
                                                                  ImVec2(sepP.x + sepW, sepP.y + sepH),
                                                                  cByd, cK7l, cK7l, cByd);
                                    ImGui::Dummy(ImVec2(sepW, 8.0f));
                                }


                                ImGui::CheckboxRisk("Magnet Lite", &g_Globals.Exploits.MagnetLite, "(Test)");
                                if (g_Globals.Exploits.MagnetLite) {
                                    ImGui::SliderFloat("Lite FOV##ML", &g_Globals.Exploits.MagnetLiteFov, 50.0f, 500.0f, "%.0f");
                                    ImGui::SliderFloat("Fire Cooldown##ML", &g_Globals.Exploits.MagnetLiteCooldownMs, 20.0f, 500.0f, "%.0f ms");
                                }
                                ImGui::KeyBind("Magnet Lite Toggle", &g_Globals.Exploits.MagnetLiteToggleKeybind, 0);

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
                                ImGui::KeyBind("Toggle Menu", &g_Globals.General.MenuKey, 0);


                                ImGui::Checkbox("Keybind Status Bar", &g_Globals.Misc.StatusPanelVisible);
                                // Combo "Bar Position" SUPPRIMEE : le panneau est LIBRE,
                                // on le deplace a la souris (tirer = glisser).

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
                                static std::string scanStatus = "";
                                // Bouton PLUS GRAND : largeur AUTO = libelle + marge, CENTRE dans la colonne
                                const char* againTxt = "CONNECT FREE FIRE AGAIN";
                                float againW = 260.0f;                      // largeur de secours
                                const float againH = 40.0f;                 // hauteur (etait 22 -> plus grande)
                                {
                                    ImFont* againFont = FunctionFont ? FunctionFont
                                                           : (InterBold12 ? InterBold12 : InterBold);
                                    if (againFont) ImGui::PushFont(againFont);
                                    const float againLabelW = ImGui::CalcTextSize(againTxt).x;
                                    if (againFont) ImGui::PopFont();
                                    if (againLabelW > 1.0f) againW = againLabelW + 48.0f;   // +24px de marge de chaque cote
                                }
                                againW = ImMin(againW, ImGui::GetContentRegionAvail().x);
                                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - againW) * 0.5f);
                                if (ImGui::ButtonStyled(againTxt, ImVec2(againW, againH))) {
                                    Offsets::Il2Cpp = 0; 
                                    scanStatus = "Scanning...";
                                    std::thread([&]() { 
                                        ThreadGuard guard;
                                        if (FWork::ADB::InitializeADB()) scanStatus = "Connected!"; 
                                        else scanStatus = "Failed!";
                                    }).detach();
                                }
                                if (!scanStatus.empty()) {
                                    ImGui::PushFont(InterBold12 ? InterBold12 : InterBold);
                                    ImGui::TextColored(ImVec4(0.1f, 0.85f, 0.4f, 1.0f), "Result: %s", scanStatus.c_str());
                                    ImGui::PopFont();
                                }

                                // Etat du serveur de commandes TCP (demarre a l'injection)
                                {
                                    const bool cmdUp = CmdServer::IsRunning();
                                    ImGui::Dummy(ImVec2(0.0f, 6.0f));
                                    ImGui::Text("COMMAND SERVER:");
                                    ImGui::SameLine();
                                    ImGui::TextColored(cmdUp ? ImVec4(0.1f, 0.85f, 0.4f, 1.0f)
                                                             : ImVec4(1.0f, 0.35f, 0.35f, 0.95f),
                                                       "127.0.0.1:%u  %s",
                                                       (unsigned)CmdServer::Port(),
                                                       cmdUp ? "ONLINE" : "OFFLINE");
                                }
                            }
                            ImGui::EndCustomChild();
                        }
                        ImGui::EndChild();
                    }
                    else if (CurrentTab == 7) // --- AIMKILL (lib UPDATEDOB55) ---
                    {
                        // ==========================================================
                        // Header fixe 78px : plaque logo "AIR" dessinee EN CODE
                        // (aucun asset image) + bouton CONNECT LIB + ligne statut.
                        // ==========================================================
                        ImDrawList* dl = ImGui::GetWindowDrawList();
                        const ImVec2 hdr0 = ImGui::GetCursorScreenPos();
                        const AimKillLink::LinkState akSt = AimKillLink::State();
                        const bool akBusy = AimKillLink::IsBusy();

                        // --- plaque logo 176x56 (fond + bandeau cyan + bordure) ---
                        const ImVec2 pl0 = hdr0;
                        const ImVec2 pl1 = hdr0 + ImVec2(176.0f, 56.0f);
                        dl->AddRectFilled(pl0, pl1, IM_COL32(12, 14, 20, 255), 8.0f);
                        dl->AddRectFilledMultiColor(pl0, ImVec2(pl1.x, pl0.y + 4.0f),
                                                     IM_COL32(34, 211, 238, 255), IM_COL32(37, 99, 235, 255),
                                                     IM_COL32(37, 99, 235, 255), IM_COL32(34, 211, 238, 255),
                                                     8.0f, ImDrawFlags_RoundCornersTop);
                        dl->AddRect(pl0, pl1, IM_COL32(255, 255, 255, 45), 8.0f, 0, 1.0f);

                        // croix de visee FA (cyan)
                        {
                            ImFont* fa = FontAwesomeSolid ? FontAwesomeSolid : InterBold;
                            if (fa) {
                                const char* cross = ICON_FA_CROSSHAIRS;
                                const ImVec2 cs = fa->CalcTextSizeA(18.0f, FLT_MAX, 0.0f, cross);
                                dl->AddText(fa, 18.0f,
                                            ImVec2(pl0.x + 12.0f, pl0.y + 16.0f + (18.0f - cs.y) * 0.5f),
                                            IM_COL32(34, 211, 238, 255), cross);
                            }
                        }
                        // "AIR" (TitleFont 34px) + legende "AIMKILL PANEL"
                        {
                            ImFont* tf = TitleFont ? TitleFont : InterBold;
                            if (tf)
                                dl->AddText(tf, 34.0f, ImVec2(pl0.x + 40.0f, pl0.y + 6.0f),
                                            IM_COL32(245, 248, 255, 255), "AIR");
                            ImFont* cf = InterBold12 ? InterBold12 : InterBold;
                            if (cf)
                                dl->AddText(cf, 9.0f, ImVec2(pl0.x + 42.0f, pl0.y + 42.0f),
                                            IM_COL32(130, 170, 200, 255), "AIMKILL PANEL");
                        }
                        // chip d'etat LIVE / BUSY / OFF
                        {
                            const char* chipTxt;
                            ImU32 chipBg, chipFg;
                            if (akSt == AimKillLink::LinkState::Connected) {
                                chipTxt = "LIVE"; chipBg = IM_COL32(16, 62, 38, 255);  chipFg = IM_COL32(64, 230, 138, 255);
                            } else if (akSt == AimKillLink::LinkState::Error) {
                                chipTxt = "OFF";  chipBg = IM_COL32(66, 22, 22, 255);  chipFg = IM_COL32(255, 92, 92, 255);
                            } else if (akBusy) {
                                chipTxt = "BUSY"; chipBg = IM_COL32(72, 56, 12, 255);  chipFg = IM_COL32(255, 199, 55, 255);
                            } else {
                                chipTxt = "OFF";  chipBg = IM_COL32(30, 33, 42, 255);  chipFg = IM_COL32(150, 156, 170, 255);
                            }
                            const ImVec2 c0(pl1.x - 52.0f, pl1.y - 26.0f);
                            const ImVec2 c1(pl1.x - 8.0f,  pl1.y - 8.0f);
                            dl->AddRectFilled(c0, c1, chipBg, 9.0f);
                            ImFont* cf = InterBold12 ? InterBold12 : InterBold;
                            if (cf) {
                                const ImVec2 ts = cf->CalcTextSizeA(9.0f, FLT_MAX, 0.0f, chipTxt);
                                dl->AddText(cf, 9.0f,
                                            ImVec2((c0.x + c1.x - ts.x) * 0.5f, (c0.y + c1.y - ts.y) * 0.5f),
                                            chipFg, chipTxt);
                            }
                        }

                        // --- bouton CONNECT LIB (desactive pendant le pipeline) ---
                        const char* akBtn = "CONNECT LIB";
                        ImFont* btnFont = FunctionFont ? FunctionFont : (InterBold12 ? InterBold12 : InterBold);
                        float btnW = 168.0f;
                        if (btnFont) {
                            ImGui::PushFont(btnFont);
                            btnW = ImGui::CalcTextSize(akBtn).x + 48.0f;
                            ImGui::PopFont();
                        }
                        ImGui::SetCursorPos(ImVec2(cw - btnW, 8.0f));
                        if (ImGui::ButtonStyled(akBtn, ImVec2(btnW, 40.0f), akBusy))
                            AimKillLink::StartConnect();

                        // --- ligne de statut coloree pleine largeur ---
                        ImVec4 stCol;
                        switch (akSt) {
                        case AimKillLink::LinkState::Connected: stCol = ImVec4(0.10f, 0.85f, 0.40f, 1.0f); break;
                        case AimKillLink::LinkState::Error:     stCol = ImVec4(1.00f, 0.35f, 0.35f, 1.0f); break;
                        case AimKillLink::LinkState::Busy:      stCol = ImVec4(1.00f, 0.78f, 0.20f, 1.0f); break;
                        default:                                stCol = ImVec4(0.62f, 0.65f, 0.72f, 1.0f); break;
                        }
                        ImGui::SetCursorPos(ImVec2(0.0f, 58.0f));
                        const std::string akStatus = AimKillLink::Status();
                        ImGui::TextColored(stCol, "STATUS: %s", akStatus.c_str());

                        // ==========================================================
                        // Zone scrollable (molette) : 3 cartes empilees
                        // AIM(16 lignes) / BRTL(18 lignes) / KEYBIND(6 lignes)
                        // ==========================================================
                        ImGui::SetCursorPos(ImVec2(0.0f, 78.0f));
                        ImGui::BeginChild("AkScroll", ImVec2(cw, ch - 78.0f), false,
                                          ImGuiWindowFlags_AlwaysVerticalScrollbar);
                        {
                            auto AkCard = [&](const char* title, float bodyH, auto&& body) {
                                ImGui::CustomChild(title, ImVec2(ImGui::GetContentRegionAvail().x, 38.0f + bodyH));
                                body();
                                ImGui::EndCustomChild();   // + 14px
                            };
                            // checkbox qui propage au lib (mode wire, pas l'id menu)
                            auto Lib = [](const char* label, bool* v, int mode) {
                                if (ImGui::Checkbox(label, v))
                                    AimKillLink::SendToggle(mode, *v);
                            };

                            // ---------------- AIM : 15 cases + AIMFOV ----------------
                            AkCard("AIM", 990.0f, [&]() {
                                if (ImGui::Checkbox("Activate All", &g_Globals.AimKill.ActivateAll)) {
                                    AimKillLink::SendToggle(3, g_Globals.AimKill.ActivateAll);
                                    if (g_Globals.AimKill.ActivateAll) {  // macro Menu.java id 102
                                        g_Globals.Visuals.Lines = true;
                                        g_Globals.Visuals.Box = true;
                                        g_Globals.Visuals.HealthBar = true;
                                        g_Globals.Visuals.Name = true;
                                        g_Globals.Visuals.Distance = true;
                                        g_Globals.Misc.ShowAimbotFov = true;
                                    }
                                }
                                Lib("Aimkill Send", &g_Globals.AimKill.AimkillSend, 1044);
                                Lib("Silent Kill", &g_Globals.AimKill.SilentKill, 889);
                                Lib("Send All Fov", &g_Globals.AimKill.SendAllFov, 1050);
                                Lib("SAFE AIMKILL", &g_Globals.AimKill.SafeAimkill, 5661);
                                Lib("REAL AIMKILL V2", &g_Globals.AimKill.RealAimkillV2, 5662);
                                Lib("AUTO SWITCH SEND", &g_Globals.AimKill.AutoSwitchSend, 1047);
                                Lib("AUTO SWITCH NORMAL", &g_Globals.AimKill.AutoSwitchNormal, 508);
                                Lib("COVER ELIMINATION", &g_Globals.AimKill.CoverElimination, 1048);
                                Lib("AIMKILL NO SWITCH", &g_Globals.AimKill.AimkillNoSwitch, 1049);
                                Lib("HIDE DAMAGE", &g_Globals.AimKill.HideDamage, 527);
                                Lib("COVER PULL", &g_Globals.AimKill.CoverPull, 509);
                                Lib("AIM LOCK", &g_Globals.AimKill.AimLock, 34834);
                                ImGui::Checkbox("TARGET LIST OVERLAY", &g_Globals.AimKill.TargetListOverlay);
                                if (ImGui::IsItemHovered())
                                    ImGui::SetTooltip("Overlay de la liste cibles (menu APK)\nSans effet sous 0xVoid");
                                Lib("TARGET LOCK", &g_Globals.AimKill.TargetLock, 523);
                                ImGui::SliderInt("AIMFOV", &g_Globals.AimKill.AimFov, 0, 1000, "%dx");
                                if (ImGui::IsItemDeactivatedAfterEdit())
                                    AimKillLink::SendFov(g_Globals.AimKill.AimFov);
                            });

                            // ------- BRTL : 7 ESP natives 0xVoid + 11 options lib -------
                            AkCard("BRTL", 1140.0f, [&]() {
                                ImGui::Checkbox("ESP LINE", &g_Globals.Visuals.Lines);
                                ImGui::Checkbox("ESP BOX", &g_Globals.Visuals.Box);
                                ImGui::Checkbox("ESP HEALTH", &g_Globals.Visuals.HealthBar);
                                ImGui::Checkbox("ESP NAME", &g_Globals.Visuals.Name);
                                ImGui::Checkbox("ESP WEAPON NAME", &g_Globals.Visuals.ESPWeapon);
                                ImGui::Checkbox("ESP DISTANCE", &g_Globals.Visuals.Distance);
                                ImGui::Checkbox("DRAW FOV", &g_Globals.Misc.ShowAimbotFov);
                                Lib("INVINSO HACK", &g_Globals.AimKill.InvinsHack, 532);
                                Lib("AUTOFIRE ALL GUNS", &g_Globals.AimKill.AutofireAllGuns, 521);
                                Lib("DOWN KILL", &g_Globals.AimKill.DownKill, 504);
                                Lib("DIVE KILL NEW", &g_Globals.AimKill.DiveKillNew, 5660);
                                Lib("BRUST FIRE", &g_Globals.AimKill.BrustFire, 515);
                                Lib("RAPID FIRE", &g_Globals.AimKill.RapidFire, 518);
                                Lib("FAST RELOAD", &g_Globals.AimKill.FastReload, 519);
                                Lib("FLY X40", &g_Globals.AimKill.FlyX40, 5659);
                                Lib("STABLE FLY", &g_Globals.AimKill.StableFly, 5002);
                                Lib("TELEPORT MARK", &g_Globals.AimKill.TeleportMark, 56578);
                                Lib("SEND POSITION", &g_Globals.AimKill.SendPosition, 5663);
                            });

                            // ---------- KEYBIND : 5 cases + bouton reset ----------
                            AkCard("KEYBIND", 390.0f, [&]() {
                                ImGui::Checkbox("KEYBINDS ENABLE", &g_Globals.Misc.KeybindsEnabled);
                                ImGui::Checkbox("SHOW KEYBIND STATUS", &g_Globals.Misc.StatusPanelVisible);
                                if (ImGui::ButtonStyled("CLEAR ALL KEYBINDS",
                                                        ImVec2(ImGui::GetContentRegionAvail().x - 20.0f, 40.0f))) {
                                    // 30 raccourcis -> 0. Les touches d'activation
                                    // (*Bind / *Key) sont conservees.
                                    g_Globals.AimBot.ExtremeToggleKeybind = 0;
                                    g_Globals.AimBot.FastReloadToggleKeybind = 0;
                                    g_Globals.AimBot.RapidFireToggleKeybind = 0;
                                    g_Globals.AimBot.AimbotToggleKeybind = 0;
                                    g_Globals.AimBot.AimbotTypeKeybind = 0;
                                    g_Globals.AimBot.MouseAimToggleKeybind = 0;
                                    g_Globals.AimBot.RubixToggleKeybind = 0;
                                    g_Globals.AimBot.SilentLegitToggleKeybind = 0;
                                    g_Globals.AimBot.SilentAimV2ToggleKeybind = 0;
                                    g_Globals.Exploits.InfinitySkillToggleKeybind = 0;
                                    g_Globals.Exploits.FlyHackIntToggleKeybind = 0;
                                    g_Globals.Exploits.KcNoGravityFlyToggleKeybind = 0;
                                    g_Globals.Exploits.FakeLag_TeleToggleKeybind = 0;
                                    g_Globals.Exploits.FakeLag_FreezeToggleKeybind = 0;
                                    g_Globals.Exploits.FakeLag_GhostToggleKeybind = 0;
                                    g_Globals.Exploits.ShakeKillToggleKeybind = 0;
                                    g_Globals.Exploits.SpeedHackToggleKeybind = 0;
                                    g_Globals.Exploits.MagnetEnemyToggleKeybind = 0;
                                    g_Globals.Exploits.EnemyPullToggleKeybind = 0;
                                    g_Globals.Exploits.InvisibleToggleKeybind = 0;
                                    g_Globals.Exploits.AntiInvisibleToggleKeybind = 0;
                                    g_Globals.Exploits.MagnetLiteToggleKeybind = 0;
                                    g_Globals.Exploits.PushForwardToggleKeybind = 0;
                                    g_Globals.Exploits.WalkSpeedToggleKeybind = 0;
                                    g_Globals.Exploits.FakeDameToggleKeybind = 0;
                                    g_Globals.Visuals.RefreshEspToggleKeybind = 0;
                                    g_Globals.Visuals.FixESPToggleKeybind = 0;
                                    g_Globals.General.forcePartkeybind = 0;
                                    g_Globals.General.ForceAimToggleKeybind = 0;
                                    g_Globals.General.divekillToggleKeybind = 0;
                                }
                                ImGui::Checkbox("SHOW INFO HUD", &g_Globals.Visuals.Watermark);
                                Lib("FLYHACK [MINI]", &g_Globals.AimKill.FlyhackMini, 5657);
                                Lib("HIGH FPS", &g_Globals.AimKill.HighFps, 529);
                            });
                        }
                        ImGui::EndChild();
                    }
                    else if (CurrentTab == 6) // --- INFO ---
                    {
                        ImGui::CustomChild("Account & System", ImVec2(cw, ch));
                        {
                            ImGui::Text("STATUS:"); ImGui::SameLine();
                            ImGui::TextColored(ImVec4(0.1f, 0.85f, 0.4f, 1.0f), "ACTIVE / SECURE");


                            ImGui::Text("VERSION:     0x Void - Core - Internal V.1 Safe");
                            ImGui::Text("THEME:       Pearl White & Obsidian Glass");
                            ImGui::Text("BUILD DATE:  %s", __DATE__);
                            ImGui::Text("BUILD TIME:  %s", __TIME__);

                            ImGui::TextColored(ImVec4(0.953f, 0.957f, 0.965f, 1.0f), "Devloped By :   @4slash");
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
            if (!g_Globals.Misc.KeybindsEnabled) return;   // KEYBINDS ENABLE (tab AIMKILL)
            if (bind == 0) return;
            items.push_back({ name, bind, featureOn });
        };

        TryAdd("Fast Reload", g_Globals.AimBot.FastReloadToggleKeybind, g_Globals.AimBot.FastReload);
        TryAdd("Silent Aim", g_Globals.AimBot.SilentAimV2ToggleKeybind, g_Globals.AimBot.SilentAimV2);
       // TryAdd("Force Aim", g_Globals.General.ForceAimToggleKeybind, g_Globals.General.ForceonAim);
       // TryAdd("Mouse Aim", g_Globals.AimBot.MouseAimToggleKeybind, g_Globals.AimBot.MouseAim);
        TryAdd("Ghost Lag", g_Globals.Exploits.FakeLag_TeleToggleKeybind, g_Globals.Exploits.FakeLag_Tele);
        TryAdd("Freeze", g_Globals.Exploits.FakeLag_FreezeToggleKeybind, g_Globals.Exploits.FakeLag_Freeze);
        TryAdd("Ghost", g_Globals.Exploits.FakeLag_GhostToggleKeybind, g_Globals.Exploits.FakeLag_Ghost);
       // TryAdd("Infinite Skill", g_Globals.Exploits.InfinitySkillToggleKeybind, g_Globals.Exploits.InfinitySkill);
        TryAdd("Speed Timer", g_Globals.Exploits.SpeedHackToggleKeybind, g_Globals.Exploits.SpeedHack);
        TryAdd("Walk Speed", g_Globals.Exploits.WalkSpeedToggleKeybind, g_Globals.Exploits.WalkSpeed);
        TryAdd("Fake Damage", g_Globals.Exploits.FakeDameToggleKeybind, g_Globals.Exploits.FakeDame);
        TryAdd("Pull Enemy", g_Globals.Exploits.PullEnemyBind, g_Globals.Exploits.PullEnemy);
        TryAdd("Magnet Lite", g_Globals.Exploits.MagnetLiteToggleKeybind, g_Globals.Exploits.MagnetLite);

        int enemyCount = 0;
        {
            SpinLockGuard lock(g_Globals.EspConfig.EntitiesLock);
            for (auto& [id, p] : g_Globals.EspConfig.Entities) {
                (void)id;
                if (!p.IsDead) enemyCount++;
            }
        }

        ImU32 titleCol = IM_COL32(255, 255, 255, 255); // Pearl White

        // ------------------------------------------------------------------
        // KEYBinds - design "obsidian glass" :
        //   header  : icone clavier + KEYBINDS + badge compteur (x/y)
        //   lignes  : pastille LED a halo pulse + trait d'accent + touche 3D
        //   footer  : pillule "Enemies : n"
        // ------------------------------------------------------------------
        const float panelW = 198.0f;
        const float rowH   = 27.0f;
        const float headH  = 36.0f;
        const float footH  = 32.0f;
        const float corner = 10.0f;
        const float margin = 11.0f;
        const float pad    = 6.0f;
        float panelH = headH + items.size() * rowH + pad + footH;

        ImVec2 screen = ImGui::GetIO().DisplaySize;
        if (screen.x > 0 && screen.y > 0) {
            // --- MODE LIBRE : plus d'ancrage (Top/Right/Left/Bottom) SUPPRIME,
            //     le panneau se DEPLACE a la souris : cliquer + tirer n'importe ou.
            {
                const ImVec2& m = ImGui::GetIO().MousePos;
                const float x0 = g_Globals.Misc.StatusPanelX;
                const float y0 = g_Globals.Misc.StatusPanelY;
                const bool inside = (m.x >= x0 - 7.0f) && (m.x <= x0 + panelW + 7.0f) &&
                                    (m.y >= y0 - 7.0f) && (m.y <= y0 + panelH + 7.0f);
                static bool s_drag = false;
                if (inside && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) s_drag = true;
                if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))             s_drag = false;
                if (s_drag) {
                    g_Globals.Misc.StatusPanelX += ImGui::GetIO().MouseDelta.x;
                    g_Globals.Misc.StatusPanelY += ImGui::GetIO().MouseDelta.y;
                }
            }

            g_Globals.Misc.StatusPanelX = (std::max)(0.0f, (std::min)(g_Globals.Misc.StatusPanelX, screen.x - panelW));
            g_Globals.Misc.StatusPanelY = (std::max)(0.0f, (std::min)(g_Globals.Misc.StatusPanelY, screen.y - panelH));
        }

        ImVec2 pos(g_Globals.Misc.StatusPanelX, g_Globals.Misc.StatusPanelY);
        // FOREGROUND : le panneau se dessine DESSUS tout -> TOUJOURS visible et GLISSABLE
        // (en BACKGROUND il restait cache sous le menu -> impossible a attraper)
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        const ImVec2 panelMax(pos.x + panelW, pos.y + panelH);
        const float tNow = (float)ImGui::GetTime();

        int activeBinds = 0;
        for (const auto& it : items) if (it.featureOn) ++activeBinds;

        // ===== CONTENANT : obsidian glass + halo fin =====
        dl->AddRectFilled(ImVec2(pos.x - 7.0f, pos.y - 7.0f),
                          ImVec2(panelMax.x + 7.0f, panelMax.y + 7.0f),
                          IM_COL32(243, 244, 246, 16), corner + 5.0f);
        dl->AddRectFilledMultiColor(pos, panelMax,
            IM_COL32(18, 19, 27, 255), IM_COL32(18, 19, 27, 255),
            IM_COL32(9, 10, 14, 255),  IM_COL32(9, 10, 14, 255), corner);

        // ===== FOOTER (fond, avant les liseres) =====
        float footSepY = pos.y + panelH - footH;
        dl->AddRectFilled(ImVec2(pos.x, footSepY), panelMax,
                          IM_COL32(12, 13, 19, 255), corner, ImDrawFlags_RoundCornersBottom);

        // ===== HEADER (fond) =====
        dl->AddRectFilled(pos, ImVec2(pos.x + panelW, pos.y + headH),
                          IM_COL32(15, 16, 24, 255), corner, ImDrawFlags_RoundCornersTop);

        // liseres de separation (centre vif, bords fades)
        auto Seam = [&](float y) {
            dl->AddRectFilledMultiColor(ImVec2(pos.x + 10.0f, y - 1.0f),
                                        ImVec2(panelMax.x - 10.0f, y + 1.0f),
                IM_COL32(243, 244, 246, 0),
                IM_COL32(255, 255, 255, 190),
                IM_COL32(255, 255, 255, 190),
                IM_COL32(243, 244, 246, 0));
        };
        Seam(pos.y + headH);
        Seam(footSepY);

        // ===== HEADER : icone + titre + badge compteur =====
        ImGui::PushFont(FontAwesomeSolid);
        const char* kbIcon = ICON_FA_KEYBOARD;
        ImVec2 iconSz = ImGui::CalcTextSize(kbIcon);
        ImGui::PopFont();

        ImGui::PushFont(InterBold);
        const char* kbTitle = "KEYBINDS";
        ImVec2 titleSz = ImGui::CalcTextSize(kbTitle);
        ImGui::PopFont();

        const float hdrMidY = pos.y + headH * 0.5f;
        const float iconX = pos.x + margin;

        ImGui::PushFont(FontAwesomeSolid);
        dl->AddText(ImVec2(iconX, hdrMidY - iconSz.y * 0.5f + 1.0f), IM_COL32(243, 244, 246, 55), kbIcon);
        dl->AddText(ImVec2(iconX, hdrMidY - iconSz.y * 0.5f), titleCol, kbIcon);
        ImGui::PopFont();

        const float titleX = iconX + iconSz.x + 7.0f;
        ImGui::PushFont(InterBold);
        dl->AddText(ImVec2(titleX + 1.0f, hdrMidY - titleSz.y * 0.5f + 1.0f), IM_COL32(243, 244, 246, 70), kbTitle);
        dl->AddText(ImVec2(titleX, hdrMidY - titleSz.y * 0.5f), IM_COL32(255, 255, 255, 255), kbTitle);
        ImGui::PopFont();

        // badge compteur "actifs/total"
        {
            std::string cnt = std::to_string(activeBinds) + "/" + std::to_string((int)items.size());
            ImFont* cntFont = InterBold12 ? InterBold12 : InterBold;

            ImGui::PushFont(cntFont);
            ImVec2 csz = ImGui::CalcTextSize(cnt.c_str());
            ImGui::PopFont();

            const float bw = csz.x + 14.0f;
            const float bh = 18.0f;
            const float bx = pos.x + panelW - margin - bw;
            const float by = hdrMidY - bh * 0.5f;

            dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + bw, by + bh), IM_COL32(243, 244, 246, 26), bh * 0.5f);
            dl->AddRect(ImVec2(bx, by), ImVec2(bx + bw, by + bh),
                        activeBinds > 0 ? IM_COL32(16, 185, 129, 190) : IM_COL32(243, 244, 246, 70),
                        bh * 0.5f, 0, 1.0f);

            ImGui::PushFont(cntFont);
            dl->AddText(ImVec2(bx + (bw - csz.x) * 0.5f, by + (bh - csz.y) * 0.5f),
                        IM_COL32(243, 244, 246, 255), cnt.c_str());
            ImGui::PopFont();
        }

        // ===== LIGNES =====
        float itemY = pos.y + headH + pad;
        for (int idx = 0; idx < (int)items.size(); ++idx) {
            const auto& item = items[idx];
            const std::string keyStr = VKName(item.bind);

            const ImVec2 rowMin(pos.x + 5.0f, itemY);
            const ImVec2 rowMax(pos.x + panelW - 5.0f, itemY + rowH - 3.0f);

            // fond de ligne : alterne, teinte verte si la fonction est ON
            if (item.featureOn)
                dl->AddRectFilled(rowMin, rowMax, IM_COL32(16, 185, 129, 30), 7.0f);
            else if ((idx % 2) == 0)
                dl->AddRectFilled(rowMin, rowMax, IM_COL32(255, 255, 255, 11), 7.0f);

            // trait d'accent a gauche (vert ON / gris OFF)
            dl->AddRectFilled(ImVec2(rowMin.x + 3.0f, rowMin.y + 5.0f),
                              ImVec2(rowMin.x + 5.5f, rowMax.y - 5.0f),
                              item.featureOn ? IM_COL32(16, 185, 129, 255) : IM_COL32(75, 85, 99, 170), 1.5f);

            // pastille LED (halo qui pulse si ON)
            const ImVec2 ledC(pos.x + margin + 7.0f, (rowMin.y + rowMax.y) * 0.5f);
            if (item.featureOn) {
                const float ph = fmodf(tNow * 1.7f, 1.0f);
                const float pulse = ph < 0.5f ? ph * 2.0f : (1.0f - ph) * 2.0f;
                dl->AddCircleFilled(ledC, 8.5f, IM_COL32(16, 185, 129, (int)(30 + 40 * pulse)), 20);
                dl->AddCircleFilled(ledC, 5.2f, IM_COL32(16, 185, 129, 110), 16);
                dl->AddCircleFilled(ledC, 3.0f, IM_COL32(52, 211, 153, 255), 12);
                dl->AddCircle(ledC, 4.3f, IM_COL32(16, 185, 129, (int)(110 + 110 * pulse)), 16, 1.2f);
            } else {
                dl->AddCircleFilled(ledC, 6.0f, IM_COL32(75, 85, 99, 45), 16);
                dl->AddCircleFilled(ledC, 3.0f, IM_COL32(105, 112, 128, 220), 12);
            }

            // nom de la fonction
            ImGui::PushFont(InterRegular);
            const float nameH = ImGui::GetFontSize();
            const ImU32 nameColor = item.featureOn ? IM_COL32(245, 246, 250, 255) : IM_COL32(140, 146, 162, 215);
            dl->AddText(ImVec2(pos.x + margin + 17.0f, (rowMin.y + rowMax.y) * 0.5f - nameH * 0.5f),
                        nameColor, item.name.c_str());
            ImGui::PopFont();

            // touche : keycap 3D (corps sombre + sommet clair + ombre)
            ImFont* keyFont = InterBold12 ? InterBold12 : InterBold;
            ImGui::PushFont(keyFont);
            ImVec2 keySz = ImGui::CalcTextSize(keyStr.c_str());

            const float capW = keySz.x + 14.0f;
            const float capH = 18.0f;
            const float capX = pos.x + panelW - margin - capW;
            const float capY = (rowMin.y + rowMax.y) * 0.5f - capH * 0.5f;
            const ImVec2 capMin(capX, capY), capMax(capX + capW, capY + capH);

            dl->AddRectFilled(ImVec2(capMin.x, capMin.y + 2.0f),
                              ImVec2(capMax.x, capMax.y + 2.0f), IM_COL32(0, 0, 0, 150), 5.0f);
            dl->AddRectFilled(capMin, capMax, IM_COL32(24, 26, 36, 255), 5.0f);
            dl->AddRectFilled(ImVec2(capMin.x + 1.5f, capMin.y + 1.5f),
                              ImVec2(capMax.x - 1.5f, capMin.y + 8.0f), IM_COL32(40, 43, 58, 190), 4.0f);
            dl->AddRect(capMin, capMax,
                        item.featureOn ? IM_COL32(243, 244, 246, 240) : IM_COL32(56, 60, 82, 220),
                        5.0f, 0, item.featureOn ? 1.3f : 1.0f);
            const ImU32 keyColor = item.featureOn ? titleCol : IM_COL32(156, 163, 175, 220);
            dl->AddText(ImVec2(capMin.x + (capW - keySz.x) * 0.5f, capMin.y + (capH - keySz.y) * 0.5f),
                        keyColor, keyStr.c_str());
            ImGui::PopFont();

            itemY += rowH;
        }

        // ===== FOOTER : pillule "Enemies : n" =====
        {
            ImFont* fFont = InterBold12 ? InterBold12 : InterBold;
            ImGui::PushFont(fFont);

            std::string numStr = std::to_string(enemyCount);
            const char* prefTxt = "Enemies";
            ImVec2 prefSz = ImGui::CalcTextSize(prefTxt);
            ImVec2 numSz = ImGui::CalcTextSize(numStr.c_str());

            const float pillH = 19.0f;
            const float pillW = 25.0f + prefSz.x + 7.0f + numSz.x;
            const float pillX = pos.x + (panelW - pillW) * 0.5f;
            const float pillY = footSepY + (footH - pillH) * 0.5f;

            dl->AddRectFilled(ImVec2(pillX, pillY), ImVec2(pillX + pillW, pillY + pillH),
                              IM_COL32(243, 244, 246, 22), pillH * 0.5f);
            dl->AddRect(ImVec2(pillX, pillY), ImVec2(pillX + pillW, pillY + pillH),
                        IM_COL32(243, 244, 246, 65), pillH * 0.5f, 0, 1.0f);
            dl->AddCircleFilled(ImVec2(pillX + 11.0f, pillY + pillH * 0.5f), 3.2f,
                                enemyCount > 0 ? IM_COL32(244, 63, 94, 255) : IM_COL32(75, 85, 99, 220), 12);

            float tx = pillX + 19.0f;
            dl->AddText(ImVec2(tx, pillY + (pillH - prefSz.y) * 0.5f), IM_COL32(156, 163, 175, 235), prefTxt);
            tx += prefSz.x + 7.0f;
            dl->AddText(ImVec2(tx, pillY + (pillH - numSz.y) * 0.5f), titleCol, numStr.c_str());
            ImGui::PopFont();
        }

        // ===== LISERE QUI BOUCLE (tout dernier : passe au-dessus) =====
        DrawMovingBorder(dl, pos, panelMax, corner);
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
            if (!g_Globals.Misc.KeybindsEnabled) { keyDown = false; return; }  // KEYBINDS ENABLE
            if (vk == 0) return;
            if (GetAsyncKeyState(vk) & 0x8000) {
                if (!keyDown) value = !value;
                keyDown = true;
            } else {
                keyDown = false;
            }
        };

        static bool fastReloadToggleKeyDown = false;
        toggleBoolKey(g_Globals.AimBot.FastReloadToggleKeybind, g_Globals.AimBot.FastReload, fastReloadToggleKeyDown);

        static bool mouseToggleKeyDown = false;
        toggleBoolKey(g_Globals.AimBot.MouseAimToggleKeybind, g_Globals.AimBot.MouseAim, mouseToggleKeyDown);

        static bool teleToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.FakeLag_TeleToggleKeybind, g_Globals.Exploits.FakeLag_Tele, teleToggleKeyDown);

        static bool freezeToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.FakeLag_FreezeToggleKeybind, g_Globals.Exploits.FakeLag_Freeze, freezeToggleKeyDown);

        static bool ghostToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.FakeLag_GhostToggleKeybind, g_Globals.Exploits.FakeLag_Ghost, ghostToggleKeyDown);

        static bool infinitySkillToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.InfinitySkillToggleKeybind, g_Globals.Exploits.InfinitySkill, infinitySkillToggleKeyDown);

        static bool speedHackToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.SpeedHackToggleKeybind, g_Globals.Exploits.SpeedHack, speedHackToggleKeyDown);

        static bool walkSpeedToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.WalkSpeedToggleKeybind, g_Globals.Exploits.WalkSpeed, walkSpeedToggleKeyDown);

        static bool fakeDameToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.FakeDameToggleKeybind, g_Globals.Exploits.FakeDame, fakeDameToggleKeyDown);

        static bool magnetLiteToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.MagnetLiteToggleKeybind, g_Globals.Exploits.MagnetLite, magnetLiteToggleKeyDown);

        static bool pushForwardToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.PushForwardToggleKeybind, g_Globals.Exploits.PushForward, pushForwardToggleKeyDown);

        static bool enemyPullToggleKeyDown = false;
        toggleBoolKey(g_Globals.Exploits.EnemyPullToggleKeybind, g_Globals.Exploits.EnemyPullEnabled, enemyPullToggleKeyDown);

        static bool silentAimV2ToggleKeyDown = false;
        toggleBoolKey(g_Globals.AimBot.SilentAimV2ToggleKeybind, g_Globals.AimBot.SilentAimV2, silentAimV2ToggleKeyDown);

        static bool refreshEspToggleKeyDown = false;
        static bool prevRefreshEsp = g_Globals.Visuals.RefreshEsp;
        toggleBoolKey(g_Globals.Visuals.RefreshEspToggleKeybind, g_Globals.Visuals.RefreshEsp, refreshEspToggleKeyDown);
        if (g_Globals.Visuals.RefreshEsp && !prevRefreshEsp)
            FWork::Data::Refresh();
        prevRefreshEsp = g_Globals.Visuals.RefreshEsp;

        static bool fixEspToggleKeyDown = false;
        toggleBoolKey(g_Globals.Visuals.FixESPToggleKeybind, g_Globals.Visuals.FixESP, fixEspToggleKeyDown);

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
        if (GetAsyncKeyState(g_Globals.General.MenuKey) & 0x8000)
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
