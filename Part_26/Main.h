
bool FlyHackin = false;
bool bSpeedHackEnabled = false;
bool bSpeedHackActive = false;
bool fastfire = false;
static bool lastFastFireState = false; // Track previous state
bool RapidFireMAX = false;
bool RapidFireActiveMAX = false;

bool DamageHackMAX = false;
bool DamageHackActiveMAX = false;


void RainEffect()
{
    static ImVec2 drops[100];
    static float speed[100];

    ImVec2 screen_size = { (float)GetSystemMetrics(SM_CXSCREEN), (float)GetSystemMetrics(SM_CYSCREEN) };

    for (int i = 0; i < 100; i++)
    {
        if (drops[i].y == 0)
        {
            drops[i] = ImVec2(rand() % (int)screen_size.x, rand() % (int)screen_size.y);
            speed[i] = 5 + rand() % 10;
        }

        drops[i].y += speed[i];

        if (drops[i].y > screen_size.y)
            drops[i].y = 0;

        ImGui::GetWindowDrawList()->AddLine(drops[i], drops[i] + ImVec2(0, 10), ImGui::GetColorU32(c::accent), 2.0f);
    }
}

namespace texture
{
    ID3D11ShaderResourceView* esp_preview = nullptr;
    ID3D11ShaderResourceView* incono = nullptr;
    ID3D11ShaderResourceView* hora = nullptr;
    ID3D11ShaderResourceView* king = nullptr;
    ID3D11ShaderResourceView* user_input = nullptr;
    ID3D11ShaderResourceView* key_input = nullptr;
    ID3D11ShaderResourceView* key = nullptr;
}

#define min(x,y) ((x)<(y)?(x):(y))
#define wh(a) ImColor(1.f,1.f,1.f,a)
const float changing_speed = 0.15f;


ImColor GetColorWithAlpha(ImColor color, float alpha)
{
    return ImColor(color.Value.x, color.Value.y, color.Value.z, alpha);
}
void FXRain(ImDrawList* d, ImVec2 a, ImVec2 b, ImVec2 sz, ImVec2, float t)
{
    for (int i = 0; i < 2000; ++i) {
        unsigned h = ImGui::GetID((void*)(intptr_t)(d + i + int(t / 4)));
        auto f = fmodf(t + fmodf(h / 777.f, 99), 99);
        auto tx = h % (int)sz.x;
        auto ty = h % (int)sz.y;

        if (f < 1) {
            auto py = ty - 1000 * (1 - f);

            // Создаем квадрат из 4 точек вместо линии
            float line_length = min(py + 10, ty) - py;
            ImVec2 points[4] = {
                { a.x + tx - 1.0f, a.y + py },          // левая верхняя
                { a.x + tx + 1.0f, a.y + py },          // правая верхняя
                { a.x + tx + 1.0f, a.y + py + line_length }, // правая нижняя
                { a.x + tx - 1.0f, a.y + py + line_length }  // левая нижняя
            };

            // Рисуем тень для квадрата
            d->AddShadowConvexPoly(
                points,
                4,
                main_color,
                45.f,
                ImVec2(0, 0),
                ImDrawFlags_None
            );

            // Рисуем саму линию (теперь прямоугольник)
            d->AddConvexPolyFilled(points, 4, main_color);
        }
        else if (f < 1.2f) {

            d->AddShadowCircle({ a.x + tx, a.y + ty },
                1.1f + (f - 1) * 10 + h % 5,
                GetColorWithAlpha(main_color, 0.5f), 45.f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground, 36);

            d->AddCircle({ a.x + tx, a.y + ty },
                1.1f + (f - 1) * 10 + h % 5,
                GetColorWithAlpha(main_color, 0.5f));


        }
    }
}


namespace image
{
    ID3D11ShaderResourceView* bell_notify = nullptr;
}

int page = 0;

static float tab_alpha = 0.f; /* */ static float tab_add; /* */ static int active_tab = 0;
bool enable_preview = true;

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace ImGui;

DWORD picker_flags = ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview;

#include <imgui.h>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")





HWND hwnd;
RECT rc;
inline ImGuiWindow* main_window;

void move_window()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 10.0f;
    GetWindowRect(hwnd, &rc);
    float new_x = roundf(rc.left + ImGui::GetWindowPos().x);
    float new_y = roundf(rc.top + ImGui::GetWindowPos().y);
    MoveWindow(hwnd, static_cast<int>(new_x), static_cast<int>(new_y), 929, 535, TRUE);
    ImGui::SetWindowPos(ImVec2(0.f, 0.f));
}



