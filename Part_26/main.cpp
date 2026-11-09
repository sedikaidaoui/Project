#include <windows.h>
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <D3DX11tex.h>
#pragma comment(lib, "D3DX11.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")
#include <thread>
#include <iostream>
#include <atomic> 
#include <string>
#include <vector>
#include <ctime>
#include <include/MinHook.h>
#include <TlHelp32.h>
#include <src/adb/adb.hpp>
#include <src/Overlay/Overlay.hpp>
#include <src/Overlay/Render.hpp>
#include <src/Globals.hpp>
#include <EspLines/Data/Data.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Features/Visuals/Visual.hpp>
#include <src/adb/adb_utils.hpp>
#include <EspLines\Aimbot\Aimbot.hpp>
#include <EspLines\Aimbot\SilentAim.hpp>
#include <EspLines\Aimbot\AimbotLogic.h>
#include <EspLines/Data/SilentAimLegit.hpp>
#include <EspLines/Data/MovementExploits.hpp>
#include <EspLines/MethodsAll/MethodsAll.hpp>
#include <src/ui/ui.hpp>
#include <src/AuthGuard.hpp>
#include <src/CmdServer/CmdServer.hpp>
#include <src/aimkill/AimKillLink.hpp>
#include <MenuChams/Chams.h>
struct ThreadGuard {
    ThreadGuard() { g_Globals.General.ActiveThreads++; }
    ~ThreadGuard() { g_Globals.General.ActiveThreads--; }
};
extern int CurrentTab;

bool g_ConsoleAllocated = false;
void initcmd() {
	if (AllocConsole()) {
		g_ConsoleAllocated = true;
		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
		SetConsoleTitleA("Debug Console");
	}
}

void closecmd() {
	if (g_ConsoleAllocated) {
		fclose(stdin);
		fclose(stdout);
		fclose(stderr);
		FreeConsole();
		g_ConsoleAllocated = false;
	}
}

HMODULE g_hModule;
FWork::Interface* g_pInterface = nullptr;
std::thread g_DataThread;
std::thread g_InitThread;

DWORD WINAPI Unload() {
	g_Globals.General.ShutDown = true;

// 1. Stop feature threads
    CmdServer::Stop();          // canal de commandes : coupe AVANT la teardown
    AimKillLink::Shutdown();    // attend le pipeline CONNECT LIB (socket laisse ouvert)
    Aim::Aimbot::StopAimbot();
    Aim::SilentAim::Stop();
    SilentAimIntegration::StopAimLogic();
    SilentAimLegit::Cleanup();
    FWork::MovementExploits::Shutdown();
    FWork::MethodsAll::Shutdown();
    FWork::Data::RestoreFakeDamage();     // BuffDamage -> 1.0f avant de decharger

	// 2. Wait for data thread
	if (g_DataThread.joinable()) {
		g_DataThread.join();
	}

	// 3. Cleanup UI and Overlay
	if (g_pInterface) {
		g_pInterface->ShutDown();
		delete g_pInterface;
		g_pInterface = nullptr;
	} else {
		// If interface wasn't created, we still need to shut down the overlay
		FWork::Overlay::ShutDown();
	}

	// 4. Disable Hooks
	if (Mem::TargetPhysRead) {
		MH_DisableHook(Mem::TargetPhysRead);
		MH_RemoveHook(Mem::TargetPhysRead);
	}
	MH_Uninitialize();

	// 5. Cleanup Console
	closecmd();

	if (g_hModule) {
		FreeLibraryAndExitThread(g_hModule, 0);
	}
	return 0;
}

bool memoryinit = false;
void Memoryy() {
	HMODULE vmm = nullptr;
	int vmmRetries = 0;
	
	while (!vmm && vmmRetries < 20) {
		vmm = GetModuleHandleA("BstkVMM.dll");
		if (!vmm) {
			std::this_thread::sleep_for(std::chrono::milliseconds(250));
			vmmRetries++;
		}
	}

	if (vmm == nullptr) return;

	auto readFunc = (Mem::PGMPhysReadFunc)GetProcAddress(vmm, "PGMPhysRead");
	if (readFunc == nullptr) return;

	if (MH_Initialize() != MH_OK && MH_Initialize() != MH_ERROR_ALREADY_INITIALIZED) return;

	Mem::TargetPhysRead = (LPVOID)readFunc;
	MH_STATUS hookStatus = MH_CreateHook(Mem::TargetPhysRead, Mem::HookedPGMPhysRead, (LPVOID*)&Mem::ogPhysRead);
	if (hookStatus != MH_OK) return;

	if (MH_EnableHook(Mem::TargetPhysRead) != MH_OK) return;

    // --- ACTIVE VM RESOLVER ---
    if (Mem::vmPtr == nullptr) {
        typedef void* (__cdecl* GetVMFunc)();
        const char* exports[] = { "BstkGetVM", "GetVM", "GetVMPtr", "GetPVM" };
        for (const char* name : exports) {
            auto fn = (GetVMFunc)GetProcAddress(vmm, name);
            if (fn) {
                __try {
                    Mem::vmPtr = fn();
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    Mem::vmPtr = nullptr;
                }
                if (Mem::vmPtr) break;
            }
        }
    }

	int timeout = 3000;
	int elapsed = 0;
	while (Mem::vmPtr == nullptr && elapsed < timeout) {
		Sleep(1);
		elapsed++;
	}

	if (Mem::vmPtr == nullptr) return;

	Mem::ogCPU = (Mem::VMMGetCpuByIdFunc)GetProcAddress(vmm, "VMMGetCpuById");
	Mem::ogCast = (Mem::PGMPhysGCPtr2GCPhysFunc)GetProcAddress(vmm, "PGMPhysGCPtr2GCPhys");
	Mem::ogWrite = (Mem::PGMPhysSimpleWriteGCPhysFunc)GetProcAddress(vmm, "PGMPhysSimpleWriteGCPhys");

	if (!Mem::ogCPU || !Mem::ogCast || !Mem::ogWrite) return;

	__try {
		Mem::Initialize(Mem::vmPtr);
		memoryinit = (Mem::pVMAddr != nullptr && Mem::cpuAddr != nullptr);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		memoryinit = false;
	}
}

void BackgroundInit() {
    ThreadGuard guard;
    g_Globals.General.State = Globals::InitState::InitializingMemory;
    g_Globals.General.StatusMessage = "Initializing Memory Library...";
    Memoryy();
    if (!memoryinit) {
        g_Globals.General.State = Globals::InitState::Failed;
        g_Globals.General.StatusMessage = "Memory Error! BstkVMM.dll missing.";
        return;
    }

    g_Globals.General.State = Globals::InitState::FindingWindow;
    g_Globals.General.StatusMessage = "Looking for Emulator window...";
    HWND TargetHWND = nullptr;
    int retries = 0;
    while (!TargetHWND && retries < 40 && !g_Globals.General.ShutDown) {
        TargetHWND = Render::FindRenderWindow();
        if (!TargetHWND) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            retries++;
        }
    }

    if (!TargetHWND) {
        g_Globals.General.State = Globals::InitState::Failed;
        g_Globals.General.StatusMessage = "Emulator not found!";
        return;
    }

    // Attach to real window
    FWork::Overlay::Setup(TargetHWND);
    
    g_Globals.General.State = Globals::InitState::ReadyToScan;
    g_Globals.General.StatusMessage = "System Ready.";
}

namespace Cheat {
	void Initialize() {
		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

		// Anti-crack : baseline d'integrite .text DES le demarrage
		AuthGuard::Init();

		// Serveur de commandes TCP (127.0.0.1:5555) — demarre des l'injection
		CmdServer::Start();

		// Start UI with default size
		FWork::Overlay::Setup(nullptr); 
		FWork::Overlay::Initialize();
		
		if (FWork::Overlay::IsInitialized() && FWork::Overlay::dxGetDevice() && FWork::Overlay::dxGetDeviceContext())
		{
			g_pInterface = new FWork::Interface(FWork::Overlay::GetOverlayWindow(), FWork::Overlay::GetTargetWindow(), FWork::Overlay::dxGetDevice(), FWork::Overlay::dxGetDeviceContext());
			g_pInterface->UpdateStyle();
			FWork::Overlay::SetupWindowProcHook(std::bind(&FWork::Interface::WindowProc, g_pInterface, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

			// High-precision frame limiter setup
			LARGE_INTEGER qpcFreq, qpcLast;
			QueryPerformanceFrequency(&qpcFreq);
			QueryPerformanceCounter(&qpcLast);
			const double targetFrameTime = 1.0 / 144.0; // ~144 FPS cap

			MSG Message;
			ZeroMemory(&Message, sizeof(Message));
			while (Message.message != WM_QUIT && !g_Globals.General.ShutDown) {

				// Drain ALL pending messages per frame for responsive input
				while (PeekMessage(&Message, NULL, 0U, 0U, PM_REMOVE))
				{
					TranslateMessage(&Message);
					DispatchMessage(&Message);
					if (Message.message == WM_QUIT) break;
				}
				if (Message.message == WM_QUIT) break;

				if (g_pInterface) {
					ImGui::GetIO().MouseDrawCursor = g_pInterface->GetMenuOpen();

					if (g_pInterface->ResizeHeight != 0 || g_pInterface->ResizeWidht != 0) 
					{
						FWork::Overlay::dxCleanupRenderTarget();
						FWork::Overlay::dxGetSwapChain()->ResizeBuffers(0, g_pInterface->ResizeWidht, g_pInterface->ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
						g_pInterface->ResizeHeight = g_pInterface->ResizeWidht = 0;
						FWork::Overlay::dxCreateRenderTarget();
					}

					g_pInterface->HandleMenuKey();
				}
				
				FWork::Overlay::UpdateWindowPos();

				static bool CaptureBypassOn = false;
				if (g_Globals.General.Capture != CaptureBypassOn) 
				{
					CaptureBypassOn = g_Globals.General.Capture;
					SetWindowDisplayAffinity(FWork::Overlay::GetOverlayWindow(), CaptureBypassOn ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
				}

				// --- Anti-crack : session / anti-debug / integrite a chaque frame
				AuthGuard::Poll();
				if (AuthGuard::Tampered()) {
					g_Globals.General.ShutDown = true;   // decharge propre (Unload)
					break;
				}

				ImGui_ImplDX11_NewFrame();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();
				{
					// Chạy logic data trong luồng riêng nếu đã hoàn tất scan
					static bool dataThreadStarted = false;
					if (!dataThreadStarted && Offsets::Il2Cpp != 0 && AuthGuard::Unlocked()) {
						dataThreadStarted = true;
						FWork::MethodsAll::Initialize();
						SilentAimIntegration::StartAimLogic();
						g_DataThread = std::thread([]() {
							ThreadGuard guard;
							while (!g_Globals.General.ShutDown) {
								if (AuthGuard::Unlocked())              // session perdue -> plus de donnees
									FWork::Data::Work();
								std::this_thread::sleep_for(std::chrono::milliseconds(2)); // Snappier ESP data (was 8ms)
							}
						});
					}
					
					if (g_pInterface) {
						g_pInterface->RenderGui();
						g_pInterface->UpdateClickThrough();
					}

					if (CurrentTab >= 2 && Offsets::Il2Cpp != 0 && AuthGuard::Unlocked()) {
						if (g_Globals.Visuals.Enable) {
							ESP::Players();
						}
						ESP::PlayerCounter();   // compteur d'ennemis haut-centre
					}
				}

				ImGui::EndFrame();
				ImGui::Render();
				FWork::Overlay::dxRefresh();
				ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
				FWork::Overlay::dxGetSwapChain()->Present(0, 0); // No VSync — uncapped for lowest latency

				// High-precision frame limiter: spin-wait to target FPS (preserves user Delay on top)
				{
					double extraDelay = g_Globals.General.Delay * 0.001; // user delay in seconds
					double totalTarget = targetFrameTime + extraDelay;
					LARGE_INTEGER qpcNow;
					for (;;) {
						QueryPerformanceCounter(&qpcNow);
						double elapsed = (double)(qpcNow.QuadPart - qpcLast.QuadPart) / (double)qpcFreq.QuadPart;
						if (elapsed >= totalTarget) break;
						// Yield CPU briefly if we have more than 1ms to spare
						if (totalTarget - elapsed > 0.001)
							Sleep(0);
					}
					qpcLast = qpcNow;
				}
			}
		}
	}
}

void MainLogic() {
	srand((unsigned int)time(NULL));
	g_Globals.General.ShutDown = false;
    Cheat::Initialize();

	while (!g_Globals.General.ShutDown) {
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	if (g_InitThread.joinable()) g_InitThread.join();
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	Unload();
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    MainLogic();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Start, hModule, 0, NULL);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, NULL);
        break;
    case DLL_PROCESS_DETACH:
		g_Globals.General.ShutDown = true;
        break;
    }
    return TRUE;
}
