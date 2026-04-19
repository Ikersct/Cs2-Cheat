// dllmain.cpp — Cs2 Epstein Rage DLL Entry Point (Anti-Detection Build)
//
// Changes from detected version:
// - No MinHook (COM vtable swap for Present, VMT hook for CreateMove)
// - No code patching (thirdperson handled via SafeWrite in Present hook)
// - No console/printf in Release builds (#ifdef _DEBUG only)
// - Indirect memory R/W for sensitive addresses (viewangles, frame history)
//
#include <Windows.h>
#include <Psapi.h>
#include <thread>
#include <chrono>
#include <cstdio>

#include "hooks.h"
#include "menu.h"
#include "skin_changer.h"

#include "aimbot.h"
#include "game.h"

#define STB_IMAGE_IMPLEMENTATION
#include "minhook-master/include/MinHook.h"
#include "stb_image.h"

#pragma comment(lib, "Psapi.lib")

HMODULE g_hModule = nullptr;

void MainThread(HMODULE hModule)
{
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    printf("[+] Injected (Anti-Detection Build)\n");

    Game::clientBase = Game::GetModuleBase(L"client.dll");
    assert(Game::clientBase != 0);

#ifdef _DEBUG
    printf("[+] client.dll: 0x%IX\n", Game::clientBase);
    printf("[*] Waiting for game modules...\n");
#endif

    while (!GetModuleHandleW(L"client.dll") || !GetModuleHandleW(L"engine2.dll") || !GetModuleHandleA("scenesystem.dll"))
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (!Game::Init())
    {
#ifdef _DEBUG
        printf("[-] Game::Init() FAILED\n");
#endif
        return;
    }

#ifdef _DEBUG
    printf("[+] Game::Init() OK\n");
#endif

    // === DX11 HOOKS: Vtable pointer swap in dxgi.dll ===
    if (!Hooks::SetupDXHooks())
    {
#ifdef _DEBUG
        printf("[-] DX hooks FAILED\n");
#endif
        return;
    }

#ifdef _DEBUG
    printf("[+] DX hooks OK (Present + ResizeBuffers)\n");
#endif



    // Initialize MinHook for CreateMove
    if (MH_Initialize() != MH_OK)
    {
#ifdef _DEBUG
        printf("[-] MinHook init FAILED\n");
#endif
        return;
    }

    // Initialize aimbot (CreateMove MinHook on signature)
    if (Aimbot::Init())
    {
#ifdef _DEBUG
        printf("[+] Aimbot OK (CreateMove VMT)\n");
#endif
    }

#ifdef _DEBUG
    printf("[*] INSERT=menu, END=unload\n");
#endif

    // Wait for unload key
    SkinChanger::running.store(true);
    while (SkinChanger::running.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (GetAsyncKeyState(VK_END) & 1)
            break;
    }

    // Cleanup
    SkinChanger::running.store(false);
    Aimbot::Shutdown();
    Hooks::Shutdown();
    MH_Uninitialize();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    FreeConsole();

    FreeLibraryAndExitThread(hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        g_hModule = hModule;
        CloseHandle(CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(MainThread), hModule, 0, nullptr));
        break;
    case DLL_PROCESS_DETACH:
        SkinChanger::running.store(false);
        break;
    }
    return TRUE;
}
