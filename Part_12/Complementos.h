ImVec4 colorTop(255, 0, 0, 255);
#include <atomic>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <random>
ImVec4 colorBottom(0, 0, 0, 255);
ImVec4 boxColor(1.0f, 1.0f, 1.0f, 1.0f);
ImVec4 lineColorTeam(0.0f, 1.0f, 0.0f, 1.0f);
ImVec4 lineColorEnemy(1.0f, 1.0f, 1.0f, 1.0f);
ImVec4 healthborderColor(0.0f, 0.0f, 0.0f, 1.0f);
ImVec4 healthtextColor(1.0f, 1.0f, 1.0f, 1.0f);
ImVec4 nameColor(1.0f, 1.0f, 1.0f, 1.0f);
ImVec4 bonesColor(1.0f, 1.0f, 1.0f, 1.0f);
ImVec4 fovColor(1.0f, 1.0f, 1.0f, 1.0f);
ImVec4 fovColor2(255, 0, 0, 255);
ImVec4 boxColorFilled = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
ImVec4 healthcolor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
static ImVec4 linecolor(1, 0.65f, 0, 1);
static ImVec4 glowcolorfirst(255, 0, 0, 255);

int AimBotDis = 1000;
float smoothFactor = 0.1f;
int fov = 200;
int fov2 = 100;
int aimdista = 5000;

enum class AimKey { RightMouseButton, LeftMouseButton, MiddleMouseButton, ShiftKey, ControlKey, AltKey };
AimKey selectedKey = AimKey::LeftMouseButton;


bool PullCheck = false;
const int fireCooldownMs360 = 100; // Ajusta este valor seg�n necesites
float PullDis = 300.0f; // Distancia m�xima para pull
float PullFov = 900.0f; // FOV para el pull
float pullTime = 0.0f;
float suavidad360 = 2.0f; // Factor de suavidad

bool vd = false;
bool pullcheck = false;
bool AimbotIAV2 = false;

void* vmPtr = nullptr;

typedef int(__cdecl* PGMPhysReadFunc)(void*, uintptr_t, void*, size_t);
typedef int(__cdecl* PGMPhysSimpleWriteGCPhysFunc)(void*, uintptr_t, void*, size_t);
typedef int(__cdecl* PGMPhysGCPtr2GCPhysFunc)(void*, uintptr_t, uintptr_t*);
typedef void* (__cdecl* VMMGetCpuByIdFunc)(void*, int);

PGMPhysReadFunc ogPhysRead = nullptr;
VMMGetCpuByIdFunc ogCPU = nullptr;
PGMPhysGCPtr2GCPhysFunc ogCast = nullptr;
PGMPhysSimpleWriteGCPhysFunc ogWrite = nullptr;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* buffer) {
    buffer->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int __cdecl HookedPGMPhysRead(void* pVM, uintptr_t GCPhys, void* pvBuf, size_t cbRead) {
    if (!vmPtr) vmPtr = pVM;
    return ogPhysRead(pVM, GCPhys, pvBuf, cbRead);
}

int zHookWrite(void* pVM, uintptr_t GCPhys, void* pvBuf, size_t cbRead) {
    return ogWrite(pVM, GCPhys, pvBuf, cbRead);
}

int zHookRead(void* pVM, uintptr_t GCPhys, void* pvBuf, size_t cbRead) {
    return ogPhysRead(pVM, GCPhys, pvBuf, cbRead);
}

void* CPU(void* pVM, int cpuId) {
    return ogCPU(pVM, cpuId);
}

int Cast(void* pVCpu, uintptr_t address, uintptr_t* physAddress) {
    return ogCast(pVCpu, address, physAddress);
}

void* pVMAddr = nullptr;
void* cpuAddr = nullptr;
std::unordered_map<uintptr_t, uintptr_t> CacheZ;
void* pVM = nullptr;

void InitializeZ(void* pVM) {
    pVMAddr = pVM;
    cpuAddr = CPU(pVM, 0);
    CacheZ.clear();
}

static int rgbvelocidad = 10;
static int linexd = 2;

bool ConvertZ(uintptr_t address, uintptr_t& phys) {
    auto it = CacheZ.find(address);
    if (it != CacheZ.end()) {
        phys = it->second;
        return true;
    }

    if (!cpuAddr && !(cpuAddr = CPU(pVMAddr, 0))) return false;

    int status = Cast(cpuAddr, address, &phys);
    if (status != 0) return false;

    CacheZ[address] = phys;
    return true;
}

void DrawSmoothCircle(ImVec2 center, float radius = 120.0f, float thickness = 3.0f)
{
    ImDrawList* vList = ImGui::GetForegroundDrawList();

    int segments = 64;
    float step = 1.0f / segments;

    ImU32 color = IM_COL32(255, 255, 255, 255); // BEYAZ

    for (int i = 0; i < segments; ++i)
    {
        float t1 = i * step;
        float t2 = (i + 1) * step;

        float angle1 = t1 * 2.0f * IM_PI;
        float angle2 = t2 * 2.0f * IM_PI;

        ImVec2 p1(center.x + cos(angle1) * radius, center.y + sin(angle1) * radius);
        ImVec2 p2(center.x + cos(angle2) * radius, center.y + sin(angle2) * radius);

        vList->AddLine(p1, p2, color, thickness);
    }
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

    // glow
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

    // filled
    DrawList->AddRectFilled(
        ImVec2(x, y),
        ImVec2(x + FilledBarWidth, y + BarHeight),
        BarColorU32
    );

    // ============================
    //       HP TEXT (EKLEND�)
    // ============================
    char healthText[12];
    snprintf(healthText, sizeof(healthText), "%d HP", CurrentHealth);

    ImVec2 textSize = ImGui::CalcTextSize(healthText);

    // Alt bardaki ile AYNI pozisyon tarz� ? bar�n hemen �st�
    ImVec2 textPos(
        x + (w / 2.0f) - (textSize.x / 2.0f),
        y - textSize.y - 2.0f
    );

    ImU32 textColor = IM_COL32(255, 255, 255, 230);
    if (HealthPercentage <= 0.25f)
        textColor = IM_COL32(255, 60, 60, 230);
    else if (HealthPercentage <= 0.5f)
        textColor = IM_COL32(255, 215, 0, 230);

    DrawList->AddText(textPos, textColor, healthText);
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

template<typename T>
bool ReadZ(uintptr_t address, T& data) {
    uintptr_t physAddress;
    return ConvertZ(address, physAddress) && zHookRead(pVMAddr, physAddress, &data, sizeof(T)) == 0;
}

bool GetBluestacksMiddlePos(POINT& middlePos) {
    const char* windowTitles[] = {
        "BlueStacks App Player", "MSI App Player", "BlueStacks",
        "App Player", "Ultra God", "e4vX Bs4", "@elsrt_alex", nullptr
    };

    HWND HwndEmul = nullptr;
    for (int i = 0; windowTitles[i]; i++) {
        HwndEmul = FindWindow(NULL, windowTitles[i]);
        if (HwndEmul) break;
    }

    if (!HwndEmul) return false;

    RECT clientRect;
    if (!GetClientRect(HwndEmul, &clientRect)) return false;

    middlePos.x = (clientRect.right - clientRect.left) / 2;
    middlePos.y = (clientRect.bottom - clientRect.top) / 2;
    ClientToScreen(HwndEmul, &middlePos);
    return true;
}

bool GetBluestacksTopCenterPos(POINT& topCenterPos) {
    const char* windowTitles[] = {
        "BlueStacks App Player", "MSI App Player", "BlueStacks",
        "App Player", "Ultra God", "e4vX Bs4", "@Pasrrot Security", nullptr
    };

    HWND HwndEmul = nullptr;
    for (int i = 0; windowTitles[i]; i++) {
        HwndEmul = FindWindow(NULL, windowTitles[i]);
        if (HwndEmul) break;
    }

    if (!HwndEmul) return false;

    RECT clientRect;
    if (!GetClientRect(HwndEmul, &clientRect)) return false;

    topCenterPos.x = (clientRect.right - clientRect.left) / 2;
    topCenterPos.y = 0;
    ClientToScreen(HwndEmul, &topCenterPos);
    return true;
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

void DrawGlowCorneredBox(float x, float y, float w, float h, ImU32 boxColorx, float thickness, float glowRadius, float feather, bool fillEnabled = false) {
    auto drawList = ImGui::GetForegroundDrawList();

    float newWidth = w;
    float newHeight = h * 1.15f;
    float newY = y - (newHeight - h);

    // Dolgu �izimi opsiyonel, fillEnabled ile kontrol edilir
    if (fillEnabled) {
        drawList->AddRectFilled(
            ImVec2(x, newY),
            ImVec2(x + newWidth, newY + newHeight),
            boxColorx
        );
    }

    // Glow efektli k��eler
    for (int i = (int)glowRadius; i > 0; --i) {
        float alphaFactor = 25.0f * (1.0f - ((float)i / glowRadius));
        ImU32 glowColor = IM_COL32(
            (int)(ImGui::GetColorU32(ImVec4(
                ((boxColorx >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f,
                ((boxColorx >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f,
                ((boxColorx >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f,
                alphaFactor / 255.0f
            )) & 0xFF),
            (int)(alphaFactor), 0, 0); // Bu sat�r basitle�tirilmi�, istersen daha do�ru alpha hesaplanabilir

        // K��e �izimleri
        drawList->AddLine(ImVec2(x - i, newY - i), ImVec2(x + newWidth / 6 + i, newY - i), boxColorx, thickness);
        drawList->AddLine(ImVec2(x - i, newY - i), ImVec2(x - i, newY + newHeight / 6 + i), boxColorx, thickness);

        drawList->AddLine(ImVec2(x + newWidth + i, newY - i), ImVec2(x + newWidth - newWidth / 6 - i, newY - i), boxColorx, thickness);
        drawList->AddLine(ImVec2(x + newWidth + i, newY - i), ImVec2(x + newWidth + i, newY + newHeight / 6 + i), boxColorx, thickness);

        drawList->AddLine(ImVec2(x - i, newY + newHeight + i), ImVec2(x + newWidth / 3 + i, newY + newHeight + i), boxColorx, thickness);
        drawList->AddLine(ImVec2(x - i, newY + newHeight + i), ImVec2(x - i, newY + newHeight - newHeight / 6 - i), boxColorx, thickness);

        drawList->AddLine(ImVec2(x + newWidth + i, newY + newHeight + i), ImVec2(x + newWidth - newWidth / 6 - i, newY + newHeight + i), boxColorx, thickness);
        drawList->AddLine(ImVec2(x + newWidth + i, newY + newHeight + i), ImVec2(x + newWidth + i, newY + newHeight - newHeight / 6 - i), boxColorx, thickness);
    }

    // Normal k��e �izgileri
    drawList->AddLine(ImVec2(x, newY), ImVec2(x + newWidth / 6, newY), boxColorx, thickness);
    drawList->AddLine(ImVec2(x, newY), ImVec2(x, newY + newHeight / 6), boxColorx, thickness);

    drawList->AddLine(ImVec2(x + newWidth, newY), ImVec2(x + newWidth - newWidth / 6, newY), boxColorx, thickness);
    drawList->AddLine(ImVec2(x + newWidth, newY), ImVec2(x + newWidth, newY + newHeight / 6), boxColorx, thickness);

    drawList->AddLine(ImVec2(x, newY + newHeight), ImVec2(x + newWidth / 6, newY + newHeight), boxColorx, thickness);
    drawList->AddLine(ImVec2(x, newY + newHeight), ImVec2(x, newY + newHeight - newHeight / 6), boxColorx, thickness);

    drawList->AddLine(ImVec2(x + newWidth, newY + newHeight), ImVec2(x + newWidth - newWidth / 6, newY + newHeight), boxColorx, thickness);
    drawList->AddLine(ImVec2(x + newWidth, newY + newHeight), ImVec2(x + newWidth, newY + newHeight - newHeight / 6), boxColorx, thickness);
}


void DrawFullBox(float x, float y, float w, float h, ImU32 boxColorx, float thickness) {
    auto drawList = ImGui::GetForegroundDrawList();

    // Box'u biraz k���lt ve yukar� kayd�r (orijinal mant�k)
    float newWidth = w * 0.95f;
    float newHeight = h * 1.15f;
    float newX = x + (w - newWidth) / 2.0f;
    float newY = y - (newHeight - h);

    // Sadece border �izimi
    drawList->AddRect(
        ImVec2(newX, newY),
        ImVec2(newX + newWidth, newY + newHeight),
        boxColorx,
        0.0f,
        ImDrawFlags_None,
        thickness * 0.5f
    );
}




template<typename T>
bool ReadArrayZ(uintptr_t address, std::vector<T>& array) {
    uintptr_t physAddress;
    if (!ConvertZ(address, physAddress)) return false;

    size_t size = sizeof(T) * array.size();
    return zHookRead(pVMAddr, physAddress, array.data(), size) == 0;
}

std::string ReadStringZ(uintptr_t address, int size, bool unicode = false) {
    if (size <= 0) return "";

    std::vector<uint8_t> stringBytes(size);
    if (!ReadArrayZ(address, stringBytes)) return "";

    std::string readString;
    if (unicode) {
        readString.assign(stringBytes.begin(), stringBytes.end());
    }
    else {
        readString = std::string(stringBytes.begin(), stringBytes.end());
    }

    size_t nullPos = readString.find('\0');
    if (nullPos != std::string::npos) readString = readString.substr(0, nullPos);
    return readString;
}

int entitiesCount = 0;
std::atomic<bool> runningxd2(true);

void ProcessEntities() {
    while (runningxd2) {
        CacheZ.clear();
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

template<typename T>
bool ReadArrayZ2(uintptr_t address, std::vector<T>& array) {
    uintptr_t convertedAddress;
    if (!ConvertZ(address, convertedAddress)) return false;

    size_t size = sizeof(T) * array.size();
    return zHookRead(pVMAddr, convertedAddress, array.data(), size) == 0;
}

std::string ReadStringZ2(uintptr_t address, int size, bool unicode = true) {
    std::vector<uint8_t> stringBytes(size);
    if (!ReadArrayZ2(address, stringBytes)) return "";

    std::string readString;
    if (unicode) {
        std::wstring wideString(reinterpret_cast<wchar_t*>(stringBytes.data()), size / 2);
        readString = std::string(wideString.begin(), wideString.end());
    }
    else {
        readString = std::string(stringBytes.begin(), stringBytes.end());
    }

    auto nullTerminator = readString.find('\0');
    if (nullTerminator != std::string::npos) readString = readString.substr(0, nullTerminator);
    return readString;
}

template<typename T>
void WriteZ(uintptr_t address, const T& value) {
    uintptr_t physAddress;
    if (!ConvertZ(address, physAddress)) return;
    zHookWrite(pVMAddr, physAddress, (void*)&value, sizeof(T));
}

struct TMatrix {
    Vector4 position;
    Quaternion rotation;
    Vector4 scale;
};

struct HealthData {
    float vidaPorcentaje;
    std::string vidaStr;
};

struct BoxData {
    ImVec2 topLeft;
    ImVec2 bottomRight;
};

static bool GetPosition(uint32_t transform, Vector3& pos) {
    pos = Vector3::Zero();

    uint32_t transformObjValue;
    if (!ReadZ(transform + 0x8, transformObjValue))
        return false;

    uint32_t indexValue;
    if (!ReadZ(transformObjValue + 0x24, indexValue))
        return false;

    uint32_t matrixValue;
    if (!ReadZ(transformObjValue + 0x20, matrixValue))
        return false;

    uint32_t matrixListValue;
    if (!ReadZ(matrixValue + 0x18, matrixListValue))
        return false;

    uint32_t matrixIndicesValue;
    if (!ReadZ(matrixValue + 0x1C, matrixIndicesValue))
        return false;

    Vector3 resultValue;
    if (!ReadZ((uint32_t)(indexValue * 0x30 + matrixListValue), resultValue))
        return false;

    const int maxTries = 50;
    int tries = 0;

    int transformIndexValue;
    if (!ReadZ((uint32_t)((indexValue * 0x4) + matrixIndicesValue), transformIndexValue))
        return false;

    while (transformIndexValue >= 0) {
        tries++;
        if (tries == maxTries) break;

        TMatrix tMatrixValue;
        if (!ReadZ((uint32_t)(0x30 * transformIndexValue + matrixListValue), tMatrixValue))
            return false;

        float rotX = tMatrixValue.rotation.X;
        float rotY = tMatrixValue.rotation.Y;
        float rotZ = tMatrixValue.rotation.Z;
        float rotW = tMatrixValue.rotation.W;

        float scaleX = resultValue.X * tMatrixValue.scale.x;
        float scaleY = resultValue.Y * tMatrixValue.scale.y;
        float scaleZ = resultValue.Z * tMatrixValue.scale.z;

        resultValue.X = tMatrixValue.position.x + scaleX +
            (scaleX * ((rotY * rotY * -2.0f) - (rotZ * rotZ * 2.0f))) +
            (scaleY * ((rotW * rotZ * -2.0f) - (rotY * rotX * -2.0f))) +
            (scaleZ * ((rotZ * rotX * 2.0f) - (rotW * rotY * -2.0f)));

        resultValue.Y = tMatrixValue.position.y + scaleY +
            (scaleX * ((rotX * rotY * 2.0f) - (rotW * rotZ * -2.0f))) +
            (scaleY * ((rotZ * rotZ * -2.0f) - (rotX * rotX * 2.0f))) +
            (scaleZ * ((rotW * rotX * -2.0f) - (rotZ * rotY * -2.0f)));

        resultValue.Z = tMatrixValue.position.z + scaleZ +
            (scaleX * ((rotW * rotY * -2.0f) - (rotX * rotZ * -2.0f))) +
            (scaleY * ((rotY * rotZ * 2.0f) - (rotW * rotX * -2.0f))) +
            (scaleZ * ((rotX * rotX * -2.0f) - (rotY * rotY * 2.0f)));

        if (!ReadZ((uint32_t)(transformIndexValue * 0x4 + matrixIndicesValue), transformIndexValue))
            return false;
    }

    pos = resultValue;
    return tries != maxTries;
}

static bool GetNodePosition(uint32_t nodeTransform, Vector3& result) {
    uint32_t transformValue;
    if (!ReadZ(nodeTransform + 0x8, transformValue)) {
        result = Vector3::Zero();
        return false;
    }
    return GetPosition(transformValue, result);
}

static void SetNodePosition(uint32_t baseAddress, uint32_t boneOffset, Vector3 newPos) {
    uint32_t boneTransform;
    if (!ReadZ(baseAddress + boneOffset, boneTransform)) return;

    uint32_t transformValue;
    if (!ReadZ(boneTransform + 0x8, transformValue)) return;

    uint32_t transformObjPtr;
    if (!ReadZ(transformValue + 0x8, transformObjPtr)) return;

    uint32_t matrixValue;
    if (!ReadZ(transformObjPtr + 0x20, matrixValue)) return;

    WriteZ<Vector3>(matrixValue + 0x80, newPos);
}
int refreshRateMs = 10000;

std::string uInt_StringsZ(uint32_t value) {
    std::stringstream stream;
    stream << "0x" << std::setfill('0') << std::setw(8) << std::hex << value;
    return stream.str();
}

BOOL CALLBACK GetPosEmlBS(HWND hWnd, LPARAM lParam) {
    char title[256];
    GetWindowTextA(hWnd, title, sizeof(title));
    std::string windowName(title);

    if (windowName == "HD-Player" || windowName == "_ctl.Window") {
        *reinterpret_cast<HWND*>(lParam) = hWnd;
        return FALSE;
    }
    return TRUE;
}

HWND FindHDPlayerWindow(HWND parent) {
    HWND renderWindow = nullptr;
    EnumChildWindows(parent, GetPosEmlBS, reinterpret_cast<LPARAM>(&renderWindow));
    return renderWindow;
}

ID3D11Device* device = nullptr;
ID3D11DeviceContext* device_constext = nullptr;
IDXGISwapChain* swap_chain = nullptr;
ID3D11RenderTargetView* render_target_view = nullptr;
D3D_FEATURE_LEVEL level;

int numLines = 150;
float amplitude = 56.0f;
float frequency9 = 0.031f;
float phaseOffset = 0.4f;
float lineSpacing = 20.0f;
float xOffset = 1000.0f;
const ImU32 lineColor = IM_COL32(255, 255, 0, 10);

std::string ErrorLoadAdb = "NUll";



const char* zGetWName(uint32_t ID)
{
    switch (ID)
    {
    case 0:   return "AK47";
    case 1:   return "Punos";
    case 2:   return "M4A1";
    case 3:   return "USP";
    case 4: return "AWM";
    case 5: return "M1014";
    case 6:   return "AK47";
    case 7:   return "UMP";
    case 8:   return "MP5";
    case 9:   return "DESERT EAGLE";
    case 10:   return "G18";
    case 11:   return "M14";
    case 12:   return "SCAR";
    case 13:   return "VSS";
    case 14:   return "GROZA";
    case 15:   return "MP40";
    case 16:   return "SARTEN";
    case 17:   return "MACHETE";
    case 18:   return "SKS";
    case 19:   return "M249";
    case 20:   return "M1873";
    case 21:   return "KAR98K";
    case 24:   return "FAMAS";
    case 25:   return "M500";
    case 26:   return "SVD";
    case 27:   return "BATE";
    case 28:   return "XM8";
    case 29:   return "SPAS12";
    case 30:   return "M60";
    case 32:   return "P90";
    case 33:   return "AN94";
    case 34:   return "KATANA";
    case 35:   return "CG15";
    case 39:   return "PLASMA";
    case 41:   return "M1887";
    case 43:   return "THOMPSON";
    case 45:   return "M828";
    case 46:   return "AUG";
    case 47:   return "PARAFAL";
    case 48:   return "WOODPECKER";
    case 49:   return "VECTOR";
    case 50:   return "MAG-7";
    case 51:   return "HOZ";
    case 54:   return "KORD";
    case 55:   return "M1917";
    case 56:   return "USP-2";
    case 57:   return "KINGFISHER";
    case 58:   return "MINI UZI";
    case 60: return "MP5 1";
    case 62: return "VSS 1";
    case 65: return "AWM-Y";
    case 67:   return "FAMAS 1";
    case 70:   return "GROZA-X";
    case 71:   return "M249-X";
    case 72:   return "SVD-Y";
    case 73:   return "G36";
    case 75:   return "M24";
    case 78:   return "RIFLE CURATIVO";
    case 80:   return "M4A1 1";
    case 81:   return "M4A1 2";
    case 82:   return "M4A1 3";
    case 86:   return "CHARGE BUSTER";
    case 88:   return "MAC10";
    case 89:   return "AC80";
    case 93:   return "PISTOLA CURATIVA";
    case 99:   return "ARMA ESCUDO";
    case 100:   return "LANZALLAMAS";
    case 119:   return "M1887-X";
    case 120: return "MP5 2";
    case 121: return "MP5 3";
    case 124: return "VSS 2";
    case 125: return "VSS 3";
    case 130:   return "FAMAS 2";
    case 131:   return "FAMAS 3";
    case 150:   return "BIZON";
    case 178:   return "SCAR 1";
    case 179:   return "SCAR 2";
    case 180:   return "SCAR 3";
    case 181:   return "TROGON";
    case 182:   return "TROGON - GRANADA";
    case 184:   return "M1014 1";
    case 185:   return "M1014 2";
    case 186:   return "M1014 3";
    case 193:   return "AUG 1";
    case 194:   return "AUG 2";
    case 195:   return "AUG 3";
    case 197:   return "VSK94";
    case 228:   return "MAC10 1";
    case 229:   return "MAC10 2";
    case 230:   return "MAC10 3";
    case 602:   return "GRANADA CEGADORA";
    case 608:   return "GRANADA HIELO";
    case 1201:   return "PARED GLOO";
    case 21001:   return "PISTOLA CURATIVA-Y";
    case 21002:   return "M590";
    default:  return "Desconocido";
    }
}
const char* zGetWIcon(uint32_t ID) {
    switch (ID) {
    case 0:   return "K";
    case 1: return "h";
    case 2: return "b";
    case 3: return "+";
    case 4: return "L";
    case 5: return "j";
    case 6: return "K";
    case 7: return "3";
    case 8:   return "f";
    case 9:   return "9";
    case 10:   return "A";
    case 11:   return "&";
    case 12:   return "-";
    case 13:   return "5";
    case 14:   return "P";
    case 15:   return "W";
    case 16:   return ".";
    case 17:   return ":";
    case 18:   return "U";
    case 19:   return "M";
    case 20:   return "=";
    case 21:   return "i";
    case 24:   return "E";
    case 25:   return "B";
    case 26:   return "[";
    case 27:   return "F";
    case 28:   return "`";
    case 29:   return ")";
    case 30:   return "'";
    case 32:   return "4";
    case 33:   return "/";
    case 34:   return ">";
    case 35:   return "]";
    case 39:   return "R";
    case 41:   return "S";
    case 43:   return "<";
    case 45:   return "#";
    case 46:   return "e";
    case 47:   return "I";
    case 48:   return "C";
    case 49:   return "N";
    case 50:   return "Q";
    case 51:   return "V";
    case 54:   return ";";
    case 55:   return "c";
    case 56:   return "+";
    case 57:   return ",";
    case 58:   return "O";
    case 60: return "f";
    case 62: return "5";
    case 65: return "L";
    case 67:   return "E";
    case 70:   return "P";
    case 72:   return "[";
    case 71:   return "M";
    case 73:   return "D";
    case 75:   return "8";
    case 78:   return "(";
    case 80:   return "b";
    case 81:   return "b";
    case 82:   return "b";
    case 86:   return "T";
    case 88:   return "%";
    case 89:   return "G";
    case 93:   return "_";
    case 99:   return "$";
    case 100:   return "Y";
    case 119:   return "S";
    case 120: return "f";
    case 121: return "f";
    case 124: return "5";
    case 125: return "5";
    case 130:   return "E";
    case 131:   return "E";
    case 150:   return "a";
    case 178:   return "-";
    case 179:   return "-";
    case 180:   return "-";
    case 181:   return "*";
    case 182:   return "*";
    case 184:   return "j";
    case 185:   return "j";
    case 186:   return "j";
    case 193:   return "e";
    case 194:   return "e";
    case 195:   return "e";
    case 197:   return "Z";
    case 228:   return "%";
    case 229:   return "%";
    case 230:   return "%";
    case 602:   return "1";
    case 608:   return "1";
    case 1201:   return "g";
    case 21001:   return "_";
    case 21002:   return "?";
    default: return "m";
    }
}


uint32_t zGetInfoW(uint32_t player, bool zWOnScan)
{
    uint32_t zWdata, zWdata1, zWdata2, zWID;
    if (zWOnScan)
    {
        if (!ReadZ(player + 0x44, zWdata)) { return 0; }
        if (!ReadZ(zWdata + 0x8, zWdata1)) { return 0; }
        if (!ReadZ(zWdata1 + 0x20, zWdata2)) { return 0; }
        if (ReadZ(zWdata2 + 0x10, zWID))
        {
            return zWID;
        }
    }
    return 0;
}

float GetDistancePull360(const ImVec2& point, const ImVec2& center)
{
    float dx = point.x - center.x;
    float dy = point.y - center.y;
    return sqrtf(dx * dx + dy * dy); // �ki nokta aras�ndaki 2D mesafe
}


static std::unordered_map<uint32_t, Vector3> originalPositions;
static uint32_t currentPullTarget = 0;
static auto lastFireTime = std::chrono::steady_clock::now();
int TpDistance = 180.0f;
bool TpBaseCheck = false;
bool TpEnemyCheck = false;

int EnemyDistance = 0.0f;
int EnemyDistanceX = 8;
int EnemySide = 0.0f;

static float PullHeightOffset = 0.0f;
static float screenPullRange = 140.0f;
static float minDistanceToSkip = 1.0f;

bool enemypullv2 = false;
bool FlyWall = false;
int tpkey = 0;
int enkey = 0;
int flykey1 = 0;
enum ModeType {
    PullPlayerTargetType = 0,
};


// Aim Features
bool AimbotRage = false;
bool AimbotAI = false;
bool FovEnable = false;
bool AimbotVisible = false;
bool headtraking = false;
bool Aimlock = false;
bool AimbotLegit = false;
// Brutal Silent - replaces Rage method (renamed to Silent)
bool BrutalSilent = false;
inline bool ExternalAimbotEnabled = false;
inline bool NoGravityFlyEnabled = false;
static uint32_t BrutalCachedWeaponId = 0;
static Vector3 Brutal_lastTargetPos = Vector3::Zero();
static Vector3 Brutal_targetVelocity = Vector3::Zero();
static uint64_t Brutal_lastUpdate = 0;
static Vector3 Brutal_lastWritten = Vector3::Zero();
static bool Brutal_hasLastWrite = false;
static bool Brutal_prevFiring = false;
inline void BrutalReset() { BrutalCachedWeaponId=0; Brutal_lastTargetPos=Vector3::Zero(); Brutal_targetVelocity=Vector3::Zero(); Brutal_lastUpdate=GetTickCount64(); Brutal_hasLastWrite=false; }
inline Vector3 BrutalCalculateLead(const Vector3& cur) { uint64_t now=GetTickCount64(); float dt=(Brutal_lastUpdate>0)?(float)(now-Brutal_lastUpdate)/1000.f:0.f; if(dt>0 && !(Brutal_lastTargetPos==Vector3::Zero())) Brutal_targetVelocity=(cur - Brutal_lastTargetPos)/dt; Brutal_lastTargetPos=cur; Brutal_lastUpdate=now; constexpr float leadFactor=0.4f; float sm=1.f; float sp=Vector3::Magnitude(Brutal_targetVelocity); if(sp>5) sm=1.2f; else if(sp>2) sm=1.1f; Vector3 lead=Brutal_targetVelocity*leadFactor*sm*0.06f; if(Vector3::Magnitude(lead)>1.f) lead=Vector3::Normalized(lead)*1.f; return lead; }
inline Vector3 BrutalCalculateAim(const Vector3& head, const Vector3& camPos) { Vector3 tgt=head+BrutalCalculateLead(head)*0.25f; float d=Vector3::Distance(camPos,head); if(d<30) tgt+=Vector3(0,0.02f,0); else if(d<80) tgt+=Vector3(0,0.01f,0); return tgt; }

bool aimbotVisibleActive = false;

static uint32_t lastPatchedAddr = 0;
static uint32_t lastOriginalValue = 0;
static bool isPatched = false;

static int selectedBone = 0;

static bool SpeedEnable;
static bool WallhackFlyEnable;

// Features
bool TeleportBase = false;
bool aimsilent = false;
bool frwardPlayer = false;
bool UpPlayer = false;
bool TeleKill = false;
bool norecoilpro = false;
bool FastReload = false;
bool PullPlayerEnable = false;

float recoiloff = 0.0f;

float enemyplayerdistance = 10.0f;
float teleportbasedistance = 70.0f;

float UpPlayerAlture = 0.25f;


// Esp Features
bool EspLineZ = false;
bool ESPBoxFILLED = false;
bool rgbMode = false;
bool SHowTeam = false;
bool SHowVis = false;
bool ESPNameZ = false;
bool EspRadar = false;
bool ESPBox = false;
bool ESPWeapon = false;
bool ESPDistance = false;
bool ESPHealth = false;
bool ESPBones = false;

static char defaultName[144] = "BOT";

float espLineThickness = 1.0f;
float boneThickness = 1.0f;

static int selectedHealthBarPos = 0;
static int ESPBoxType = 0;
int lineType = 0;
int lineType1 = 0;

static float maxDistance = 150.f;


static int SelectedGame = 0;

bool RefreshEsp = false;
bool StreamerMode = false;
bool REIMAN = false;

static bool ActiveADB = false;

int ProcId = 0;

int FreezeLagKey;
int GhostHackKey;

int TeleportKey;
int TeleKillKey;
int UpPlayerKey;

int SpeedKey;
int WallhackFlyKey;

float rgbSpeed2 = 1.0f;

int g_windowWidth = GetSystemMetrics(SM_CXSCREEN);
int g_windowHeight = GetSystemMetrics(SM_CYSCREEN);
HWND hdPlayerWindow = NULL;
bool isClickable = true;

// �mgui
static int Main_Window = false;
static int Login_Verified = false;
static int Login_Window = true;


void ToggleClickability(bool clickable) {
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    if (clickable) {
        exStyle &= ~WS_EX_TRANSPARENT;
    }
    else {
        exStyle |= WS_EX_TRANSPARENT;
    }
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);

    // Redraw the window to apply the changes
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
}

void KillProcessByName(const char* processName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE)
        return;

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnap, &pe32)) {
        do {
            if (_stricmp(pe32.szExeFile, processName) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, pe32.th32ProcessID);
                if (hProcess) {
                    // Opcional: Esperar a que se cierre de forma segura (timeout: 2 segundos)
                    if (TerminateProcess(hProcess, 0)) {
                        WaitForSingleObject(hProcess, 2000);
                    }
                    CloseHandle(hProcess);

                    // Si quieres matar solo una instancia, puedes salir aqu�:
                    // break;
                }
            }
        } while (Process32Next(hSnap, &pe32));
    }

    CloseHandle(hSnap);
}


void DrawCircle(ImVec2 center, float radius, float thickness, int segments = 64) {
    ImDrawList* vList = ImGui::GetForegroundDrawList();
    float step = 1.0f / segments;
    float time = ImGui::GetTime();

    for (int i = 0; i < segments; ++i) {
        float t1 = i * step;
        float t2 = (i + 1) * step;

        float hue1 = fmod(t1 + time * rgbSpeed2, 1.0f);
        float hue2 = fmod(t2 + time * rgbSpeed2, 1.0f);

        ImU32 color1 = ImColor::HSV(hue1, 1.0f, 1.0f);
        ImU32 color2 = ImColor::HSV(hue2, 1.0f, 1.0f);

        float angle1 = t1 * 2.0f * IM_PI;
        float angle2 = t2 * 2.0f * IM_PI;

        ImVec2 p1(center.x + cos(angle1) * radius, center.y + sin(angle1) * radius);
        ImVec2 p2(center.x + cos(angle2) * radius, center.y + sin(angle2) * radius);

        vList->AddLine(p1, p2, color1, thickness);
    }
}


void KillProcessByD7(const char* processName) {
    HANDLE hSnap;
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return;

    if (Process32First(hSnap, &pe32)) {
        do {
            if (_stricmp(pe32.szExeFile, processName) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                if (hProcess) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                    std::cout << "Proceso " << processName << " cerrado exitosamente.\n";
                }
            }
        } while (Process32Next(hSnap, &pe32));
    }
    CloseHandle(hSnap);
}


std::wstring ReadWideString(uintptr_t nameAddr, size_t maxLength = 64) {
    std::wstring result;

    for (size_t i = 0; i < maxLength; i++) {
        uint16_t wchar_value = 0;
        if (!ReadZ(nameAddr + 0xC + (i * 2), wchar_value)) {
            break;
        }
        if (wchar_value == 0x0000) {
            break;
        }

        result += static_cast<wchar_t>(wchar_value);
    }

    return result;
}

std::string ConvertUTF16ToUTF8(const std::wstring& wstr) {
    if (wstr.empty()) return "";

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0) return "[InvalidName]";

    std::string utf8Str(size_needed - 1, 0);
    if (!WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8Str[0], size_needed, nullptr, nullptr)) {
        return "[InvalidName]";
    }

    return utf8Str;
}

std::string ReadPlayerName(uintptr_t nameAddr) {
    return ConvertUTF16ToUTF8(ReadWideString(nameAddr));
}




bool IsValidScreenPosition(const ImVec2& pos, int screenWidth, int screenHeight) {
    return !(pos.x <= 0.0f || pos.y <= 0.0f ||
        pos.x >= static_cast<float>(screenWidth) ||
        pos.y >= static_cast<float>(screenHeight) ||
        std::isnan(pos.x) || std::isnan(pos.y));
}

bool IsValidEntity(uint32_t entity) {
    if (!entity) return false;

    // Check if entity pointer is valid
    uint32_t validationCheck;
    if (!ReadZ(entity + 0x0, validationCheck)) return false;

    // Check if entity is alive
    bool isDead = true;
    if (!ReadZ(entity + Offsets::Player_IsDead, isDead)) return false;
    if (isDead) return false;

    // Check avatar manager
    uint32_t avatarManager;
    if (!ReadZ(entity + Offsets::AvatarManager, avatarManager)) return false;
    if (!avatarManager) return false;

    return true;
}

std::mutex cacheMutex;
float cacheClearInterval = 5.0f; // default 5s
static auto lastCacheClearTime = std::chrono::steady_clock::now();

// ---- Auto Refresh System ----
void AutoRefreshSystem()
{
    auto now = std::chrono::steady_clock::now();
    float elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - lastCacheClearTime).count();

    if (elapsedSeconds >= cacheClearInterval) {
        CacheZ.clear();
        lastCacheClearTime = now;

    }
}

void AutoRefreshLoop()
{
    while (true)
    {
        std::thread(AutoRefreshSystem).detach();
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // CPU'yu yormas�n
    }
}
namespace NoGravityFly {
    static constexpr float HORIZONTAL_SPEED = 6.0f;
    static constexpr float UP_SPEED = 3.0f;
    static constexpr float DOWN_SPEED = -3.0f;
    static constexpr float HOVER_SPEED = 0.02f;
    static constexpr float MAX_FLY_HEIGHT = 9000.0f;
    static float baseHeight = 0.0f;
    static bool heightInitialized = false;
    static bool boostApplied = false;
    static std::atomic<bool> isRunning{false};
    static std::thread workThread;
    static void ApplyFly() {
        uint32_t baseGameFacade=0,gameFacade=0,staticGameFacade=0,currentGame=0,currentMatch=0,localPlayer=0;
        if(!ReadZ(Offsets::Il2Cpp+Offsets::InitBase,baseGameFacade)||!baseGameFacade) return;
        if(!ReadZ(baseGameFacade,gameFacade)||!gameFacade) return;
        if(!ReadZ(gameFacade+Offsets::StaticClass,staticGameFacade)||!staticGameFacade) return;
        if(!ReadZ(staticGameFacade,currentGame)||!currentGame) return;
        if(!ReadZ(currentGame+Offsets::CurrentMatch,currentMatch)||!currentMatch) return;
        if(!ReadZ(currentMatch+Offsets::LocalPlayer,localPlayer)||!localPlayer) return;
        uint32_t moveComp=0; if(!ReadZ(localPlayer+Offsets::MovementComponent,moveComp)||!moveComp) return;
        uint8_t grounded=0; WriteZ<uint8_t>(localPlayer+Offsets::IsGrounded,grounded);
        Vector3 pos{}; if(!ReadZ(moveComp+Offsets::Position,pos)) return;
        if(!heightInitialized){baseHeight=pos.Y;heightInitialized=true;}
        if(!boostApplied){pos.Y+=20.0f;boostApplied=true;WriteZ<Vector3>(moveComp+Offsets::Position,pos);WriteZ<float>(moveComp+Offsets::VSpeed,UP_SPEED);return;}
        float maxAllowedHeight=baseHeight+MAX_FLY_HEIGHT;
        uint32_t mainTransform=0,followCamera=0,camera=0,cameraBase=0; Matrix4x4 viewMatrix{}; bool hasMatrix=false;
        if(ReadZ(localPlayer+Offsets::MainCameraTransform,mainTransform)&&mainTransform){Vector3 camPos; GetPosition(mainTransform,camPos); if(ReadZ(localPlayer+Offsets::FollowCamera,followCamera)&&followCamera){if(ReadZ(followCamera+Offsets::Camera,camera)&&camera){if(ReadZ(camera+0x8,cameraBase)&&cameraBase){if(ReadZ(cameraBase+Offsets::ViewMatrix,viewMatrix)) hasMatrix=true;}}}}
        if(!hasMatrix) return;
        Vector3 forward(viewMatrix.m02,viewMatrix.m12,viewMatrix.m22); Vector3 right(viewMatrix.m00,viewMatrix.m10,viewMatrix.m20);
        Vector3 fwd=Vector3::Normalized(Vector3(forward.X,0,forward.Z)); Vector3 rgt=Vector3::Normalized(Vector3(right.X,0,right.Z));
        Vector3 moveDir=Vector3::Zero();
        if(GetAsyncKeyState(0x57)&0x8000) moveDir=moveDir+fwd;
        if(GetAsyncKeyState(0x53)&0x8000) moveDir=moveDir-fwd;
        if(GetAsyncKeyState(0x44)&0x8000) moveDir=moveDir+rgt;
        if(GetAsyncKeyState(0x41)&0x8000) moveDir=moveDir-rgt;
        float moveLen=std::sqrt(moveDir.X*moveDir.X+moveDir.Z*moveDir.Z);
        if(moveLen>0){moveDir.X=(moveDir.X/moveLen)*HORIZONTAL_SPEED; moveDir.Z=(moveDir.Z/moveLen)*HORIZONTAL_SPEED;}
        pos.X+=moveDir.X*0.016f; pos.Z+=moveDir.Z*0.016f;
        if(pos.Y>maxAllowedHeight) pos.Y=maxAllowedHeight;
        WriteZ<Vector3>(moveComp+Offsets::Position,pos);
        float vSpeed=HOVER_SPEED;
        if((GetAsyncKeyState(0x20)&0x8000)&&pos.Y<maxAllowedHeight-0.1f) vSpeed=UP_SPEED;
        else if(GetAsyncKeyState(0xA2)&0x8000) vSpeed=DOWN_SPEED;
        WriteZ<float>(moveComp+Offsets::VSpeed,vSpeed);
    }
    static void Loop(){while(isRunning.load()){try{if(!NoGravityFlyEnabled){std::this_thread::sleep_for(std::chrono::milliseconds(40));continue;} ApplyFly();}catch(...){} std::this_thread::sleep_for(std::chrono::milliseconds(10));}}
    inline void Start(){if(isRunning.load()) return; isRunning=true; heightInitialized=false; boostApplied=false; workThread=std::thread(Loop); workThread.detach();}
    inline void Stop(){isRunning=false; heightInitialized=false; boostApplied=false;}
    inline bool IsRunning(){return isRunning.load();}
}