#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "imgui_edited.hpp"
#include <dwmapi.h>
#include <TlHelp32.h>

#include <cmath>
#include <TlHelp32.h>
#include "imgui_freetype.h"
#include <d3d11.h>
#include <direct.h>
#include <mutex>
#include <ShObjIdl_core.h>
#include <tchar.h>
#include "font.h"
#include "texture.h"
#include <vector>
#include <string>
#include <iostream>
#include <ctime>
#include <random>
#include <chrono>
#include <thread>
#include <windows.h>
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
#include "Particulas.h"

#include"Mem/MemoryF.h"

#include <vector>
#include <string>
#include <time.h>
#include <thread>
#pragma comment(lib,"Wininet.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ntdll.lib")
#include <filesystem>
#include <dwmapi.h>

#include <algorithm>
#include <sstream>
#include "Main.h"
#include <sysinfoapi.h>


#include "ESP/ESP LINES/Offset.hpp"
#include "ESP/ESP LINES/Unity.hh"
#include "ESP/ESP LINES/ADBZapi.hpp"
#include <examples/example_win32_directx11/ESP/ESP LINES/Complementos.h>
#include "ESP/ESP LINES/NoGravityFly.hpp"

#include "ESP/MinHook/include/MinHook.h"

#include "FakeLag/windivert.h"
#include "FakeLag/FakeLag.h"
#include "FakeLag/AimLag.h"

#include"Mem/memNzxxn.h"
#include"Mem/Memory2.h"

#include"MemNxxzn/MemNxxzn.hpp"
#include "icons.h"
#include "NotificationNew.h"
#include "Mem/externalmem.h"
#include "auth/auth.hpp"
#include "auth/skStr.h"
#include "auth/utils.hpp"
#include "auth/wnetwrap.h"
#include <atomic>






#pragma comment(lib,"Wininet.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ntdll.lib")

#pragma comment(lib, "libcpmt.lib")
#pragma comment(lib, "msvcprt.lib")   

struct D3DX11_IMAGE_LOAD_INFO {
    UINT Width, Height, Depth;
    UINT FirstMipLevel, MipLevels, Usage, BindFlags, CpuAccessFlags, MiscFlags;
    DXGI_FORMAT Format; UINT Filter, MipFilter;
    D3DX11_IMAGE_LOAD_INFO() { memset(this, 0, sizeof(*this)); Usage = D3D11_USAGE_DEFAULT; BindFlags = D3D11_BIND_SHADER_RESOURCE; Format = DXGI_FORMAT_UNKNOWN; }
};
struct ID3DX11ThreadPump;

static HRESULT D3DX11CreateShaderResourceViewFromMemory(
    ID3D11Device* device, LPCVOID data, SIZE_T dataSize,
    D3DX11_IMAGE_LOAD_INFO*, ID3DX11ThreadPump*,
    ID3D11ShaderResourceView** outSRV, HRESULT*)
{
    *outSRV = nullptr;
    IWICImagingFactory* factory = nullptr;
    IWICStream* stream = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* conv = nullptr;

    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (FAILED(hr)) return hr;
    hr = factory->CreateStream(&stream);
    if (FAILED(hr)) { factory->Release(); return hr; }
    hr = stream->InitializeFromMemory((BYTE*)data, (DWORD)dataSize);
    if (FAILED(hr)) { stream->Release(); factory->Release(); return hr; }
    hr = factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand, &decoder);
    if (FAILED(hr)) { stream->Release(); factory->Release(); return hr; }
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) { decoder->Release(); stream->Release(); factory->Release(); return hr; }
    hr = factory->CreateFormatConverter(&conv);
    if (FAILED(hr)) { frame->Release(); decoder->Release(); stream->Release(); factory->Release(); return hr; }
    hr = conv->Initialize(frame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) { conv->Release(); frame->Release(); decoder->Release(); stream->Release(); factory->Release(); return hr; }

    UINT w = 0, h = 0;
    conv->GetSize(&w, &h);
    std::vector<BYTE> pixels(w * h * 4);
    hr = conv->CopyPixels(nullptr, w * 4, (UINT)pixels.size(), pixels.data());
    conv->Release(); frame->Release(); decoder->Release(); stream->Release(); factory->Release();
    if (FAILED(hr)) return hr;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = w; desc.Height = h; desc.MipLevels = 1; desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = pixels.data(); sd.SysMemPitch = w * 4;
    ID3D11Texture2D* tex = nullptr;
    hr = device->CreateTexture2D(&desc, &sd, &tex);
    if (FAILED(hr)) return hr;
    hr = device->CreateShaderResourceView(tex, nullptr, outSRV);
    tex->Release();
    return hr;
}

static int64_t eptime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

Memory MemoryFastInject;

bool fastinter = false;
bool fastexter = false;
bool aimhelp = false;
bool mgn;
ImFont* InterSemiBold = nullptr;

void WallhackFlyInject() {

};
int Enmkey = 0;
void SpeedInject() {

    MemoryFastInject.EnablesSX("Speed", 0x0L, 0x00007fffffffffff, "", "", "Speed");
};

bool glitchf = false;
bool cameraleft = false;
int camerakey = 0;
int fastkey = 0;

std::unordered_map<uint32_t, Vector3> enemiesSunk;
std::map<uintptr_t, uint32_t> patchedMemory;

void RestorePatchedMemory()
{
    for (auto& entry : patchedMemory)
    {
        uintptr_t addr = entry.first;
        uint32_t originalValue = entry.second;

        WriteZ(addr, originalValue);
    }
    patchedMemory.clear();
}
void RestorePatchedMemoryv3()
{
    for (auto it = patchedMemory.begin(); it != patchedMemory.end(); ++it)
    {
        uintptr_t addr = it->first;
        uint32_t originalValue = it->second;

        uint32_t currentValue = 0;
        if (ReadZ(addr, currentValue))
        {
            if (currentValue != originalValue)
            {
                WriteZ(addr, originalValue);

                std::cout << "Restored addr: 0x" << std::hex << addr
                    << " to value: 0x" << originalValue << std::dec << std::endl;
            }
            else
            {
            }
        }
        else
        {
            std::cerr << "Failed to read addr: 0x" << std::hex << addr << std::dec << std::endl;
        }
    }

    patchedMemory.clear();
}



HWND InjectADB()
{
    bool adbinject = true;
    if (!adbinject)
        return NULL;

    auto vmm = GetModuleHandleA("BstkVMM.dll");
    if (vmm == nullptr) return NULL;

    auto readFunc = (PGMPhysReadFunc)GetProcAddress(vmm, "PGMPhysRead");
    if (readFunc == nullptr) return NULL;

    MH_Initialize();
    if (MH_CreateHook((LPVOID)readFunc, HookedPGMPhysRead, (LPVOID*)&ogPhysRead) != MH_OK) return NULL;
    if (MH_EnableHook((LPVOID)readFunc) != MH_OK) return NULL;

    while (vmPtr == nullptr) Sleep(10);

    ogCPU = (VMMGetCpuByIdFunc)GetProcAddress(vmm, "VMMGetCpuById");
    if (ogCPU == nullptr) return NULL;

    ogCast = (PGMPhysGCPtr2GCPhysFunc)GetProcAddress(vmm, "PGMPhysGCPtr2GCPhys");
    if (ogCast == nullptr) return NULL;

    ogWrite = (PGMPhysSimpleWriteGCPhysFunc)GetProcAddress(vmm, "PGMPhysSimpleWriteGCPhys");
    if (ogWrite == nullptr) return NULL;

    InitializeZ(vmPtr);
    std::cout << "-: Virt MemoryZ : " << pVMAddr << std::endl;

    bool GetDir = CambiarDirectoryZ(GetExeDirectoryZ());
    if (!GetDir)
        MessageBox(NULL, "Error Get Dir Emulator", "Error", NULL);

    KillAdbZ();
    ComdADBZ("kill-server");
    ComdADBZ("devices");

    std::string il2cppStr = ShellGetAddressNoSuZ("cat /proc/$(pidof com.dts.freefiremax)/maps | grep libil2cpp.so ; cat /proc/$(pidof com.dts.freefireth)/maps | grep libil2cpp.so");
    if (il2cppStr.empty())
        MessageBox(NULL, "Error Get Il2cpp Emulator", "Error", NULL);

    Offsets::Il2Cpp = uIntExtrZ(il2cppStr);
    if (Offsets::Il2Cpp == 0)
        MessageBox(NULL, "Error Get Il2cpp Panel", "Error", NULL);

    Sleep(100);

    KillProcessByD7("HD-Adb.exe");
    KillProcessByD7("BstkSVC.exe");

    std::string EmulatorDet = "NULL";
    HWND HwndEmul = NULL;
    HwndEmul = FindWindow(NULL, "BlueStacks App Player");
    if (HwndEmul == NULL)
    {
        HwndEmul = FindWindow(NULL, "MSI App Player");
        if (HwndEmul == NULL)
        {
            HwndEmul = FindWindow(NULL, "BlueStacks");
            if (HwndEmul == NULL)
            {
                HwndEmul = FindWindow(NULL, "App Player");
                if (HwndEmul == NULL)
                {
                    HwndEmul = FindWindow(NULL, "MSIZ");
                    if (HwndEmul == NULL)
                    {
                        HwndEmul = FindWindow(NULL, "e4vX Bs4");
                        if (HwndEmul != NULL)
                            EmulatorDet = "e4vX";
                    }
                    else
                    {
                        EmulatorDet = "BlueStacks 5 China";
                    }
                }
                else
                {
                    EmulatorDet = "MSI 4";
                }
            }
            else
            {
                EmulatorDet = "BlueStacks 4";
            }
        }
        else
        {
            EmulatorDet = "MSI 5";
        }
    }
    else
    {
        EmulatorDet = "BlueStacks 5";
    }

    HWND hdPlayerWindow = FindHDPlayerWindow(HwndEmul);


    REIMAN = true;

    return hdPlayerWindow;
}

bool SpeedTimerEnable;

namespace pic
{
    ID3D11ShaderResourceView* misc = nullptr;
    ID3D11ShaderResourceView* extra = nullptr;
    ID3D11ShaderResourceView* info11 = nullptr;
    ID3D11ShaderResourceView* discord = nullptr;
    ID3D11ShaderResourceView* aimbot = nullptr;
    ID3D11ShaderResourceView* shieldoxcy = nullptr;
    ID3D11ShaderResourceView* visuals = nullptr;
    ID3D11ShaderResourceView* skins = nullptr;
    ID3D11ShaderResourceView* settings = nullptr;
    ID3D11ShaderResourceView* combo_widget = nullptr;
    ID3D11ShaderResourceView* input_widget = nullptr;
    ID3D11ShaderResourceView* menu_settings_icon = nullptr;
    ID3D11ShaderResourceView* circle_success = nullptr;
    ID3D11ShaderResourceView* more = nullptr;
    ID3D11ShaderResourceView* log11 = nullptr;
}

struct SilentTarget {
    uint32_t entity = 0;
    Vector3 headPosition;
    float screenDistance = FLT_MAX;
};
int FovRage = 180;

int FovAll = 180;
int DistanceAll = 150;
bool InstantF;
float valueFire = 0.01f;

static int silent_combo;
const char* silent_list[] = { "Normal", "360", "Fov Ultra" };

float GetDistance(ImVec2 a, ImVec2 b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}
bool SilentHook;
bool SilentHookG;
char Username[255] = "";
char Password[255] = "";
bool ShowFov = false;
inline ImVec4 ColorFov = ImColor(255, 255, 255);
const char* cheat_name = "DELTA INTERNAL";
static bool isFlywallActive = false;
static Vector3 flywallPosition = Vector3(0, 0, 0);
static float flywallOffset = 1.9f;
static int flywallTickDelay = 1;
bool Fastrun = false;
bool Speed = false;
int delayToHeadMs = 120;
float AimDistancia = 150.0f;
bool predictionEnable = true;

bool AimbotHex = false;
bool SilentAim = false;

int targetMode = 0;




NotificationSystem notificationSystem;

void DrawAnimatedRainbowFOVWithGlow(ImVec2 center, float radius, float lineThickness, float glowThickness, int segments = 100, float rainbowSpeed = 0.2f) {
    auto* drawList = ImGui::GetBackgroundDrawList();
    double time = ImGui::GetTime();
    float hueOffset = fmod(time * rainbowSpeed, 1.0f);
    float glowOpacity = 0.10f;
    for (float thickness = glowThickness; thickness > 1.0f; thickness -= 1.5f) {
        for (int i = 0; i < segments; ++i) {
            float a0 = (2.0f * IM_PI * i) / segments;
            float a1 = (2.0f * IM_PI * (i + 1)) / segments;

            ImVec2 p0 = ImVec2(center.x + cosf(a0) * radius, center.y + sinf(a0) * radius);
            ImVec2 p1 = ImVec2(center.x + cosf(a1) * radius, center.y + sinf(a1) * radius);

            ImU32 glowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, glowOpacity));
            drawList->AddLine(p0, p1, glowColor, thickness);
        }
        glowOpacity *= 0.6f;
    }
    for (int i = 0; i < segments; ++i) {
        float a0 = (2.0f * IM_PI * i) / segments;
        float a1 = (2.0f * IM_PI * (i + 1)) / segments;
        ImVec2 p0 = ImVec2(center.x + cosf(a0) * radius, center.y + sinf(a0) * radius);
        ImVec2 p1 = ImVec2(center.x + cosf(a1) * radius, center.y + sinf(a1) * radius);
        float hue = fmod(hueOffset + (float)i / segments, 1.0f);
        ImU32 color = ImColor::HSV(hue, 1.0f, 1.0f);
        drawList->AddLine(p0, p1, color, lineThickness);
    }
}


extern "C" {
}

static char user[64] = { "" };
static char Key[64] = { "" };
static char pass[64] = { "" };


std::string name = "DELTAX";
std::string ownerid = "lFPmFvGfseo";
std::string secret = "1ab2c0b7bea42f6a60bb0f20da986c56181e5c8905545454FSEfsfes30499f1d3093";
std::string version = "1.0";
std::string url = "https://keyauth.win/api/1.3/";
KeyAuth::api KeyAuthApp(name, ownerid, secret, version, url);


static char g_loginError[256] = { 0 };
static std::string g_sessionid;
static std::string g_lastLoginExpiry;
static bool g_initDone = false;
static std::atomic<int> g_loginState = 0; // 0=idle, 1=busy, 2=success, 3=fail

static std::string GetHWID()
{
    char buffer[128] = { 0 };
    DWORD size = sizeof(buffer);
    HKEY hKey = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueExA(hKey, "MachineGuid", nullptr, nullptr, (LPBYTE)buffer, &size) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return std::string(buffer);
        }
        RegCloseKey(hKey);
    }
    return std::string("00000000-0000-0000-0000-000000000000");
}
static std::string JsonStr(const json& val, const std::string& def = "Unknown")
{
    if (val.is_string()) return val.get<std::string>();
    if (val.is_number_integer()) return std::to_string(val.get<long long>());
    if (val.is_number_unsigned()) return std::to_string(val.get<unsigned long long>());
    if (val.is_number_float()) return std::to_string(val.get<double>());
    return def;
}

static int DoLoginWork()
{
    try
    {
        if (!g_initDone)
        {
            std::string init_url = url + "?type=init&ver=" + version + "&name=" + name + "&ownerid=" + ownerid + "&secret=" + secret;
            wrap::Response r = wrap::HttpsRequest(wrap::Url{ init_url.c_str() }, wrap::Method{ "POST" });
            json data = json::parse(r.text);
            if (data.value("message", "Not Found") != "Initialized")
            {
                snprintf(g_loginError, sizeof(g_loginError), "Init: %s", std::string(data.value("message", "Not Found")).c_str());
                return 3;
            }
            g_sessionid = data.value("sessionid", "");
            if (g_sessionid.empty())
            {
                snprintf(g_loginError, sizeof(g_loginError), "Init: missing sessionid");
                return 3;
            }
            g_initDone = true;
        }
        std::string user_str(user);
        std::string pass_str(pass);
        std::string hwid = GetHWID();
        std::string login_url = url + "?type=login&username=" + user_str + "&pass=" + pass_str + "&sessionid=" + g_sessionid + "&name=" + name + "&ownerid=" + ownerid + "&hwid=" + hwid;
        wrap::Response r1 = wrap::HttpsRequest(wrap::Url{ login_url.c_str() }, wrap::Method{ "POST" });
        json data1 = json::parse(r1.text);
        std::string msg = data1.value("message", "Not Found");
        if (msg != "Logged in!")
        {
            snprintf(g_loginError, sizeof(g_loginError), "Login: %s", msg.c_str());
            return 3;
        }
        std::string expiry = "Unknown";
        if (data1.contains("info") && data1["info"].is_object())
        {
            auto& info = data1["info"];
            if (info.contains("subscriptions") && info["subscriptions"].is_array() && !info["subscriptions"].empty())
            {
                auto& firstSub = info["subscriptions"][0];
                if (info.contains("timeleft"))
                    expiry = JsonStr(info["timeleft"]);
                else if (firstSub.contains("timeleft"))
                    expiry = JsonStr(firstSub["timeleft"]);
                else if (firstSub.contains("expiry"))
                    expiry = JsonStr(firstSub["expiry"]);
            }
        }
        g_lastLoginExpiry = expiry;
        return 2;
    }
    catch (const std::exception& e)
    {
        snprintf(g_loginError, sizeof(g_loginError), "KeyAuth error: %s", e.what());
        return 3;
    }
    catch (...)
    {
        snprintf(g_loginError, sizeof(g_loginError), "KeyAuth error: unknown");
        return 3;
    }
}
static int SafeKeyAuthLogin()
{
    __try { return DoLoginWork(); }
    __except (EXCEPTION_EXECUTE_HANDLER) { snprintf(g_loginError, sizeof(g_loginError), "KeyAuth exception"); return 3; }
}



int spdkey = 0;
int climbkey = 0;
int dvkey = 0;
int dwkey = 0;

bool TeleportMarkEnable = false;
static uint32_t speedTeleportBestTarget = 0;
static Vector3 speedTeleportBestPos{};
static float speedTeleportClosestDist = FLT_MAX;
static auto speedTeleportLastTick = std::chrono::steady_clock::now();

bool MagnetPullEnable = false;
static bool diveKillUnderground = false;
static Vector3 savedOriginalPos{};
static bool climbUpActive = false;
static bool ClimbUpEnable = false;
static bool DiveKillEnable = false;

bool DownPlayerEnable = false;
float DownPlayerSpeed = 0.5f;
static std::unordered_map<uint32_t, Vector3> chestPullOriginalPos;
static uint32_t currentChestPullTarget = 0;
static bool wasChestPullFiring = false;
bool EnemyPullChestEnable = false;
bool SpeedTeleportSpeed = false;
bool SpeedTeleportEnable = false;
bool SpeedTeleportRange = false;





double getCurrentTime() {
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now - start).count();
}

void REIMAN_PRIME()
{



    if (!ProcId)
        ProcId = CMemory::Initialize()->GetProcessID();

    CMemory::Initialize()->Start(ProcId);

    int width2 = GetSystemMetrics(SM_CXSCREEN);
    int height2 = GetSystemMetrics(SM_CYSCREEN);

    WNDCLASSEXW wc;
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = NULL;
    wc.cbWndExtra = NULL;
    wc.hInstance = nullptr;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszMenuName = L"ImGui";
    wc.lpszClassName = L"Example";
    wc.hIconSm = LoadIcon(0, IDI_APPLICATION);
    ::RegisterClassExW(&wc);

    hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        wc.lpszClassName, L" ",
        WS_POPUP,
        0, 0, width2, height2,
        nullptr, nullptr, wc.hInstance, nullptr
    );



    SetWindowLongA(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED | ~WS_EX_TRANSPARENT);
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);

    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);


    POINT mouse;
    RECT rc = { 0 };


    GetWindowRect(hwnd, &rc);


    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImFontConfig cfg;
    cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint | ImGuiFreeTypeBuilderFlags_LightHinting | ImGuiFreeTypeBuilderFlags_LoadColor;

    font::lexend_general_bold = io.Fonts->AddFontFromMemoryTTF(lexend_bold, sizeof(lexend_bold), 18.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    font::lexend_bold = io.Fonts->AddFontFromMemoryTTF(lexend_regular, sizeof(lexend_regular), 17.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    font::lexend_regular = io.Fonts->AddFontFromMemoryTTF(lexend_regular, sizeof(lexend_regular), 14.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    font::icomoon = io.Fonts->AddFontFromMemoryTTF(icomoon, sizeof(icomoon), 20.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    InterSemiBold = io.Fonts->AddFontFromMemoryCompressedTTF(InterSemiBold_compressed_data, InterSemiBold_compressed_size, 16);
    font::WeaponsIco = io.Fonts->AddFontFromMemoryTTF(WIconsZ, sizeof(WIconsZ), 20.0f, NULL, NULL);


    //if (pic::misc == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, misc2, sizeof(misc2), nullptr, nullptr, &pic::misc, 0);
    //if (pic::extra == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, extra2, sizeof(extra2), nullptr, nullptr, &pic::extra, 0);
    //if (pic::info11 == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, info112, sizeof(info112), nullptr, nullptr, &pic::info11, 0);

    if (pic::aimbot == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, aimbot, sizeof(aimbot), nullptr, nullptr, &pic::aimbot, 0);
    if (pic::visuals == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, visuals, sizeof(visuals), nullptr, nullptr, &pic::visuals, 0);
    if (pic::skins == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, skins, sizeof(skins), nullptr, nullptr, &pic::skins, 0);
    if (pic::settings == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, settings, sizeof(settings), nullptr, nullptr, &pic::settings, 0);

    //if (pic::log11 == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, log11, sizeof(log11), nullptr, nullptr, &pic::log11, 0);
    //if (pic::more == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, more, sizeof(more), nullptr, nullptr, &pic::more, 0);

    ImFont* Nev = io.Fonts->AddFontFromMemoryTTF(&Nevan, sizeof Nevan, 25, NULL, io.Fonts->GetGlyphRangesCyrillic());

    ImFont* Nev1 = io.Fonts->AddFontFromMemoryTTF(&Nevan, sizeof Nevan, 10, NULL, io.Fonts->GetGlyphRangesCyrillic());

    font::icomoon_widget = io.Fonts->AddFontFromMemoryTTF(icomoon_widget, sizeof(icomoon_widget), 15.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    font::icomoon_widget2 = io.Fonts->AddFontFromMemoryTTF(icomoon, sizeof(icomoon), 16.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());

    D3DX11_IMAGE_LOAD_INFO info; ID3DX11ThreadPump* pump{ nullptr };
    if (texture::esp_preview == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, esp_preview1, sizeof(esp_preview1), &info, pump, &texture::esp_preview, 0);
    if (texture::incono == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, inocono1, sizeof(inocono1), &info, pump, &texture::incono, 0);
    if (texture::hora == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, hora1, sizeof(hora1), &info, pump, &texture::hora, 0);
    if (texture::king == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, king1, sizeof(king1), &info, pump, &texture::king, 0);
    if (image::bell_notify == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, bell_notify, sizeof(bell_notify), &info, pump, &image::bell_notify, 0);


    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    bool done = false;
    while (!done)
    {

        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done) break;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();


        static bool wasKeyDown = false;
        if (GetAsyncKeyState(WallhackFlyKey) & 0x8000)
        {
            if (!wasKeyDown)
            {
                WallhackFlyEnable = !WallhackFlyEnable;

                if (WallhackFlyEnable)
                {
                    std::thread([]() {
                        std::thread(WallhackFlyInject).detach();
                        }).detach();
                }
                else
                {
                    std::thread([]() {
                        std::thread(WallhackFlyInject).detach();
                        }).detach();
                }

                wasKeyDown = true;
            }
        }
        else
        {
            wasKeyDown = false;
        }

        static bool wasKeyDown1 = false;
        if (GetAsyncKeyState(SpeedKey) & 0x8000)
        {
            if (!wasKeyDown1)
            {
                SpeedEnable = !SpeedEnable;

                if (SpeedEnable)
                {
                    std::thread([]() {
                        std::thread(SpeedInject).detach();
                        }).detach();
                }
                else
                {
                    std::thread([]() {
                        std::thread(SpeedInject).detach();
                        }).detach();
                }

                wasKeyDown1 = true;
            }
        }
        else
        {
            wasKeyDown1 = false;
        }



        ImGui::NewFrame();
        {
            if (REIMAN)
            {
                RECT rect;
                GetWindowRect(hdPlayerWindow, &rect);
                int x = rect.left;
                int y = rect.top;
                int width = rect.right - rect.left;
                int height = rect.bottom - rect.top;
                Vector2 screenCenter = Vector2(x + width / 2, y + height / 2);

                uint32_t baseGameFacade = 0;
                ReadZ(Offsets::Il2Cpp + Offsets::InitBase, baseGameFacade);

                uint32_t gameFacade = 0;
                ReadZ(baseGameFacade, gameFacade);

                uint32_t staticGameFacade = 0;
                ReadZ(gameFacade + Offsets::StaticClass, staticGameFacade);

                uint32_t currentGame = 0;
                ReadZ(staticGameFacade, currentGame);

                uint32_t currentMatch = 0;
                ReadZ(currentGame + Offsets::CurrentMatch, currentMatch);

                uint32_t matchStatus = 0;
                ReadZ(currentMatch + Offsets::MatchStatus, matchStatus);

                uint32_t localPlayer = 0;
                if (ReadZ(currentMatch + Offsets::LocalPlayer, localPlayer)) {}

                uint32_t mainTransform = 0;
                ReadZ(localPlayer + Offsets::MainCameraTransform, mainTransform);

                Vector3 mainPos;
                GetPosition(mainTransform, mainPos);

                uint32_t followCamera = 0;
                ReadZ(localPlayer + Offsets::FollowCamera, followCamera);

                uint32_t camera = 0;
                ReadZ(followCamera + Offsets::Camera, camera);

                uint32_t cameraBase = 0;
                ReadZ(camera + 0x8, cameraBase);

                Matrix4x4 viewMatrix;
                ReadZ(cameraBase + Offsets::ViewMatrix, viewMatrix);

                uint32_t entityDictionary = 0;
                ReadZ(currentGame + Offsets::DictionaryEntities, entityDictionary);

                uint32_t entities = 0;
                ReadZ(entityDictionary + 0xc, entities);
                entities += 0x1c;

                uint32_t entitiesCount = 0;
                ReadZ(entityDictionary + 0x10, entitiesCount);

                if (currentMatch != 0 && localPlayer != 0 && entitiesCount > 0 && entitiesCount < 150)
                {
                    const char* windowTitles[] = { "MSI App Player", "BlueStacks App Player", nullptr };
                    HWND hwndEmulator = nullptr;
                    for (int j = 0; windowTitles[j]; j++) {
                        if ((hwndEmulator = FindWindowA(NULL, windowTitles[j]))) break;
                    }



                    RECT clientRect, windowRect;
                    GetClientRect(hwndEmulator, &clientRect);
                    GetWindowRect(hwndEmulator, &windowRect);

                    int emulatorWidth = clientRect.right - clientRect.left;
                    int emulatorHeight = clientRect.bottom - clientRect.top;
                    int clientX = windowRect.left;
                    int clientY = windowRect.top + (windowRect.bottom - windowRect.top - emulatorHeight);

                    for (uint32_t i = 0; i < entitiesCount; i++)
                    {
                        bool isDead = false, IsKnown = false, IsTeam = false, isVisible = false, isBot = false;
                        int EspHealthInt = 0;
                        float DistanceA = 0.0f, rectWidth = 150.0f, rectHeight = 30.0f;

                        uint32_t entity = 0;
                        if (!ReadZ(entities + i * 0x10, entity) || !entity) continue;

                        uint32_t avatarManager = 0;
                        if (!ReadZ(entity + Offsets::AvatarManager, avatarManager) || !avatarManager) continue;

                        uint32_t avatar = 0;
                        if (!ReadZ(avatarManager + Offsets::Avatar, avatar) || !avatar) continue;
                        if (!ReadZ(avatar + Offsets::Avatar_IsVisible, isVisible)) continue;

                        uint32_t avatarData = 0;
                        if (!ReadZ(avatar + Offsets::Avatar_Data, avatarData) || !avatarData) continue;

                        bool isTeam = false;
                        if (ReadZ(avatarData + Offsets::Avatar_Data_IsTeam, isTeam)) {
                            IsTeam = isTeam;
                            IsKnown = !isTeam;
                        }
                        ReadZ(entity + Offsets::isBotOffs, isBot);

                        if (ESPHealth) {
                            uint32_t dataPool = 0;
                            if (ReadZ(entity + Offsets::Player_Data, dataPool)) {
                                uint32_t poolObj = 0;
                                if (ReadZ(dataPool + 0x8, poolObj)) {
                                    uint32_t pool = 0;
                                    if (ReadZ(poolObj + 0x10, pool)) {
                                        int Health = 0;
                                        if (ReadZ(pool + 0x10, Health) && Health) EspHealthInt = Health;
                                    }
                                }
                            }
                        }

                        struct BonePositions {
                            Vector3 HeadV3, RootV3, LeftWristV3, SpineV3, HipV3,
                                RightCalfV3, LeftCalfV3, RightFootV3, LeftFootV3,
                                RightWristV3, LeftHandV3, LeftShoulderV3, RightShoulderV3,
                                RightWristJointV3, LeftWristJointV3, LeftElbowV3, RightElbowV3;
                        } bonePositions;

                        std::vector<std::pair<uint32_t, Vector3*>> bones = {
                            {Offsets::Bones::Head,           &bonePositions.HeadV3},
                            {Offsets::Bones::Root,           &bonePositions.RootV3},
                            {Offsets::Bones::LeftWrist,      &bonePositions.LeftWristV3},
                            {Offsets::Bones::Neck,           &bonePositions.SpineV3},
                            {Offsets::Bones::Hip,            &bonePositions.HipV3},
                            {Offsets::Bones::RightFoot,      &bonePositions.RightFootV3},
                            {Offsets::Bones::LeftFoot,       &bonePositions.LeftFootV3},
                            {Offsets::Bones::RightWrist,     &bonePositions.RightWristV3},
                            {Offsets::Bones::LeftHand,       &bonePositions.LeftHandV3},
                            {Offsets::Bones::LeftShoulder,   &bonePositions.LeftShoulderV3},
                            {Offsets::Bones::RightShoulder,  &bonePositions.RightShoulderV3},
                            {Offsets::Bones::LeftElbow,      &bonePositions.LeftElbowV3},
                            {Offsets::Bones::RightElbow,     &bonePositions.RightElbowV3}
                        };

                        for (const auto& [boneID, bonePos] : bones) {
                            uint32_t bone = 0;
                            if (ReadZ(entity + boneID, bone) && bone) GetNodePosition(bone, *bonePos);
                        }

                        bool isKnocked = false;
                        uint32_t shadowBase = 0;
                        if (ReadZ(entity + Offsets::Player_ShadowBase, shadowBase) && shadowBase) {
                            int xpose = 0;
                            if (ReadZ(shadowBase + Offsets::XPose, xpose) && xpose == 8) isKnocked = true;
                        }

                        DistanceA = Vector3::Distance(mainPos, bonePositions.HeadV3);
                        if (DistanceA > maxDistance) continue;

                        auto headScreenPos = WorldToScreenImVec2(viewMatrix, bonePositions.HeadV3, width, height);
                        auto NeckScreenPos = WorldToScreenImVec2(viewMatrix, bonePositions.SpineV3, width, height);
                        auto HipScreenPos = WorldToScreenImVec2(viewMatrix, bonePositions.HipV3, width, height);
                        auto RightShoulderScreenPos = WorldToScreenImVec2(viewMatrix, bonePositions.RightShoulderV3, width, height);
                        auto RightElbowScreenPos = WorldToScreenImVec2(viewMatrix, bonePositions.RightElbowV3, width, height);
                        auto RightWristScreenPos = WorldToScreenImVec2(viewMatrix, bonePositions.RightWristV3, width, height);
                        auto LeftShoulderScreenPos = WorldToScreenImVec2(viewMatrix, bonePositions.LeftShoulderV3, width, height);
                        auto LeftElbowScreenPos = WorldToScreenImVec2(viewMatrix, bonePositions.LeftElbowV3, width, height);
                        auto LeftWristScreenPos = WorldToScreenImVec2(viewMatrix, bonePositions.LeftWristV3, width, height);
                        auto RightFootScreenPos = WorldToScreenImVec2(viewMatrix, bonePositions.RightFootV3, width, height);
                        auto LeftFootScreenPos = WorldToScreenImVec2(viewMatrix, bonePositions.LeftFootV3, width, height);
                        auto bottomScreenPos = WorldToScreenImVec2(viewMatrix, bonePositions.RootV3, width, height);

                        ImVec2 rectTopLeft = ImVec2(x + headScreenPos.x - rectWidth / 2, y + headScreenPos.y - rectHeight - 10);
                        ImVec2 rectBottomRight = ImVec2(x + headScreenPos.x + rectWidth / 2, y + headScreenPos.y - 10);

                        Vector2 PlayerPosHead(x + headScreenPos.x, y + headScreenPos.y);
                        Vector2 PlayerPosBott(x + bottomScreenPos.x, y + bottomScreenPos.y);
                        float CornerHeight = std::abs(PlayerPosHead.Y - PlayerPosBott.Y);
                        float CornerWidth = CornerHeight * 0.65f;

                        if (SHowVis == false && !isVisible) continue;

                        bool isOnScreen = (headScreenPos.x > 0 && headScreenPos.y > 0 &&
                            (x + headScreenPos.x) >= clientX && (x + headScreenPos.x) <= clientX + emulatorWidth &&
                            (y + headScreenPos.y) >= clientY && (y + headScreenPos.y) <= clientY + emulatorHeight);

                        if (!isOnScreen) continue;

                        auto vList = ImGui::GetForegroundDrawList();
                        vList->PushClipRect(ImVec2(clientX, clientY),
                            ImVec2(clientX + emulatorWidth, clientY + emulatorHeight), true);

                        if (ReadZ(entity + Offsets::Player_IsDead, isDead) && isDead) continue;

                        auto WIDGET = zGetInfoW(entity, ESPWeapon);
                        if (ESPNameZ && !IsTeam)
                        {
                            uint32_t nameAddr = 0;
                            std::string strEspNameScan = defaultName;
                            if (ReadZ(entity + Offsets::Player_Name, nameAddr))
                            {
                                strEspNameScan = ReadPlayerName(nameAddr);
                                if (strEspNameScan.empty()) strEspNameScan = defaultName;
                            }

                            ImVec2 textSize = ImGui::CalcTextSize(strEspNameScan.c_str());
                            float centerX = (rectTopLeft.x + rectBottomRight.x) * 0.5f;
                            ImVec2 namePos(centerX - (textSize.x * 0.5f),
                                rectTopLeft.y + 10.0f - (textSize.y * 0.5f));

                            vList->AddText(namePos, ImGui::GetColorU32(nameColor), strEspNameScan.c_str());
                        }

                        if (EspRadar && !IsTeam)
                        {
                            ImU32 radarBorderColor = ImColor(0, 40, 0, 50);
                            ImU32 radarFillColor = ImColor(0, 40, 0, 50);
                            ImU32 gridColor = ImColor(0, 100, 0, 50);
                            ImU32 markerGlowColor = ImColor(255, 127, 0, 150);
                            ImU32 markerColor = ImColor(255, 255, 0, 255);
                            ImU32 textColor = ImColor(255, 255, 255, 220);
                            ImU32 triangleColor = ImColor(255, 0, 0, 255);

                            auto drawList = ImGui::GetBackgroundDrawList();
                            ImVec2 radarPos(clientX + 20, clientY + 20);
                            float  radarSize = 150.0f;
                            ImVec2 radarCenter = ImVec2(radarPos.x + radarSize / 2, radarPos.y + radarSize / 2);

                            drawList->AddCircleFilled(radarCenter, radarSize / 2, radarFillColor, 64);
                            for (int r = 1; r <= 3; r++)
                                drawList->AddCircle(radarCenter, (radarSize / 2) * (0.2f + 0.25f * r), gridColor, 64, 1.0f);

                            for (int r = 0; r < 8; r++) {
                                float angle = (2.0f * M_PI / 8) * r;
                                drawList->AddLine(
                                    ImVec2(radarCenter.x + cosf(angle) * 0.1f * radarSize, radarCenter.y + sinf(angle) * 0.1f * radarSize),
                                    ImVec2(radarCenter.x + cosf(angle) * (radarSize / 2 - 10), radarCenter.y + sinf(angle) * (radarSize / 2 - 10)),
                                    gridColor, 1.0f);
                            }

                            drawList->AddCircle(radarCenter, radarSize / 2, ImColor(0, 0, 0, 0), 64, 3.0f);
                            drawList->AddCircle(radarCenter, radarSize / 2, radarBorderColor, 64, 2.0f);

                            ImVec2 tri[3] = {
                                ImVec2(radarCenter.x,      radarCenter.y - 10),
                                ImVec2(radarCenter.x - 10, radarCenter.y + 5),
                                ImVec2(radarCenter.x + 10, radarCenter.y + 5)
                            };
                            drawList->AddTriangleFilled(tri[0], tri[1], tri[2], triangleColor);

                            ImVec2 headPos(x + headScreenPos.x, y + headScreenPos.y);
                            if (headPos.x >= clientX && headPos.x <= clientX + emulatorWidth &&
                                headPos.y >= clientY && headPos.y <= clientY + emulatorHeight)
                            {
                                ImVec2 rel((headPos.x - clientX - emulatorWidth / 2) * 0.18f,
                                    (headPos.y - clientY - emulatorHeight / 2) * 0.18f);
                                float maxR = radarSize / 2 - 20.0f;
                                float dist = sqrtf(rel.x * rel.x + rel.y * rel.y);
                                if (dist > maxR) rel = ImVec2(rel.x * maxR / dist, rel.y * maxR / dist);

                                ImVec2 markerPos(radarCenter.x + rel.x, radarCenter.y + rel.y);
                                drawList->AddCircleFilled(markerPos, 6.0f, markerGlowColor, 12);
                                drawList->AddCircleFilled(markerPos, 3.0f, markerColor, 12);

                                char distTxt[32];
                                snprintf(distTxt, sizeof(distTxt), "%dm", static_cast<int>(DistanceA));
                                ImVec2 ts = ImGui::CalcTextSize(distTxt);
                                drawList->AddText(ImVec2(markerPos.x - ts.x / 2, markerPos.y + 10), textColor, distTxt);
                            }
                        }


                        if (ESPBox && !IsTeam)
                        {
                            float WBox = CornerWidth;
                            float HBox = CornerHeight + 20.0f;
                            float XBox = PlayerPosHead.X - (WBox / 2);
                            float YBox = PlayerPosHead.Y - 10.0f;

                            float thickness = 1.5f;
                            float cornerSize = WBox * 0.18f;

                            ImU32 boxColorx = ImGui::ColorConvertFloat4ToU32(boxColor);

                            if (rgbMode) {
                                double t = getCurrentTime();
                                float freq = 0.5f;
                                float r = 0.5f * (1 + sinf((float)t * 6.28f * freq));
                                float g = 0.5f * (1 + sinf((float)t * 6.28f * freq + 2.09f));
                                float b = 0.5f * (1 + sinf((float)t * 6.28f * freq + 4.18f));
                                boxColor = ImVec4(r, g, b, 1.0f);
                                boxColorx = ImGui::ColorConvertFloat4ToU32(boxColor);
                            }
                            else {
                                boxColorx = isKnocked ? IM_COL32(255, 0, 0, 255)
                                    : ImGui::ColorConvertFloat4ToU32(boxColor);
                            }

                            if (ESPBoxType == 0) // Cornered
                            {
                                float glowR = 12.0f;
                                for (int gi = (int)glowR; gi > 0; --gi) {
                                    float alpha = 25.0f * (1.0f - (float)gi / glowR);
                                    ImU32 gc = IM_COL32((boxColorx & 0xFF), (boxColorx >> 8) & 0xFF,
                                        (boxColorx >> 16) & 0xFF, (int)alpha);
                                    vList->AddLine(ImVec2(XBox - gi, YBox - gi), ImVec2(XBox + cornerSize + gi, YBox - gi), gc, 1.0f);
                                    vList->AddLine(ImVec2(XBox - gi, YBox - gi), ImVec2(XBox - gi, YBox + cornerSize + gi), gc, 1.0f);
                                    vList->AddLine(ImVec2(XBox + WBox + gi, YBox - gi), ImVec2(XBox + WBox - cornerSize - gi, YBox - gi), gc, 1.0f);
                                    vList->AddLine(ImVec2(XBox + WBox + gi, YBox - gi), ImVec2(XBox + WBox + gi, YBox + cornerSize + gi), gc, 1.0f);
                                    vList->AddLine(ImVec2(XBox - gi, YBox + HBox - cornerSize - gi), ImVec2(XBox - gi, YBox + HBox + gi), gc, 1.0f);
                                    vList->AddLine(ImVec2(XBox - gi, YBox + HBox + gi), ImVec2(XBox + cornerSize + gi, YBox + HBox + gi), gc, 1.0f);
                                    vList->AddLine(ImVec2(XBox + WBox - cornerSize - gi, YBox + HBox + gi), ImVec2(XBox + WBox + gi, YBox + HBox + gi), gc, 1.0f);
                                    vList->AddLine(ImVec2(XBox + WBox + gi, YBox + HBox - cornerSize - gi), ImVec2(XBox + WBox + gi, YBox + HBox + gi), gc, 1.0f);
                                }
                                vList->AddLine(ImVec2(XBox, YBox), ImVec2(XBox + cornerSize, YBox), boxColorx, thickness);
                                vList->AddLine(ImVec2(XBox, YBox), ImVec2(XBox, YBox + cornerSize), boxColorx, thickness);
                                vList->AddLine(ImVec2(XBox + WBox - cornerSize, YBox), ImVec2(XBox + WBox, YBox), boxColorx, thickness);
                                vList->AddLine(ImVec2(XBox + WBox, YBox), ImVec2(XBox + WBox, YBox + cornerSize), boxColorx, thickness);
                                vList->AddLine(ImVec2(XBox, YBox + HBox - cornerSize), ImVec2(XBox, YBox + HBox), boxColorx, thickness);
                                vList->AddLine(ImVec2(XBox, YBox + HBox), ImVec2(XBox + cornerSize, YBox + HBox), boxColorx, thickness);
                                vList->AddLine(ImVec2(XBox + WBox - cornerSize, YBox + HBox), ImVec2(XBox + WBox, YBox + HBox), boxColorx, thickness);
                                vList->AddLine(ImVec2(XBox + WBox, YBox + HBox - cornerSize), ImVec2(XBox + WBox, YBox + HBox), boxColorx, thickness);
                            }
                            else if (ESPBoxType == 1) // Full box
                            {
                                vList->AddRect(ImVec2(XBox, YBox), ImVec2(XBox + WBox, YBox + HBox), boxColorx, 0, 0, thickness);
                            }
                            else if (ESPBoxType == 2) // Shadow box
                            {
                                float lineLengthH = WBox * 0.30f;
                                float lineLengthV = HBox * 0.35f;
                                float shadowPad = 2.0f;
                                float shadowThick = 10.0f;
                                ImVec2 shadowOff = ImVec2(1.5f, 1.5f);

                                auto AddShadowLineH = [&](float sx, float sy, float len) {
                                    vList->AddShadowRect(
                                        ImVec2(sx - shadowPad / 2, sy - (shadowPad + thickness) / 2),
                                        ImVec2(sx + len + shadowPad / 2, sy + (shadowPad + thickness) / 2),
                                        boxColorx, shadowThick, shadowOff, 0, 5.0f);
                                    vList->AddLine(ImVec2(sx, sy), ImVec2(sx + len, sy), boxColorx, thickness);
                                    };
                                auto AddShadowLineV = [&](float sx, float sy, float len) {
                                    vList->AddShadowRect(
                                        ImVec2(sx - (shadowPad + thickness) / 2, sy - shadowPad / 2),
                                        ImVec2(sx + (shadowPad + thickness) / 2, sy + len + shadowPad / 2),
                                        boxColorx, shadowThick, shadowOff, 0, 5.0f);
                                    vList->AddLine(ImVec2(sx, sy), ImVec2(sx, sy + len), boxColorx, thickness);
                                    };

                                AddShadowLineH(XBox, YBox, lineLengthH);
                                AddShadowLineV(XBox, YBox, lineLengthV);
                                AddShadowLineH(XBox + WBox - lineLengthH, YBox, lineLengthH);
                                AddShadowLineV(XBox + WBox, YBox, lineLengthV);
                                AddShadowLineH(XBox, YBox + HBox, lineLengthH);
                                AddShadowLineV(XBox, YBox + HBox - lineLengthV, lineLengthV);
                                AddShadowLineH(XBox + WBox - lineLengthH, YBox + HBox, lineLengthH);
                                AddShadowLineV(XBox + WBox, YBox + HBox - lineLengthV, lineLengthV);
                            }
                        }

                        if (ESPBoxFILLED && !IsTeam)
                        {
                            float XBox = PlayerPosHead.X - (CornerWidth / 2);
                            float YBox = PlayerPosHead.Y - 10.0f;
                            float WBox = CornerWidth;
                            float HBox = CornerHeight + 20.0f;
                            float rounding = 4.0f;

                            if (rgbMode) {
                                double t = getCurrentTime();
                                float freq = 0.5f;
                                auto getRGB = [&](float off, int a) -> ImU32 {
                                    return IM_COL32(
                                        (int)(0.5f * (1 + sinf(6.28f * freq * (float)t + off)) * 255),
                                        (int)(0.5f * (1 + sinf(6.28f * freq * (float)t + off + 2.09f)) * 255),
                                        (int)(0.5f * (1 + sinf(6.28f * freq * (float)t + off + 4.18f)) * 255), a);
                                    };
                                vList->AddRectFilled(ImVec2(XBox, YBox), ImVec2(XBox + WBox, YBox + HBox), getRGB(0.2f, 80), rounding);
                                vList->AddRect(ImVec2(XBox, YBox), ImVec2(XBox + WBox, YBox + HBox), getRGB(0.5f, 255), rounding, 0, 0.0f);
                            }
                            else {
                                ImU32 fillCol = IM_COL32(
                                    (int)(boxColorFilled.x * 255), (int)(boxColorFilled.y * 255),
                                    (int)(boxColorFilled.z * 255), 50);
                                vList->AddRectFilled(ImVec2(XBox, YBox), ImVec2(XBox + WBox, YBox + HBox), fillCol, rounding);
                            }
                        }
                        float EspThickness = 0.5f;

                        if (EspLineZ && !IsTeam)
                        {
                            ImU32 lineColor = rgbMode
                                ? [&]() -> ImU32 {
                                double t = getCurrentTime(); float f = 0.5f;
                                return IM_COL32(
                                    (int)(0.5f * (1 + sinf(6.28f * f * (float)t)) * 255),
                                    (int)(0.5f * (1 + sinf(6.28f * f * (float)t + 2.09f)) * 255),
                                    (int)(0.5f * (1 + sinf(6.28f * f * (float)t + 4.18f)) * 255), 255);
                                }()
                                    : ImGui::ColorConvertFloat4ToU32(lineColorEnemy);

                                ImVec2 p1(x + headScreenPos.x, y + headScreenPos.y);
                                ImVec2 p2;
                                if (lineType == 0) p2 = ImVec2(x + width / 2.0f, (float)y);
                                else if (lineType == 1) p2 = ImVec2(x + width / 2.0f, y + height / 2.0f);
                                else                     p2 = ImVec2(x + width / 2.0f, y + (float)height);

                                for (float t = EspThickness + 2.0f; t > EspThickness; t -= 1.5f) {
                                    ImVec4 c = ImGui::ColorConvertU32ToFloat4(lineColor);
                                    vList->AddLine(p1, p2, IM_COL32((int)(c.x * 255), (int)(c.y * 255), (int)(c.z * 255), 0), t);
                                }
                                vList->AddLine(p1, p2, lineColor, EspThickness);
                        }

                        if (ESPHealth && !IsTeam)
                        {
                            float vidaPorcentaje = std::clamp(EspHealthInt / 200.0f, 0.0f, 1.0f);
                            float XBox = PlayerPosHead.X - (CornerWidth / 2);
                            float YBox = PlayerPosHead.Y - 10.0f;
                            float WBox = CornerWidth;
                            float HBox = CornerHeight + 20.0f;

                            ImDrawList* hpList = ImGui::GetForegroundDrawList();
                            ImFont* font = InterSemiBold;
                            float fontSize = font->FontSize * 0.75f;

                            if (selectedHealthBarPos < 0 || selectedHealthBarPos > 4) selectedHealthBarPos = 0;

                            switch (selectedHealthBarPos)
                            {
                            case 0: {
                                DrawVerticalHealthBar(EspHealthInt, 200, ImVec2(XBox - 5.0f, YBox), HBox);
                                char hpTxt[12]; snprintf(hpTxt, sizeof(hpTxt), "%d", EspHealthInt);
                                ImU32 tc = vidaPorcentaje <= 0.25f ? IM_COL32(255, 60, 60, 220)
                                    : vidaPorcentaje <= 0.5f ? IM_COL32(255, 215, 0, 220)
                                    : IM_COL32(255, 255, 255, 200);
                                hpList->AddText(font, fontSize, ImVec2(XBox - 0.5f, YBox + HBox + 1.0f), tc, hpTxt);
                                break;
                            }
                            case 1: {
                                DrawVerticalHealthBar(EspHealthInt, 200, ImVec2(XBox + WBox + 5.0f, YBox), HBox);
                                char hpTxt[12]; snprintf(hpTxt, sizeof(hpTxt), "%d", EspHealthInt);
                                ImU32 tc = vidaPorcentaje <= 0.25f ? IM_COL32(255, 60, 60, 220)
                                    : vidaPorcentaje <= 0.5f ? IM_COL32(255, 215, 0, 220)
                                    : IM_COL32(255, 255, 255, 200);
                                hpList->AddText(font, fontSize, ImVec2(XBox + WBox + 9.5f, YBox + HBox + 1.0f), tc, hpTxt);
                                break;
                            }
                            case 2: {
                                DrawHorizontalHealthBarTop(EspHealthInt, 200, XBox, YBox - 10.0f, WBox);
                                break;
                            }
                            case 3: {
                                DrawHealthBarBelowFullBox(EspHealthInt, 200, XBox, YBox, WBox, HBox);
                                break;
                            }
                            case 4: {
                                char hpTxt[12]; snprintf(hpTxt, sizeof(hpTxt), "%d HP", EspHealthInt);
                                ImU32 tc = vidaPorcentaje <= 0.25f ? IM_COL32(255, 60, 60, 250)
                                    : vidaPorcentaje <= 0.5f ? IM_COL32(255, 215, 0, 250)
                                    : IM_COL32(3, 252, 32, 250);
                                hpList->AddText(font, fontSize, ImVec2(XBox + WBox + 7.0f, YBox + HBox + 1.0f), tc, hpTxt);
                                break;
                            }
                            }
                        }

                        if (ESPWeapon && !IsTeam)
                        {
                            vList->PushClipRect(ImVec2(clientX, clientY),
                                ImVec2(clientX + emulatorWidth, clientY + emulatorHeight), true);

                            std::string weaponName = zGetWName(WIDGET);
                            const char* icono = zGetWIcon(WIDGET);

                            if (!weaponName.empty() && icono != nullptr)
                            {
                                PushFont(font::lexend_regular);
                                ImVec2 textSize = ImGui::CalcTextSize(weaponName.c_str());
                                PopFont();

                                float  boxW = textSize.x + 20.0f;
                                float  boxH = 42.0f;
                                ImVec2 basePos = ImVec2(x + headScreenPos.x - boxW / 2.0f,
                                    y + headScreenPos.y - 60.0f - boxH);

                                ImVec2 iconPos = ImVec2(basePos.x + boxW / 2.0f - 7.0f, basePos.y + 4.0f);
                                ImGui::PushFont(font::WeaponsIco);
                                vList->AddText(iconPos, IM_COL32(255, 255, 255, 255), icono);
                                ImGui::PopFont();

                                ImVec2 txtPos = ImVec2(basePos.x + (boxW - textSize.x) / 2.0f, basePos.y + 22.0f);
                                PushFont(font::lexend_regular);
                                ImGui::SetWindowFontScale(0.97f);
                                vList->AddText(txtPos, IM_COL32(255, 255, 255, 255), weaponName.c_str());
                                ImGui::SetWindowFontScale(1.0f);
                                PopFont();
                            }
                            vList->PopClipRect();
                        }

                        if (ESPDistance && entity != localPlayer && !IsTeam)
                        {
                            std::string distStr = std::to_string(static_cast<int>(DistanceA)) + "m";
                            ImVec2 distPos(
                                rectTopLeft.x + (rectBottomRight.x - rectTopLeft.x) / 2.0f - 15.0f,
                                rectTopLeft.y - 15.0f);

                            ImU32 outCol = IM_COL32(0, 0, 0, 255);
                            ImU32 mainCol = IM_COL32(255, 255, 255, 255);
                            ImU32 glowCol = IM_COL32(255, 255, 255, 160);

                            for (float ox : {-1.f, 1.f}) vList->AddText(ImVec2(distPos.x + ox, distPos.y), outCol, distStr.c_str());
                            for (float oy : {-1.f, 1.f}) vList->AddText(ImVec2(distPos.x, distPos.y + oy), outCol, distStr.c_str());
                            for (float ox : {-1.f, 1.f})
                                for (float oy : {-1.f, 1.f}) vList->AddText(ImVec2(distPos.x + ox, distPos.y + oy), glowCol, distStr.c_str());
                            vList->AddText(distPos, mainCol, distStr.c_str());
                        }

                        if (ESPBones && !IsTeam)
                        {
                            auto draw = ImGui::GetBackgroundDrawList();
                            draw->_FringeScale = 1.2f;


                            ImColor finalColor = isKnocked
                                ? ImColor(255, 0, 0, 255)
                                : ImColor(ImGui::GetColorU32(bonesColor));


                            auto DrawBone = [&](uint32_t offsetFrom, uint32_t offsetTo) {
                                uint32_t boneA = 0, boneB = 0;
                                Vector3  posA{}, posB{};
                                if (!ReadZ(entity + offsetFrom, boneA) || !boneA) return;
                                if (!ReadZ(entity + offsetTo, boneB) || !boneB) return;
                                if (!GetNodePosition(boneA, posA)) return;
                                if (!GetNodePosition(boneB, posB)) return;

                                auto f = WorldToScreenImVec2(viewMatrix, posA, width, height);
                                auto t = WorldToScreenImVec2(viewMatrix, posB, width, height);
                                if (f.x > 0 && f.y > 0 && t.x > 0 && t.y > 0)
                                    draw->AddLine(
                                        ImVec2(x + f.x, y + f.y),
                                        ImVec2(x + t.x, y + t.y),
                                        finalColor, boneThickness);
                                };

                            float circleSize = 3.f * std::clamp(100.0f / DistanceA, 0.5f, 1.5f);
                            draw->AddCircle(
                                ImVec2(x + headScreenPos.x, y + headScreenPos.y),
                                circleSize, finalColor, 0, boneThickness);

                            DrawBone(Offsets::Bones::Head, Offsets::Bones::Neck);
                            DrawBone(Offsets::Bones::Neck, Offsets::Bones::Hip);
                            DrawBone(Offsets::Bones::Hip, Offsets::Bones::Root);
                            DrawBone(Offsets::Bones::Neck, Offsets::Bones::LeftShoulder);
                            DrawBone(Offsets::Bones::LeftShoulder, Offsets::Bones::LeftElbow);
                            DrawBone(Offsets::Bones::LeftElbow, Offsets::Bones::LeftWrist);
                            DrawBone(Offsets::Bones::LeftWrist, Offsets::Bones::LeftHand);
                            DrawBone(Offsets::Bones::Neck, Offsets::Bones::RightShoulder);
                            DrawBone(Offsets::Bones::RightShoulder, Offsets::Bones::RightElbow);
                            DrawBone(Offsets::Bones::RightElbow, Offsets::Bones::RightWrist);
                            DrawBone(Offsets::Bones::Hip, Offsets::Bones::LeftFoot);
                            DrawBone(Offsets::Bones::Hip, Offsets::Bones::RightFoot);
                        }

                        if (ShowFov)
                        {
                            ImVec2 fovCenter = ImVec2(x + width / 2.0f, y + height / 2.0f);
                            ImDrawList* draw = ImGui::GetBackgroundDrawList();
                            draw->PushClipRect(ImVec2(x, y), ImVec2(x + width, y + height), true);

                            if (rgbMode) {
                                DrawAnimatedRainbowFOVWithGlow(fovCenter, FovAll, 1.f, 0.f, 100, 0.0f);
                            }
                            else {
                                draw->AddCircle(fovCenter, FovAll, ImGui::GetColorU32(ColorFov), 100, 1.f);
                            }

                            ImVec2 titleSize = ImGui::CalcTextSize(cheat_name);
                            ImVec2 titlePos = ImVec2(fovCenter.x - titleSize.x / 2.0f,
                                fovCenter.y - FovAll - titleSize.y - 8.0f);
                            ImGui::PushFont(font::lexend_bold);
                            for (int ox = -1; ox <= 1; ++ox)
                                for (int oy = -1; oy <= 1; ++oy)
                                    if (ox != 0 || oy != 0)
                                        draw->AddText(ImVec2(titlePos.x + ox, titlePos.y + oy), IM_COL32(0, 0, 0, 255), cheat_name);
                            draw->AddText(titlePos, IM_COL32(255, 255, 255, 255), cheat_name);
                            ImGui::PopFont();
                            draw->PopClipRect();
                        }

                        bool isMouseButtonHeld = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

                        if (FastReload) {
                            uint32_t reload = 0;
                            if (ReadZ(localPlayer + Offsets::PlayerAttributes, reload) && reload)
                                WriteZ(reload + Offsets::NoReload2, true);
                        }
                        else {
                            uint32_t reload = 0;
                            if (ReadZ(localPlayer + Offsets::PlayerAttributes, reload) && reload)
                                WriteZ(reload + Offsets::NoReload2, false);
                        }

                        if (norecoilpro) {
                            uint32_t weapon = 0;
                            if (ReadZ(localPlayer + Offsets::Weapon, weapon)) {
                                uint32_t weaponData = 0;
                                if (ReadZ(weapon + Offsets::WeaponData, weaponData)) {
                                    float recoil = 0.0f;
                                    if (ReadZ(weaponData + Offsets::WeaponRecoil, recoil) && recoil != recoiloff)
                                        WriteZ(weaponData + Offsets::WeaponRecoil, recoiloff);
                                }
                            }
                        }

                        if (UpPlayer && !IsTeam) {
                            uint32_t enemyRootBonePtr = 0;
                            if (ReadZ(entity + Offsets::Bones::Root, enemyRootBonePtr)) {
                                Vector3 enemyRootPos;
                                if (GetNodePosition(enemyRootBonePtr, enemyRootPos)) {
                                    float playerDistance = Vector3::Distance(mainPos, enemyRootPos);
                                    if (playerDistance <= 100.0f) {
                                        uint32_t t1 = 0, t2 = 0, mat = 0;
                                        ReadZ(enemyRootBonePtr + 0x8, t1);
                                        ReadZ(t1 + 0x8, t2);
                                        ReadZ(t2 + 0x20, mat);
                                        Vector3 cur;
                                        if (ReadZ<Vector3>(mat + 0x80, cur))
                                            WriteZ<Vector3>(mat + 0x80, cur + Vector3(0, UpPlayerAlture, 0));
                                    }
                                }
                            }
                        }

                        if (TeleKill && !IsTeam) {
                            uint32_t localHeadBonePtr = 0;
                            Vector3  localHeadPos;
                            if (ReadZ(localPlayer + Offsets::Bones::Head, localHeadBonePtr) &&
                                GetNodePosition(localHeadBonePtr, localHeadPos))
                            {
                                uint32_t enemyHeadBonePtr = 0;
                                if (ReadZ(entity + Offsets::Bones::Head, enemyHeadBonePtr)) {
                                    Vector3 enemyHeadPos;
                                    if (GetNodePosition(enemyHeadBonePtr, enemyHeadPos)) {
                                        float dist = Vector3::Distance(localHeadPos, enemyHeadPos);
                                        if (dist <= enemyplayerdistance) {
                                            uint32_t localRootBonePtr = 0;
                                            if (ReadZ(localPlayer + Offsets::Bones::Root, localRootBonePtr)) {
                                                uint32_t t1 = 0, t2 = 0, mat = 0;
                                                ReadZ(localRootBonePtr + 0x8, t1);
                                                ReadZ(t1 + 0x8, t2);
                                                ReadZ(t2 + 0x20, mat);
                                                WriteZ<Vector3>(mat + 0x80, enemyHeadPos);
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        if (TeleportBase && !IsTeam) {
                            uint32_t localRootBonePtr = 0;
                            if (ReadZ(localPlayer + Offsets::Bones::Root, localRootBonePtr)) {
                                Vector3 localRootPos;
                                if (GetNodePosition(localRootBonePtr, localRootPos)) {
                                    uint32_t enemyRootBonePtr = 0;
                                    if (ReadZ(entity + Offsets::Bones::Root, enemyRootBonePtr)) {
                                        Vector3 enemyPos;
                                        if (GetNodePosition(enemyRootBonePtr, enemyPos)) {
                                            if (Vector3::Distance(localRootPos, enemyPos) <= teleportbasedistance) {
                                                uint32_t t1 = 0, t2 = 0, mat = 0;
                                                ReadZ(localRootBonePtr + 0x8, t1);
                                                ReadZ(t1 + 0x8, t2);
                                                ReadZ(t2 + 0x20, mat);
                                                WriteZ<Vector3>(mat + 0x80, enemyPos);
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        if (TpBaseCheck && !IsTeam) {
                            uint32_t localRootBonePtr = 0;
                            if (ReadZ(localPlayer + Offsets::Bones::Root, localRootBonePtr)) {
                                Vector3 localRootPos;
                                if (GetNodePosition(localRootBonePtr, localRootPos)) {
                                    uint32_t enemyRootBonePtr = 0;
                                    if (ReadZ(entity + Offsets::Bones::Root, enemyRootBonePtr)) {
                                        Vector3 enemyPos;
                                        if (GetNodePosition(enemyRootBonePtr, enemyPos)) {
                                            if (Vector3::Distance(localRootPos, enemyPos) <= TpDistance) {
                                                uint32_t t1 = 0, t2 = 0, mat = 0;
                                                ReadZ(localRootBonePtr + 0x8, t1);
                                                ReadZ(t1 + 0x8, t2);
                                                ReadZ(t2 + 0x20, mat);
                                                WriteZ<Vector3>(mat + 0x80, enemyPos);
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        if (TpEnemyCheck && !IsTeam) {
                            uint32_t localRootBonePtr = 0, localHeadBonePtr = 0;
                            Vector3  localRootPos, localHeadPos;
                            if (ReadZ(localPlayer + Offsets::Bones::Root, localRootBonePtr) && GetNodePosition(localRootBonePtr, localRootPos) &&
                                ReadZ(localPlayer + Offsets::Bones::Head, localHeadBonePtr) && GetNodePosition(localHeadBonePtr, localHeadPos))
                            {
                                Vector3 forwardDir = Vector3::Normalized(
                                    Vector3(localHeadPos.X, 0, localHeadPos.Z) - Vector3(localRootPos.X, 0, localRootPos.Z));
                                Vector3 rightDir = Vector3::Normalized(Vector3(forwardDir.Z, 0, -forwardDir.X));
                                Vector3 offsetPos = localRootPos + forwardDir * EnemyDistance + rightDir * EnemySide;

                                uint32_t enemyRootBonePtr = 0;
                                if (ReadZ(entity + Offsets::Bones::Root, enemyRootBonePtr)) {
                                    Vector3 enemyPos;
                                    if (GetNodePosition(enemyRootBonePtr, enemyPos) &&
                                        Vector3::Distance(localRootPos, enemyPos) <= EnemyDistanceX)
                                    {
                                        uint32_t t1 = 0, t2 = 0, mat = 0;
                                        ReadZ(enemyRootBonePtr + 0x8, t1);
                                        ReadZ(t1 + 0x8, t2);
                                        ReadZ(t2 + 0x20, mat);
                                        Vector3 cur;
                                        if (ReadZ<Vector3>(mat + 0x80, cur) &&
                                            Vector3::Distance(cur, offsetPos) > 0.01f)
                                            WriteZ<Vector3>(mat + 0x80, offsetPos);
                                    }
                                }
                            }
                        }

                        if (DownPlayerEnable && !IsTeam && !isDead) {
                            uint32_t rootBonePtr = 0;
                            if (ReadZ(entity + Offsets::Bones::Root, rootBonePtr)) {
                                uint32_t t1 = 0, t2 = 0, mat = 0;
                                ReadZ(rootBonePtr + 0x8, t1); ReadZ(t1 + 0x8, t2); ReadZ(t2 + 0x20, mat);
                                Vector3 cur;
                                if (ReadZ<Vector3>(mat + 0x80, cur)) {
                                    cur.Y -= DownPlayerSpeed;
                                    WriteZ<Vector3>(mat + 0x80, cur);
                                }
                            }
                        }

                        if (FlyWall) {
                            uint32_t rootPtr = 0;
                            if (ReadZ<uint32_t>(localPlayer + (uint32_t)Offsets::Bones::Root, rootPtr) && rootPtr) {
                                uint32_t t1 = 0, t2 = 0, mat = 0;
                                ReadZ<uint32_t>(rootPtr + 0x8, t1); ReadZ<uint32_t>(t1 + 0x8, t2); ReadZ<uint32_t>(t2 + 0x20, mat);
                                Vector3 cur;
                                if (ReadZ<Vector3>(mat + 0x80, cur)) {
                                    if (!isFlywallActive) {
                                        Vector3 newPos = cur; newPos.Y -= flywallOffset;
                                        for (int fi = 0; fi < 50; fi++) {
                                            WriteZ<Vector3>(mat + 0x80, newPos);
                                            std::this_thread::sleep_for(std::chrono::milliseconds(flywallTickDelay));
                                        }
                                        flywallPosition = newPos; isFlywallActive = true;
                                    }
                                    else {
                                        Vector3 posNow;
                                        if (ReadZ<Vector3>(mat + 0x80, posNow)) {
                                            if (Vector3::Distance(posNow, flywallPosition) > 0.3f) isFlywallActive = false;
                                            else WriteZ<Vector3>(mat + 0x80, flywallPosition);
                                        }
                                    }
                                }
                            }
                        }
                        else {
                            isFlywallActive = false;
                        }

                        if (FlyHackin) {
                            uint32_t localRootBonePtr = 0;
                            if (ReadZ(localPlayer + Offsets::Bones::Root, localRootBonePtr)) {
                                Vector3 localRootPos;
                                if (GetNodePosition(localRootBonePtr, localRootPos)) {
                                    Vector3 newPos = localRootPos; newPos.Y -= 2.0f;
                                    uint32_t t1 = 0, t2 = 0, mat = 0;
                                    ReadZ(localRootBonePtr + 0x8, t1); ReadZ(t1 + 0x8, t2); ReadZ(t2 + 0x20, mat);
                                    WriteZ<Vector3>(mat + 0x80, newPos);
                                }
                            }
                        }

                        {
                            static uint32_t lastValidSpeedTimer = 0;
                            uint32_t bGF = 0, gF = 0, sGF = 0, cG = 0, sT = 0;
                            if (Offsets::Il2Cpp != 0 &&
                                ReadZ(Offsets::Il2Cpp + Offsets::InitBase, bGF) && bGF &&
                                ReadZ(bGF, gF) && gF &&
                                ReadZ(gF + Offsets::StaticClass, sGF) && sGF &&
                                ReadZ(sGF, cG) && cG &&
                                ReadZ(cG + 0x10, sT) && sT)
                            {
                                lastValidSpeedTimer = sT;
                                WriteZ(sT + 0x24, SpeedTimerEnable ? 0.065000f : 0.033000f);
                            }
                        }

                        if (Fastrun || Speed) {
                            uint32_t pa = 0;
                            if (ReadZ(localPlayer + Offsets::PlayerAttributes, pa) && pa)
                                WriteZ<float>(pa + 0x50, Fastrun ? 0.2f : 0.1f);
                        }
                        else {
                            uint32_t pa = 0;
                            if (ReadZ(localPlayer + Offsets::PlayerAttributes, pa) && pa)
                                WriteZ<float>(pa + 0x50, 4.0f);
                        }

                        if (InstantF) {
                            uint32_t XDE = 0;
                            if (ReadZ(localPlayer + Offsets::PlayerAttributes, XDE) && XDE)
                                WriteZ<float>(XDE + 0x17C, valueFire);
                        }

                        if (AimbotHex && !IsTeam) {
                            ImVec2 screenCtr(g_windowWidth / 2.0f, g_windowHeight / 2.0f);
                            ImVec2 targetPos = WorldToScreenImVec2(viewMatrix, bonePositions.HeadV3, g_windowWidth, g_windowHeight);
                            float dx = targetPos.x - screenCtr.x, dy = targetPos.y - screenCtr.y;
                            if (targetPos.x > 0 && targetPos.y > 0 &&
                                targetPos.x < g_windowWidth && targetPos.y < g_windowHeight &&
                                (dx * dx + dy * dy) <= (float)fov * fov)
                            {
                                uint32_t rHC = 0;
                                if (ReadZ(entity + Offsets::HeadCollider, rHC) && rHC) {
                                    WriteZ(entity + Offsets::CurrentMatch, rHC);
                                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                                }
                            }
                        }

                        if (AimbotRage && !IsTeam) { // Brutal Silent - replaces Rage AimRotation
                            ImVec2 screenCtr(g_windowWidth / 2, g_windowHeight / 2);
                            ImVec2 targetScreenPos = WorldToScreenImVec2(viewMatrix, bonePositions.HeadV3, g_windowWidth, g_windowHeight);
                            float dist = std::sqrt(std::pow(targetScreenPos.x - screenCtr.x, 2) + std::pow(targetScreenPos.y - screenCtr.y, 2));
                            if (dist <= fov) {
                                bool firing = false;
                                if (ReadZ<bool>(localPlayer + Offsets::IS_FIRING, firing)) {
                                    bool risingEdge = firing && !Brutal_prevFiring;
                                    bool fallingEdge = !firing && Brutal_prevFiring;
                                    if (fallingEdge) { Brutal_hasLastWrite = false; BrutalReset(); }
                                    if (firing) {
                                        if (BrutalCachedWeaponId == 0) ReadZ<uint32_t>(localPlayer + Offsets::sAim2, BrutalCachedWeaponId);
                                        if (BrutalCachedWeaponId != 0) {
                                            Vector3 startPos{};
                                            if (ReadZ<Vector3>(BrutalCachedWeaponId + Offsets::sAim3, startPos) && !(startPos == Vector3::Zero())) {
                                                Vector3 aimPos = BrutalCalculateAim(bonePositions.HeadV3, mainPos);
                                                Vector3 finalAim = aimPos - startPos;
                                                bool needWrite = risingEdge || !Brutal_hasLastWrite;
                                                if (!needWrite) {
                                                    float moved = Vector3::Distance(finalAim, Brutal_lastWritten);
                                                    if (moved > 0.01f) needWrite = true;
                                                    else {
                                                        Vector3 live{};
                                                        if (ReadZ<Vector3>(BrutalCachedWeaponId + Offsets::sAim4, live))
                                                            needWrite = (live.X != finalAim.X || live.Y != finalAim.Y || live.Z != finalAim.Z);
                                                    }
                                                }
                                                if (needWrite) {
                                                    WriteZ<Vector3>(BrutalCachedWeaponId + Offsets::sAim4, finalAim);
                                                    Brutal_lastWritten = finalAim;
                                                    Brutal_hasLastWrite = true;
                                                }
                                            }
                                        }
                                    }
                                    Brutal_prevFiring = firing;
                                }
                            }
                        }
                        if (ExternalAimbotEnabled && !IsTeam) {
                            bool keyDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
                            CURSORINFO ci{}; ci.cbSize = sizeof(CURSORINFO); bool cursorVisible = true; if (GetCursorInfo(&ci)) cursorVisible = (ci.flags & CURSOR_SHOWING) != 0;
                            if (keyDown && !cursorVisible) {
                                ImVec2 screenCtr(g_windowWidth / 2, g_windowHeight / 2);
                                ImVec2 targetPos = WorldToScreenImVec2(viewMatrix, bonePositions.HeadV3, g_windowWidth, g_windowHeight);
                                float d = std::hypot(targetPos.x - screenCtr.x, targetPos.y - screenCtr.y);
                                if (d <= fov) {
                                    static std::unordered_map<uint32_t, uint32_t> hipRestoreMap;
                                    uint32_t hipAddr = entity + Offsets::Bones::Hip;
                                    uint32_t headCol = 0; if (ReadZ(entity + Offsets::Bones::Head, headCol) && headCol) {
                                        if (hipRestoreMap.find(entity) == hipRestoreMap.end()) { uint32_t orig = 0; ReadZ(hipAddr, orig); hipRestoreMap[entity] = orig; }
                                        WriteZ<uint32_t>(hipAddr, headCol);
                                    }
                                }
                            }
                        }

                        if (AimbotLegit && !IsTeam) {
                            bool isKeyPressed = selectedKey == AimKey::LeftMouseButton
                                ? (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0 : false;
                            if (isKeyPressed) {
                                ImVec2 screenCtr(width / 2.0f, height / 2.0f);
                                ImVec2 targetPos = WorldToScreenImVec2(viewMatrix, bonePositions.HeadV3, width, height);
                                float dx = targetPos.x - screenCtr.x, dy = targetPos.y - screenCtr.y;
                                if ((dx * dx + dy * dy) <= (float)fov * fov) {
                                    WriteZ(localPlayer + Offsets::AimRotation,
                                        AimBZv2::GetRotationToLocation(bonePositions.HeadV3, smoothFactor, mainPos));
                                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                                }
                            }
                        }

                        if (SilentAim && !IsTeam) {
                            ImVec2 screenCtr(g_windowWidth / 2.0f, g_windowHeight / 2.0f);
                            ImVec2 targetPos = WorldToScreenImVec2(viewMatrix, bonePositions.HeadV3, g_windowWidth, g_windowHeight);
                            float dx = targetPos.x - screenCtr.x, dy = targetPos.y - screenCtr.y;
                            float dist = std::sqrt(dx * dx + dy * dy);
                            if (dist <= fov) {
                                bool isFiring = false;
                                if (ReadZ<bool>(localPlayer + Offsets::IS_FIRING, isFiring) && isFiring) {
                                    uint32_t weaponPtr = 0;
                                    if (ReadZ(localPlayer + Offsets::sAim2, weaponPtr) && weaponPtr) {
                                        Vector3 startPos{};
                                        ReadZ<Vector3>(weaponPtr + Offsets::sAim3, startPos);
                                        Vector3 dir = (bonePositions.HeadV3 + Vector3(0, 0.1f, 0)) - startPos;
                                        WriteZ<Vector3>(weaponPtr + Offsets::sAim4, dir);
                                    }
                                }
                            }
                        }

                        if (aimbotVisibleActive && !IsTeam) {
                            bool isKeyPressed = selectedKey == AimKey::LeftMouseButton
                                ? (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0 : false;
                            if (isKeyPressed) {
                                ImVec2 screenCtr(g_windowWidth / 2.0f, g_windowHeight / 2.0f);
                                ImVec2 targetPos = WorldToScreenImVec2(viewMatrix, bonePositions.HeadV3, g_windowWidth, g_windowHeight);
                                if (targetPos.x > 0 && targetPos.y > 0 &&
                                    targetPos.x < g_windowWidth && targetPos.y < g_windowHeight)
                                {
                                    uint32_t rHC = 0;
                                    if (ReadZ(entity + Offsets::HeadCollider, rHC) && rHC) {
                                        float rnd = static_cast<float>(rand()) / RAND_MAX;
                                        if (rnd <= 0.50f) {
                                            WriteZ(entity + Offsets::CurrentMatch, rHC);
                                            std::this_thread::sleep_for(std::chrono::milliseconds(15 + rand() % 10));
                                        }
                                    }
                                }
                            }
                        }

                        if (SpeedTeleportEnable && !IsTeam && !isDead) {
                            float dist = Vector3::Distance(mainPos, bonePositions.RootV3);
                            if (dist <= SpeedTeleportRange && dist < speedTeleportClosestDist) {
                                speedTeleportClosestDist = dist;
                                speedTeleportBestTarget = entity;
                                speedTeleportBestPos = bonePositions.RootV3;
                            }
                        }
                        if (SpeedTeleportEnable && i == entitiesCount - 1 && speedTeleportBestTarget != 0) {
                            auto  now = std::chrono::steady_clock::now();
                            float deltaTime = std::chrono::duration<float>(now - speedTeleportLastTick).count();
                            speedTeleportLastTick = now;
                            uint32_t localRoot = 0;
                            if (ReadZ(localPlayer + Offsets::Bones::Root, localRoot) && localRoot) {
                                uint32_t t1 = 0, t2 = 0, mat = 0;
                                ReadZ(localRoot + 0x8, t1); ReadZ(t1 + 0x8, t2); ReadZ(t2 + 0x20, mat);
                                Vector3 cur;
                                if (ReadZ<Vector3>(mat + 0x80, cur)) {
                                    Vector3 dir = {
                                        speedTeleportBestPos.X - cur.X,
                                        speedTeleportBestPos.Y - cur.Y,
                                        speedTeleportBestPos.Z - cur.Z };
                                    float dist = sqrtf(dir.X * dir.X + dir.Y * dir.Y + dir.Z * dir.Z);
                                    if (dist > 0.1f) {
                                        dir.X /= dist; dir.Y /= dist; dir.Z /= dist;
                                        float move = SpeedTeleportSpeed * deltaTime;
                                        Vector3 next = move >= dist ? speedTeleportBestPos
                                            : Vector3{ cur.X + dir.X * move, cur.Y + dir.Y * move, cur.Z + dir.Z * move };
                                        WriteZ<Vector3>(mat + 0x80, next);
                                    }
                                }
                            }
                            speedTeleportClosestDist = FLT_MAX;
                            speedTeleportBestTarget = 0;
                        }

                        vList->PopClipRect();

                    }
                }


            }


            static int KeyClosed = VK_DELETE;
            static int keyPressCount = 0;

            if (GetAsyncKeyState(KeyClosed) & 1)
            {
                keyPressCount++;

                if (keyPressCount == 1)
                {
                    KillProcessByName("HD-Adb.exe");
                    KillProcessByName("HD-Player.exe");
                    KillProcessByName("Bluestacks.exe");
                    KillProcessByName("BstkSVC.exe");

                    exit(0);

                }
            }



            bool NotificacionByBreyner = true;
            if (NotificacionByBreyner)
            {
                notificationSystem.DrawNotifications();
            }

            if (GetAsyncKeyState(FreezeLagKey) & 0x1)
            {
                FreezeHR = !FreezeHR;
                {
                    Beep(FreezeHR ? 1500 : 800, 100);
                }

                if (FreezeHR)
                {
                    CreateThread(NULL, 0, FakeFreezeHR, NULL, 0, NULL);
                    /*   MemoryLogs = "Freeze Lag - Enable !";
                       ImGui::Notification({ ImGuiToastType_Success, 4000, MemoryLogs.c_str() });*/
                }
                else
                {
                    /* MemoryLogs = "Freeze Lag - Disable !";
                     ImGui::Notification({ ImGuiToastType_Success, 4000, MemoryLogs.c_str() });*/
                }
            }

            if (GetAsyncKeyState(GhostHackKey) & 0x1)
            {
                GhostLagHR = !GhostLagHR;
                {
                    Beep(FreezeHR ? 1500 : 800, 100);
                }

                if (GhostLagHR)
                {
                    CreateThread(NULL, 0, FakeHR, NULL, 0, NULL);
                    /* MemoryLogs = "Ghost Hack - Enable !";
                     ImGui::Notification({ ImGuiToastType_Success, 4000, MemoryLogs.c_str() });*/
                }
                else
                {
                    /*  MemoryLogs = "Ghost Hack - Enable !";
                      ImGui::Notification({ ImGuiToastType_Success, 4000, MemoryLogs.c_str() });*/
                }
            }


            if (GetAsyncKeyState(TeleKillKey) & 0x1)
            {
                TeleKill = !TeleKill;
                Beep(TeleKill ? 1500 : 800, 100);
            }

            if (GetAsyncKeyState(TeleportKey) & 0x1)
            {
                TeleportBase = !TeleportBase;
                Beep(TeleportBase ? 1500 : 800, 100);
            }

            if (GetAsyncKeyState(UpPlayerKey) & 0x1)
            {
                UpPlayer = !UpPlayer;
                Beep(UpPlayer ? 1500 : 800, 100);
            }

            if (GetAsyncKeyState(spdkey) & 0x1)
            {
                SpeedTeleportEnable = !SpeedTeleportEnable;
                Beep(SpeedTeleportEnable ? 1500 : 800, 100);
            }


            if (GetAsyncKeyState(Enmkey) & 0x1)
            {
                PullCheck = !PullCheck;
                Beep(PullCheck ? 1500 : 800, 100);
            }



            if (GetAsyncKeyState(climbkey) & 0x1)
            {
                ClimbUpEnable = !ClimbUpEnable;
                Beep(ClimbUpEnable ? 1500 : 800, 100);
            }



            if (GetAsyncKeyState(dvkey) & 0x1)
            {
                DiveKillEnable = !DiveKillEnable;
                Beep(DiveKillEnable ? 1500 : 800, 100);
            }


            if (GetAsyncKeyState(dwkey) & 0x1)
            {
                DownPlayerEnable = !DownPlayerEnable;
                Beep(DownPlayerEnable ? 1500 : 800, 100);
            }

            if (GetAsyncKeyState(tpkey) & 0x1)
            {
                TpBaseCheck = !TpBaseCheck;
                Beep(TpBaseCheck ? 1500 : 800, 100);
            }

            if (GetAsyncKeyState(enkey) & 0x1)
            {
                TpEnemyCheck = !TpEnemyCheck;
                Beep(TpEnemyCheck ? 1500 : 800, 100);
            }

            if (GetAsyncKeyState(flykey1) & 0x1)
            {
                FlyWall = !FlyWall;
                Beep(FlyWall ? 1500 : 800, 100);
            }
            if (GetAsyncKeyState(tpkey) & 0x1)
            {
                TeleportMarkEnable = !TeleportMarkEnable;
                Beep(TeleportMarkEnable ? 1500 : 800, 100);
            }





            if (GetAsyncKeyState(camerakey) & 0x1)
            {
                /* cameraleft = !cameraleft;
                 if (cameraleft)
                 {
                     memexter.CameraOn();
                 }
                 else
                 {
                     memexter.CameraOff();
                 }*/
            }

            if (GetAsyncKeyState(fastkey) & 0x1)
            {
                bSpeedHackEnabled = !bSpeedHackEnabled;
                Beep(bSpeedHackEnabled ? 1500 : 800, 100);
            }


            static int HideMenu = VK_INSERT;

            if (GetAsyncKeyState(HideMenu) & 1) {

                if (Login_Verified) {
                    isClickable = !isClickable;
                    Main_Window = !Main_Window;
                    ToggleClickability(isClickable);
                }
                else {
                    isClickable = !isClickable;
                    Login_Window = !Login_Window;

                    ToggleClickability(isClickable);
                }

                ToggleClickability(isClickable);
            }



            ImGuiStyle* style = &ImGui::GetStyle();
            style->WindowPadding = ImVec2(0, 0);
            style->ItemSpacing = ImVec2(10, 10);
            style->WindowBorderSize = 0;
            style->ScrollbarSize = 8.f;






            if (Login_Window)
            {
                if (g_loginState == 2)
                {
                    g_loginState = 0;
                    notificationSystem.AddNotification("Login Success !", 2500);
                    Main_Window = true;
                    Login_Window = false;
                    Login_Verified = true;
                }
                else if (g_loginState == 3)
                {
                    g_loginState = 0;
                    Beep(500, 300);
                    notificationSystem.AddNotification(g_loginError[0] ? g_loginError : "Login Failed !", 4000);
                    g_loginError[0] = 0;
                }

                ImGui::SetNextWindowSize(ImVec2(c::background::size.x, c::background::size.y));

                ImGui::Begin("Login", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
                {


                    const auto& p = ImGui::GetWindowPos();
                    const ImVec2& pos = ImGui::GetWindowPos();
                    const ImVec2& size = ImGui::GetWindowSize();
                    auto background_list = ImGui::GetBackgroundDrawList();

                    float time = (float)ImGui::GetTime();
                    float pulse = (sinf(time * 3.0f) + 1.0f) * 0.5f;

                    ImVec2 bottom_right = ImVec2(pos.x + size.x, pos.y + size.y);
                    ImVec2 glow_center = ImVec2(pos.x + size.x - 355, pos.y + size.y + 100);

                    float ambientGlow = 1100.f + pulse * 100.f;
                    float borderGlow = 120.f + pulse * 40.f;

                    auto draw = ImGui::GetWindowDrawList();
                    draw->AddRectFilled(pos, pos + ImVec2(c::background::size), ImColor(c::background::filling), 4.f, ImDrawFlags_RoundCornersTop);

                    ImGui::GetWindowDrawList()->AddShadowRect({ glow_center }, { glow_center }, ImColor(c::accent), ambientGlow, ImVec2(0, 0));
                    background_list->AddShadowRect(pos, bottom_right, ImColor(c::accent), borderGlow, ImVec2(0, 0));


                    RainEffect();


                    ImGui::PushFont(Nev);
                    float titleGlow = 120.0f + pulse * 50.0f;
                    ImVec2 ninjaSize = ImGui::CalcTextSize("DELTA ");
                    ImVec2 hexSize = ImGui::CalcTextSize("INTERNAL");
                    float totalTitleWidth = ninjaSize.x + hexSize.x;
                    float titleStartX = (c::background::size.x - totalTitleWidth) / 2.0f;

                    ImGui::ShadowText("DELTA ", ImColor(135, 206, 250, 255), ImColor(0, 120, 255), titleGlow, ImVec2(titleStartX, 50));
                    ImGui::ShadowText("INTERNAL", ImColor(255, 255, 255, 255), ImColor(0, 120, 255), titleGlow, ImVec2(titleStartX + ninjaSize.x, 50));
                    ImGui::PopFont();

                    ImGui::PushFont(Nev1);
                    const char* subtitleStr = "WELCOME BACK!";
                    ImVec2 subtitleSize = ImGui::CalcTextSize(subtitleStr);
                    float subtitleX = (c::background::size.x - subtitleSize.x) / 2.0f;
                    ImGui::SetCursorPos(ImVec2(subtitleX, 100));
                    ImGui::TextColored(ImColor(100 + (int)(pulse * 55), 200 + (int)(pulse * 55), 255, 200), subtitleStr);

                    ImGui::SetCursorPos(ImVec2(c::background::size.x - 25, 15));
                    ImGui::TextColored(ImColor(135, 206, 250, 255), "X");
                    if (ImGui::IsItemClicked()) exit(0);
                    ImGui::PopFont();

                    ImGui::SetCursorPos(ImVec2(0, 0));
                    ImGui::BeginChild("Inputs", ImVec2(c::background::size), false, ImGuiWindowFlags_NoScrollbar);
                    {
                        PushFont(font::lexend_bold);

                        ImGui::SetCursorPos(ImVec2(55, 130));
                        ImGui::InputTextEx("", "Username", user, 64, ImVec2(290, 40), ImGuiInputTextFlags_None, NULL, NULL);

                        ImGui::SetCursorPos(ImVec2(55, 190));
                        ImGui::InputTextEx("", "Password", pass, 64, ImVec2(290, 40), ImGuiInputTextFlags_Password, NULL, NULL);

                        ImGui::SetCursorPos(ImVec2(55, 260));

                        ImVec4 btnColor = ImVec4(135 / 255.f - pulse * 0.1f, 206 / 255.f + pulse * 0.1f, 250 / 255.f, 1.0f);
                        ImGui::PushStyleColor(ImGuiCol_Button, btnColor);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(155, 226, 255));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(115, 186, 250));
                        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(0, 0, 0));

                        if (g_loginState == 1)
                        {
                            ImGui::TextColored(ImColor(200, 200, 200, 255), "Logging in...");
                        }
                        else if (ImGui::Button("Login >", ImVec2(290, 40)))
                        {
                            if (strlen(user) == 0 || strlen(pass) == 0)
                            {
                                snprintf(g_loginError, sizeof(g_loginError), "Fill username & password!");
                                notificationSystem.AddNotification("Fill username & password!", 2500);
                            }
                            else if (g_loginState == 0)
                            {
                                g_loginState = 1;
                                std::thread taskThread([]()
                                    {
                                        g_loginState = SafeKeyAuthLogin();
                                    });
                                taskThread.detach();
                            }
                        }
                        if (g_loginError[0] != 0)
                        {
                            ImGui::SetCursorPos(ImVec2(55, 305));
                            ImGui::TextColored(ImColor(255, 80, 80, 255), g_loginError);
                        }
                        ImGui::PopStyleColor(4);

                        const char* str1 = "Don't have an account? ";
                        const char* str2 = "Register";
                        ImVec2 s1 = ImGui::CalcTextSize(str1);
                        ImVec2 s2 = ImGui::CalcTextSize(str2);
                        float totalW = s1.x + s2.x;
                        float startX = (c::background::size.x - totalW) / 2.0f;

                        ImGui::SetCursorPos(ImVec2(startX, 315));
                        ImGui::TextColored(ImColor(150, 150, 150, 255), str1);
                        ImGui::SameLine(0, 0);
                        ImGui::TextColored(ImColor(135, 206, 250, 255), str2);

                        PopFont();

                        PushFont(font::lexend_regular);
                        ImVec2 textSize = ImGui::CalcTextSize("DELTA INTERNAL  copyright 2026");
                        ImGui::SetCursorPos(ImVec2((c::background::size.x - textSize.x) / 2, c::background::size.y - 40));
                        ImGui::TextColored(ImColor(100, 100, 100, 255), "DELTA INTERNAL  copyright 2026");
                        PopFont();
                    }
                    ImGui::EndChild();

                }
                ImGui::End();
            }


            if (Main_Window) {


                Login_Verified = true;
                ImGui::SetNextWindowSize(ImVec2(c::background::size1.x, c::background::size1.y));

                ImGui::Begin("IMGUI", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
                {

                    RainEffect();
                    float textOffsetX = 170;
                    float textPosY = 470;


                    ImVec2 xy = ImGui::GetWindowPos();
                    auto backgroud_list = ImGui::GetBackgroundDrawList();
                    backgroud_list->AddShadowRect(xy, ImVec2(xy.x + 700, xy.y + 550), ImColor(c::accent), 120.f, ImVec2(0, 0));
                    const ImVec2& pos = GetWindowPos();
                    const ImVec2& region = GetContentRegionMax();
                    const ImVec2& spacing = style->ItemSpacing;
                    auto draw = ImGui::GetWindowDrawList();
                    const auto& p = ImGui::GetWindowPos();
                    GetBackgroundDrawList()->AddRectFilled(pos, pos + c::background::size1, GetColorU32(c::background::filling), c::background::rounding);





                    static double nameStartTime = -1.0;
                    if (nameStartTime < 0.0) nameStartTime = ImGui::GetTime();
                    const double elapsed = ImGui::GetTime() - nameStartTime;
                    const std::string firstWord = "DELTA ";
                    const std::string secondWord = " INTERNAL";
                    const float lettersPerSecond = 3.0f;
                    const int totalLetters = (int)firstWord.size() + (int)secondWord.size();
                    const float typeDuration = (float)totalLetters / lettersPerSecond;
                    const float pauseDuration = 0.6f;
                    const float cycleDuration = typeDuration + pauseDuration;
                    const float cyclePos = fmodf((float)elapsed, cycleDuration);
                    int visibleLetters = (cyclePos >= typeDuration)
                        ? totalLetters
                        : (int)floorf(cyclePos * lettersPerSecond);
                    const int firstVisible = (std::min)((int)firstWord.size(), visibleLetters);
                    const int secondVisible = (std::max)(0, visibleLetters - (int)firstWord.size());

                    ImGui::PushFont(Nev);
                    {

                        const float appearAnimDuration = 0.35f;
                        const float flashDuration = 0.08f; // quick flash like camera
                        float xCursor = 0.0f;
                        ImVec2 basePosFirst = ImVec2(25, 20);
                        ImDrawList* dl = ImGui::GetForegroundDrawList();

                        for (int i = 0; i < firstVisible; ++i)
                        {
                            float charAppearTime = (float)i / lettersPerSecond;
                            float timeSinceAppear = cyclePos - charAppearTime;
                            if (timeSinceAppear < 0.0f) continue;

                            float t = timeSinceAppear / appearAnimDuration;
                            if (t > 1.0f) t = 1.0f;
                            if (t < 0.0f) t = 0.0f;

                            // Photo flash effect (bright white flash at start)
                            float flashT = timeSinceAppear / flashDuration;
                            bool inFlash = flashT < 1.0f && flashT >= 0.0f;

                            // Photo-developing effect: start with sepia/white, fade to full color
                            float photoT = (t - flashDuration / appearAnimDuration) / (1.0f - flashDuration / appearAnimDuration);
                            if (photoT < 0.0f) photoT = 0.0f;
                            if (photoT > 1.0f) photoT = 1.0f;

                            char buf[2] = { firstWord[(size_t)i], 0 };
                            ImVec2 charSize = ImGui::CalcTextSize(buf);
                            ImVec2 charPos = basePosFirst + ImVec2(xCursor, 0);

                            // Flash effect (bright white glow)
                            if (inFlash)
                            {
                                float flashIntensity = 1.0f - flashT;
                                ImVec4 flashCol = ImVec4(1.0f, 1.0f, 1.0f, flashIntensity * 0.9f);
                                dl->AddRectFilled(charPos - ImVec2(2, 2), charPos + charSize + ImVec2(2, 2), ImGui::GetColorU32(flashCol), 2.0f);
                            }

                            // Photo-developing color transition
                            ImVec4 baseColor = ImGui::ColorConvertU32ToFloat4(ImColor(c::accent));
                            if (photoT < 1.0f)
                            {
                                // Start with sepia/white tint, transition to full color
                                float sepiaMix = 1.0f - photoT;
                                baseColor.x = baseColor.x * (1.0f - sepiaMix * 0.3f) + sepiaMix * 0.95f;
                                baseColor.y = baseColor.y * (1.0f - sepiaMix * 0.3f) + sepiaMix * 0.95f;
                                baseColor.z = baseColor.z * (1.0f - sepiaMix * 0.2f) + sepiaMix * 0.9f;
                                baseColor.w *= (0.3f + photoT * 0.7f); // fade in
                            }
                            else
                            {
                                baseColor.w = 1.0f;
                            }

                            ImU32 textCol = ImGui::GetColorU32(baseColor);

                            // Draw with photo-like glow
                            float glowSize = 150.f * (0.5f + photoT * 0.5f);
                            ImGui::ShadowText(buf, textCol, textCol, glowSize, charPos);

                            xCursor += charSize.x;
                        }

                        xCursor = 0.0f;
                        ImVec2 basePosSecond = ImVec2(160, 20);
                        for (int i = 0; i < (std::min)((int)secondWord.size(), secondVisible); ++i)
                        {
                            int absoluteIndex = (int)firstWord.size() + i;
                            float charAppearTime = (float)absoluteIndex / lettersPerSecond;
                            float timeSinceAppear = cyclePos - charAppearTime;
                            if (timeSinceAppear < 0.0f) continue;

                            float t = timeSinceAppear / appearAnimDuration;
                            if (t > 1.0f) t = 1.0f;
                            if (t < 0.0f) t = 0.0f;

                            float flashT = timeSinceAppear / flashDuration;
                            bool inFlash = flashT < 1.0f && flashT >= 0.0f;

                            float photoT = (t - flashDuration / appearAnimDuration) / (1.0f - flashDuration / appearAnimDuration);
                            if (photoT < 0.0f) photoT = 0.0f;
                            if (photoT > 1.0f) photoT = 1.0f;

                            char buf[2] = { secondWord[(size_t)i], 0 };
                            ImVec2 charSize = ImGui::CalcTextSize(buf);
                            ImVec2 charPos = basePosSecond + ImVec2(xCursor, 0);

                            if (inFlash)
                            {
                                float flashIntensity = 1.0f - flashT;
                                ImVec4 flashCol = ImVec4(1.0f, 1.0f, 1.0f, flashIntensity * 0.9f);
                                dl->AddRectFilled(charPos - ImVec2(2, 2), charPos + charSize + ImVec2(2, 2), ImGui::GetColorU32(flashCol), 2.0f);
                            }

                            ImVec4 white = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                            if (photoT < 1.0f)
                            {
                                float sepiaMix = 1.0f - photoT;
                                white.x = white.x * (1.0f - sepiaMix * 0.3f) + sepiaMix * 0.95f;
                                white.y = white.y * (1.0f - sepiaMix * 0.3f) + sepiaMix * 0.95f;
                                white.z = white.z * (1.0f - sepiaMix * 0.2f) + sepiaMix * 0.9f;
                                white.w = 0.3f + photoT * 0.7f;
                            }

                            ImU32 whiteCol = ImGui::GetColorU32(white);
                            float glowSize = 110.f * (0.5f + photoT * 0.5f);
                            ImGui::ShadowText(buf, whiteCol, whiteCol, glowSize, charPos);
                            xCursor += charSize.x;
                        }
                    }
                    ImGui::PopFont();




                    SetCursorPos(ImVec2(10, 195));
                    BeginGroup();

                    ImGui::SetCursorPos(ImVec2(490, 10));
                    ImGui::BeginGroup();

                    if (edited::Tab(page == 0, pic::aimbot, "", "Aimbot", ImVec2(38, 38), NULL)) {
                        page = 0;
                    }

                    ImGui::SetCursorPos(ImVec2(530, 10));

                    if (edited::Tab(page == 1, pic::visuals, "", "Visual", ImVec2(38, 38), NULL)) {
                        page = 1;
                    }

                    ImGui::SetCursorPos(ImVec2(570, 10));

                    if (edited::Tab(page == 2, pic::skins, "", "Extra", ImVec2(38, 38), NULL)) {
                        page = 2;
                    }

                    ImGui::SetCursorPos(ImVec2(610, 10));

                    if (edited::Tab(page == 3, pic::extra, "", "Misc", ImVec2(38, 38), NULL)) {
                        page = 3;
                    }

                    ImGui::SetCursorPos(ImVec2(650, 10));

                    if (edited::Tab(page == 4, pic::settings, "", "Setting", ImVec2(38, 38), NULL)) {
                        page = 4;
                    }

                    /* ImGui::SetCursorPos(ImVec2(10, 550));
                     if (edited::TabsNew(5 == page, pic::info11, "info", ImVec2(130 - spacing.x * 3, 35), NULL)) page = 6;

                     ImGui::SetCursorPos(ImVec2(50, 550));
                     if (edited::TabsNew(6 == page, pic::log11, "Logout", ImVec2(130 - spacing.x * 3, 35), NULL)) page = 7;

                     ImGui::SetCursorPos(ImVec2(650, 550));
                     if (edited::TabsNew(7 == page, pic::more, "more", ImVec2(130 - spacing.x * 3, 35), NULL)) page = 8;*/

                    ImGui::EndGroup();


                    EndGroup();

                    tab_alpha = ImLerp(tab_alpha, (page == active_tab) ? 1.f : 0.f, 15.f * ImGui::GetIO().DeltaTime);
                    if (tab_alpha < 0.01f && tab_add < 0.01f) active_tab = page;
                    ImGui::SetCursorPos(ImVec2
                    (
                        150, 100 - (tab_alpha * 100)
                    ));



                    if (active_tab == 0)
                    {
                        ImGui::SetCursorPos(ImVec2(30, 80));
                        edited::BeginChild("a", "Aim Hacks", ImVec2(310, 400), ImGuiChildFlags_Border, 0);
                        {
                            bool SilentHookG;

                            static int selected = 0;
                            static int previousSelect = -1;
                            const char* items[] = { "None", "Silent" };

                            edited::Combo("Aimbot Type", "", &selected, items, IM_ARRAYSIZE(items));

                            if (selected != previousSelect) {

                                AimbotRage = false;
                                BrutalSilent = false;
                                AimbotAI = false;
                                AimbotVisible = false;
                                aimsilent = false;
                                aimbotVisibleActive = false;
                                SilentAim = false;
                                AimbotHex = false;
                                AimbotLegit = false;

                                switch (selected) {
                                case 0: // None
                                    BrutalSilent = false;
                                    break;
                                case 1:
                                    AimbotRage = true;
                                    BrutalSilent = true;
                                    break;

                                }

                                previousSelect = selected;
                            }

                            if (AimbotAI || AimbotHex) {
                                edited::SliderInt("Headshot Delay", "ms", &delayToHeadMs, 50, 300, "%d ms");
                                edited::SliderFloat("Aim Distance", "m", &AimDistancia, 50.0f, 300.0f, "%.0f m");
                                edited::Checkbox("Velocity Prediction", "Track enemy movement", &predictionEnable);
                                edited::SliderInt("FOV", "", &fov, 1, 360);
                            }

                            if (AimbotRage) {
                                edited::SliderInt("Silent FOV", "", &fov, 1, 360);
                                /*   edited::SliderFloat("Smooth Factor", "", &smoothFactor, 1.0f, 20.0f, "%.1f");*/
                            }

                            if (AimbotLegit || aimbotVisibleActive) {
                                edited::SliderInt("Legit FOV", "", &fov, 1, 180);
                                edited::SliderFloat("Smoothness", "", &smoothFactor, 1.0f, 30.0f, "%.1f");
                                const char* aimKeys[] = { "Left Mouse Button", "Right Mouse Button", "Shift", "Ctrl" };
                                static int aimKeySelected = 0;
                                edited::Combo("Aim Key", "", &aimKeySelected, aimKeys, IM_ARRAYSIZE(aimKeys));
                                if (aimKeySelected == 0) selectedKey = AimKey::LeftMouseButton;
                                else if (aimKeySelected == 1) selectedKey = AimKey::RightMouseButton;

                            }


                            if (SilentAim || aimsilent) {
                                edited::SliderInt("Silent FOV", "", &fov, 1, 180);
                                const char* silentTargets[] = { "Head", "Neck", "Chest", "Pelvis" };
                                static int silentTargetSelected = 0;
                                edited::Combo("Target Bone", "", &silentTargetSelected, silentTargets, IM_ARRAYSIZE(silentTargets));
                            }


                            edited::Keybind("EnemyPull Head - Key", "Choose any key for ON/OFF", &Enmkey);






                            if (edited::Checkbox("Magnet Pull", "In Game", &mgn))
                            {
                                if (mgn)
                                {
                                    MagnetPullEnable = true;
                                    notificationSystem.AddNotification("Magnet Pull Enabled !", 2500);
                                }
                                else
                                {
                                    MagnetPullEnable = false;
                                    notificationSystem.AddNotification("Magnet Pull Disabled !", 2500);
                                }
                            }

                            if (edited::Checkbox("Show Fov", "In Game", &FovEnable))
                            {
                                if (FovEnable)
                                {
                                    notificationSystem.AddNotification("FOV Enabled !", 2500);
                                }
                                else
                                {
                                    notificationSystem.AddNotification("FOV Disabled !", 2500);
                                }
                            }

                            if (FovEnable) {
                                edited::SliderInt("FOV Radius", "Choose Your value", &fov, 1, 1000);
                            }
                           



                        }
                        edited::EndChild();
                        SameLine(0, 0);
                        ImGui::SetCursorPos(ImVec2(360, 80));
                        edited::BeginChild("c", "Internal Aims", ImVec2(310, 400), ImGuiChildFlags_Border, 0);
                        {


                            if (edited::Checkbox("No Recoil", "In Game", &norecoilpro))
                            {
                                if (norecoilpro)
                                {

                                    notificationSystem.AddNotification("No Recoil Enabled !", 2500);
                                }
                                else
                                {
                                    notificationSystem.AddNotification("No Recoil Desabled !", 2500);
                                }
                            }

                            if (edited::Checkbox("Fast Reload", "In Game", &FastReload))
                            {
                                if (FastReload)
                                {

                                    notificationSystem.AddNotification("Fast Reload Enabled !", 2500);
                                }
                                else
                                {
                                    notificationSystem.AddNotification("Fast Reload Desabled !", 2500);
                                }
                            }
                            edited::Checkbox("Head Tracking", "In Game", &headtraking);

                            edited::Checkbox("Forward Player", "In Game", &frwardPlayer);

                            if (AimbotAI) {
                                edited::Checkbox("AimLock", "In Game", &Aimlock);
                            }




                            if (edited::Checkbox("Speed - External", "In Game", &fastexter))
                            {
                                if (fastexter)
                                {
                                    bSpeedHackEnabled = true;

                                }
                                else
                                {
                                    bSpeedHackEnabled = false;
                                }

                            }

                            if (edited::Checkbox("Fastfire", "In Game", &InstantF))
                            {


                                if (InstantF) {
                                    notificationSystem.AddNotification("FastFire ENABLED!", 2500);
                                }
                                else {
                                    notificationSystem.AddNotification("FastFire DISABLED!", 2500);
                                }



                            }



                        }
                        edited::EndChild();
                    }
                    else if (active_tab == 1)
                    {
                        ImGui::SetCursorPos(ImVec2(30, 80));
                        edited::BeginChild("b", "Esp Menu", ImVec2(310, 400), ImGuiChildFlags_Border, 0);
                        {


                            const char* gameList[] = { "FF Max", "FF Max X86", "FF V7A", "FF Playstore" };

                            if (edited::Combo("Select FF", "FF Versions", &SelectedGame, gameList, IM_ARRAYSIZE(gameList)))
                            {
                                if (SelectedGame == 0)
                                    LoadFFMax();
                                else if (SelectedGame == 1)
                                    LoadFFmaxX86();
                                else if (SelectedGame == 2)
                                    LoadNormalFF();
                                else if (SelectedGame == 3)
                                    LoadFFPlaystore();
                            }

                            if (edited::Checkbox("Enable ADB", "Choose Your version first", &ActiveADB))
                            {
                                if (ActiveADB == 1)
                                {
                                    std::thread taskThread([]()
                                        {

                                            hdPlayerWindow = InjectADB();

                                        });
                                    taskThread.detach();
                                }
                            }

                            if (REIMAN)
                            {
                                if (edited::Checkbox("Esp Line", "In Game", &EspLineZ))
                                {
                                    if (EspLineZ)
                                    {

                                        notificationSystem.AddNotification("Esp Line Enabled !", 2500);
                                    }
                                    else
                                    {
                                        notificationSystem.AddNotification("Esp Line Desabled !", 2500);
                                    }
                                }

                                if (edited::Checkbox("Esp Box", "In Game", &ESPBox))
                                {
                                    if (ESPBox)
                                    {

                                        notificationSystem.AddNotification("Esp Box Enabled !", 2500);
                                    }
                                    else
                                    {
                                        notificationSystem.AddNotification("Esp Box Desabled !", 2500);
                                    }
                                }

                                if (edited::Checkbox("Esp Filled", "In Game", &ESPBoxFILLED))
                                {
                                    if (ESPBoxFILLED)
                                    {

                                        notificationSystem.AddNotification("Esp Box Filled Enabled !", 2500);
                                    }
                                    else
                                    {
                                        notificationSystem.AddNotification("Esp Box Filled Desabled !", 2500);
                                    }
                                }

                                if (edited::Checkbox("Esp Weapon", "In Game", &ESPWeapon))
                                {
                                    if (ESPWeapon)
                                    {

                                        notificationSystem.AddNotification("Esp Weapon Enabled !", 2500);
                                    }
                                    else
                                    {
                                        notificationSystem.AddNotification("Esp Weapon Desabled !", 2500);
                                    }
                                }


                                if (edited::Checkbox("Esp Name", "In Game", &ESPNameZ))
                                {
                                    if (ESPNameZ)
                                    {

                                        notificationSystem.AddNotification("Esp Name Enabled !", 2500);
                                    }
                                    else
                                    {
                                        notificationSystem.AddNotification("Esp Name Desabled !", 2500);
                                    }
                                }

                                if (edited::Checkbox("Esp Health", "In Game", &ESPHealth))
                                {
                                    if (ESPHealth)
                                    {

                                        notificationSystem.AddNotification("Esp Health Enabled !", 2500);
                                    }
                                    else
                                    {
                                        notificationSystem.AddNotification("Esp Health Desabled !", 2500);
                                    }
                                }

                                if (edited::Checkbox("Esp Distance", "In Game", &ESPDistance))
                                {
                                    if (ESPDistance)
                                    {

                                        notificationSystem.AddNotification("Esp Distance Enabled !", 2500);
                                    }
                                    else
                                    {
                                        notificationSystem.AddNotification("Esp Distance Desabled !", 2500);
                                    }
                                }

                                if (edited::Checkbox("Esp Skeleton", "In Game", &ESPBones))
                                {
                                    if (ESPBones)
                                    {

                                        notificationSystem.AddNotification("Esp Skeleton Enabled !", 2500);
                                    }
                                    else
                                    {
                                        notificationSystem.AddNotification("Esp Skeleton Desabled !", 2500);
                                    }
                                }

                                if (edited::Checkbox("Esp Radar", "In Game", &EspRadar))
                                {
                                    if (EspRadar)
                                    {

                                        notificationSystem.AddNotification("Esp Radar Enabled !", 2500);
                                    }
                                    else
                                    {
                                        notificationSystem.AddNotification("Esp Radar Desabled !", 2500);
                                    }
                                }

                                if (edited::Checkbox("Esp Rgb", "In Game", &rgbMode))
                                {
                                    if (rgbMode)
                                    {

                                        notificationSystem.AddNotification("Esp Rgb Enabled !", 2500);
                                    }
                                    else
                                    {
                                        notificationSystem.AddNotification("Esp Rgb Desabled !", 2500);
                                    }
                                }

                                if (edited::Checkbox("Esp Wukong", "In Game", &SHowVis))
                                {
                                    if (SHowVis)
                                    {

                                        notificationSystem.AddNotification("Esp Wukong Enabled !", 2500);
                                    }
                                    else
                                    {
                                        notificationSystem.AddNotification("Esp Wukong Desabled !", 2500);
                                    }
                                }

                                edited::Checkbox("Esp Team", "In Game", &SHowTeam);


                                if (edited::Checkbox("Auto Refresh", "Auto Refresh For ESP", &RefreshEsp)) {

                                    if (RefreshEsp)
                                    {
                                        std::thread([]()
                                            {
                                                std::thread(AutoRefreshLoop).detach();
                                            }).detach();
                                    }
                                }

                                if (RefreshEsp) {
                                    edited::SliderFloat("Refresh Delay", "Delay For Refresh", &cacheClearInterval, 1.0f, 250.0f, "%.0f");
                                }
                            }
                        }
                        edited::EndChild();

                        SameLine(0, 0);

                        ImGui::SetCursorPos(ImVec2(360, 80));
                        edited::BeginChild("f", "Esp Settings", ImVec2(310, 400), ImGuiChildFlags_Border, 0);
                        {

                            if (EspLineZ) {

                                const char* items[3]{ "Top", "Centre", "Bottom" };
                                edited::Combo("ESP Type", "", &lineType, items, IM_ARRAYSIZE(items), 3);

                                const char* items1[3]{ "Line", "Curved", "Stairs" };
                                edited::Combo("ESP Point", "", &lineType1, items1, IM_ARRAYSIZE(items1), 3);

                                edited::SliderFloat("Esp Thickness", "", &espLineThickness, 0.5f, 10.0f, "%.1f");

                                edited::SliderFloat("Esp Distance", "", &maxDistance, 150.f, 500.f, "%.0f");
                            }

                            if (ESPBox) {
                                const char* items2[3]{ "3D Box", "Full Box", "Shadow", };
                                edited::Combo("Box Type", "", &ESPBoxType, items2, IM_ARRAYSIZE(items2), 3);
                            }

                            if (ESPHealth) {
                                const char* items2[4]{ "Left", "Right", "Top", "Bottom" };
                                edited::Combo("Health Type", "", &selectedHealthBarPos, items2, IM_ARRAYSIZE(items2), 3);
                            }

                            if (ESPBones) {
                                edited::SliderFloat("Bones Thickness", "Choose Your value", &boneThickness, 0.5f, 4.0f, "%.1f");
                            }

                            if (ESPNameZ) {
                                ImGui::InputTextEx("##2", "Enter bot name...", defaultName, sizeof(defaultName), ImVec2(325, 50), ImGuiInputTextFlags_None);
                            }


                            if (REIMAN)
                            {

                                edited::ColorEdit4("Line Color", "", (float*)&lineColorEnemy, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
                                edited::ColorEdit4("Filled Color", "", (float*)&boxColorFilled, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
                                edited::ColorEdit4("Box Color", "", (float*)&boxColor, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
                                edited::ColorEdit4("Name Color", "", (float*)&nameColor, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
                                edited::ColorEdit4("Bones Color", "", (float*)&bonesColor, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);

                            }



                        }
                        edited::EndChild();
                    }
                    else if (active_tab == 2)
                    {
                        ImGui::SetCursorPos(ImVec2(30, 80));
                        edited::BeginChild("d", "Extra Functions", ImVec2(310, 400), ImGuiChildFlags_Border, 0);
                        {
                            edited::Keybind("Tp To Mark - Key", "Choose any key for ON/OFF", &tpkey);
                            edited::Keybind("Speed Teleport - Key", "Choose any key for ON/OFF", &spdkey);
                            edited::Keybind("Climb Up - Key", "Choose any key for ON/OFF", &climbkey);
                            edited::Keybind("Dive Kill - Key", "Choose any key for ON/OFF", &dvkey);
                            edited::Keybind("Down Player - Key", "Choose any key for ON/OFF", &dwkey);

                        }
                        edited::EndChild();

                        SameLine(0, 0);

                        ImGui::SetCursorPos(ImVec2(360, 80));
                        edited::BeginChild("o", "Keys", ImVec2(310, 400), ImGuiChildFlags_Border, 0);
                        {
                            edited::Keybind("Speed Internal - Key", "Choose any key for ON/OFF", &fastkey);


                        }
                        edited::EndChild();
                    }

                    else if (active_tab == 3)
                    {
                        ImGui::SetCursorPos(ImVec2(30, 80));
                        edited::BeginChild("j", "Misc feautures", ImVec2(310, 400), ImGuiChildFlags_Border, 0);
                        {


                            if (TeleportKey) {
                                edited::SliderFloat("Distance", "", &teleportbasedistance, 0.1f, 200.0f, "%.1f");
                            }

                            edited::Keybind("Teleport - Key", "Choose Your key", &TeleportKey);

                            if (TeleKillKey) {
                                edited::SliderFloat("Distance Tele", "", &enemyplayerdistance, 0.1f, 10.0f);

                            }

                            edited::Keybind("Tele Player Key!", "Choose Your key", &TeleKillKey);

                            if (UpPlayerKey) {
                                edited::SliderFloat("Distance Up", "", &UpPlayerAlture, 0.1f, 10.0f);

                            }

                            edited::Keybind("Up Player Key!", "Choose Your key", &UpPlayerKey);



                            edited::Keybind("Speed Internal", "Enable Every Game", &SpeedKey);
                            edited::Keybind("TpBaseCheck", "", &tpkey);
                            edited::Keybind("TpEnemyCheck", "", &enkey);
                            edited::Keybind("FlyWall", "", &flykey1);

                            //edited::Keybind("Wallhack FLY", "", &WallhackFlyKey);
                        }
                        edited::EndChild();

                        SameLine(0, 0);

                        ImGui::SetCursorPos(ImVec2(360, 80));
                        edited::BeginChild("l", "External feautures", ImVec2(310, 400), ImGuiChildFlags_Border, 0);
                        {




                            edited::Keybind("Freeze Lag", "", &FreezeLagKey);

                            edited::Keybind("Ghost Hack", "", &GhostHackKey);
                            if (edited::Checkbox("NoGravityFly", "In Game", &NoGravityFlyEnabled))
                            {
                                if (NoGravityFlyEnabled) { NoGravityFly::Start(); notificationSystem.AddNotification("NoGravityFly Enabled !", 2500); }
                                else { NoGravityFly::Stop(); notificationSystem.AddNotification("NoGravityFly Disabled !", 2500); }
                            }
                            // new
                            //edited::Keybind("Aim Lag", "", &GhostHackKey);


                        }
                        edited::EndChild();




                    }
                    else if (active_tab == 4)
                    {
                        ImGui::SetCursorPos(ImVec2(30, 80));
                        edited::BeginChild("a", "Settings", ImVec2(310, 400), ImGuiChildFlags_Border, 0);
                        {




                            if (edited::Checkbox("Mode Stream", "", &StreamerMode))
                            {
                                if (StreamerMode)
                                {
                                    //ImGui::Notification({ ImGuiToastType_Success, 4000, "Modo Stream - Activado!" });

                                    SetWindowDisplayAffinity(GetActiveWindow(), WDA_EXCLUDEFROMCAPTURE);
                                    ITaskbarList* pTaskList = NULL;
                                    HRESULT initRet = CoInitialize(NULL);
                                    HRESULT createRet = CoCreateInstance(CLSID_TaskbarList,
                                        NULL,
                                        CLSCTX_INPROC_SERVER,
                                        IID_ITaskbarList,
                                        (LPVOID*)&pTaskList);

                                    if (createRet == S_OK)
                                    {

                                        pTaskList->DeleteTab(GetActiveWindow());

                                        pTaskList->Release();
                                    }

                                    CoUninitialize();
                                }
                                else {
                                    //ImGui::Notification({ ImGuiToastType_Success, 4000, "Modo Stream - Desactivado!" });

                                    SetWindowDisplayAffinity(GetActiveWindow(), WDA_NONE);
                                    ITaskbarList* pTaskList = NULL;
                                    HRESULT initRet = CoInitialize(NULL);
                                    HRESULT createRet = CoCreateInstance(CLSID_TaskbarList,
                                        NULL,
                                        CLSCTX_INPROC_SERVER,
                                        IID_ITaskbarList,
                                        (LPVOID*)&pTaskList);

                                    if (createRet == S_OK)
                                    {
                                        pTaskList->AddTab(GetActiveWindow());

                                        pTaskList->Release();
                                    }

                                    CoUninitialize();
                                }
                            }


                            edited::Keybind("Hide Menu", "Hide Menu Key", &HideMenu);
                            static int lineType = 0;
                            const char* items[] = { "High", "Meduim", "Low" };
                            ImGui::Combo("Proccess Priority", &lineType, [](void* data, int idx, const char** out_text) {
                                const char* const* items = (const char* const*)data;
                                if (out_text) *out_text = items[idx];
                                return true;
                                }, items, IM_ARRAYSIZE(items));
                        }
                        edited::EndChild();

                        SameLine(0, 0);

                        ImGui::SetCursorPos(ImVec2(360, 80));
                        edited::BeginChild("d", "Others", ImVec2(310, 400), ImGuiChildFlags_Border, 0);
                        {



                        }
                        edited::EndChild();


                    }


                    else if (active_tab == 6)
                    {
                        Main_Window = false;
                        Login_Window = true;
                    }

                }ImGui::End();

            }






        }
        Render();

        const float clear_color_with_alpha[4] = { 0.f };

        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return;
}

bool CreateDeviceD3D(HWND hWnd)
{

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

HANDLE hCurrentUIThread = nullptr;
HMODULE hCurrentModule = nullptr;

#ifdef _WINDLL


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved, HMODULE hMod, DWORD reason)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hinstDLL);
        hCurrentModule = hinstDLL;
        hCurrentUIThread = CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)REIMAN_PRIME, nullptr, NULL, nullptr);


    }

    if (fdwReason == DLL_PROCESS_DETACH)
        TerminateThread(hCurrentUIThread, 0);

    return TRUE;
}

#else


int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {


    REIMAN_PRIME();
    return 0;
}

#endif



