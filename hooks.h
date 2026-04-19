#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include "imgui-master/imgui.h"
#include "imgui-master/backends/imgui_impl_win32.h"
#include "imgui-master/backends/imgui_impl_dx11.h"
#include "skin_changer.h"
#include "bullet_tracer.h"
#include "aimbot.h"
#include "vmt_hook.h"
#include "esp.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// Forward declaration for Win32 message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Hooks
{
    // Function pointer types
    using PresentFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
    using ResizeBuffersFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

    // Originals (saved before vtable swap)
    inline PresentFn oPresent = nullptr;
    inline ResizeBuffersFn oResizeBuffers = nullptr;
    inline WNDPROC oWndProc = nullptr;

    // Pointer to the vtable entries we patched (for cleanup)
    inline uintptr_t* pVTablePresentEntry = nullptr;
    inline uintptr_t* pVTableResizeEntry = nullptr;

    inline ID3D11Device* pDevice = nullptr;
    inline ID3D11DeviceContext* pContext = nullptr;
    inline ID3D11RenderTargetView* pRenderTargetView = nullptr;
    inline HWND gameWindow = nullptr;
    inline bool initialized = false;
    inline bool showMenu = true;

    // Cleanup render target
    inline void CleanupRenderTarget()
    {
        if (pRenderTargetView)
        {
            pRenderTargetView->Release();
            pRenderTargetView = nullptr;
        }
    }

    // Create render target from swap chain
    inline void CreateRenderTarget(IDXGISwapChain* pSwapChain)
    {
        ID3D11Texture2D* pBackBuffer = nullptr;
        if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer)))
        {
            pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView);
            pBackBuffer->Release();
        }
    }

    // Window procedure hook
    inline LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        // Toggle menu with INSERT key
        if (msg == WM_KEYDOWN && wParam == VK_INSERT)
        {
            showMenu = !showMenu;
            return 0;
        }

        // Pass input to ImGui when menu is shown
        if (showMenu)
        {
            if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
                return true;

            // Block game input when menu is open
            switch (msg)
            {
            case WM_LBUTTONDOWN: case WM_LBUTTONUP:
            case WM_RBUTTONDOWN: case WM_RBUTTONUP:
            case WM_MBUTTONDOWN: case WM_MBUTTONUP:
            case WM_MOUSEWHEEL:
            case WM_MOUSEMOVE:
                if (ImGui::GetIO().WantCaptureMouse)
                    return 0;
                break;
            case WM_KEYDOWN: case WM_KEYUP:
            case WM_CHAR:
                if (ImGui::GetIO().WantCaptureKeyboard)
                    return 0;
                break;
            }
        }

        return CallWindowProcW(oWndProc, hWnd, msg, wParam, lParam);
    }

    // Forward declarations
    void RenderMenu();

    // DX11 Present hook
    inline HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
    {
        if (!initialized)
        {
            // Get device and context
            if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
                return oPresent(pSwapChain, SyncInterval, Flags);

            pDevice->GetImmediateContext(&pContext);

            // Get window handle
            DXGI_SWAP_CHAIN_DESC desc;
            pSwapChain->GetDesc(&desc);
            gameWindow = desc.OutputWindow;

            // Create render target
            CreateRenderTarget(pSwapChain);

            // Init ImGui
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.IniFilename = nullptr; // Don't save ini
            
            // Load clean bold font
            io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\tahomabd.ttf", 14.0f);

            // Setup dark theme
            ImGui::StyleColorsDark();
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 8.0f;
            style.FrameRounding = 4.0f;
            style.GrabRounding = 4.0f;
            style.ScrollbarRounding = 4.0f;
            style.TabRounding = 4.0f;
            style.WindowBorderSize = 1.0f;
            style.FrameBorderSize = 0.0f;
            style.PopupRounding = 4.0f;
            style.WindowPadding = ImVec2(12, 12);
            style.FramePadding = ImVec2(8, 4);
            style.ItemSpacing = ImVec2(8, 6);
            style.ItemInnerSpacing = ImVec2(6, 4);

            // Premium dark color scheme
            ImVec4* colors = style.Colors;
            colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.08f, 0.96f);
            colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);
            colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.98f);
            colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.25f, 0.60f);
            colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.0f);
            colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.22f, 1.0f);
            colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.22f, 0.28f, 1.0f);
            colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.08f, 1.0f);
            colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.14f, 1.0f);
            colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.06f, 0.06f, 0.08f, 0.75f);
            colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.14f, 1.0f);
            colors[ImGuiCol_ScrollbarBg] = ImVec4(0.06f, 0.06f, 0.08f, 0.60f);
            colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.30f, 1.0f);
            colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.40f, 1.0f);
            colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.50f, 1.0f);
            colors[ImGuiCol_CheckMark] = ImVec4(0.85f, 0.20f, 0.20f, 1.0f);
            colors[ImGuiCol_SliderGrab] = ImVec4(0.85f, 0.20f, 0.20f, 0.80f);
            colors[ImGuiCol_SliderGrabActive] = ImVec4(0.95f, 0.25f, 0.25f, 1.0f);
            colors[ImGuiCol_Button] = ImVec4(0.85f, 0.20f, 0.20f, 0.65f);
            colors[ImGuiCol_ButtonHovered] = ImVec4(0.90f, 0.25f, 0.25f, 0.80f);
            colors[ImGuiCol_ButtonActive] = ImVec4(0.95f, 0.30f, 0.30f, 1.0f);
            colors[ImGuiCol_Header] = ImVec4(0.85f, 0.20f, 0.20f, 0.40f);
            colors[ImGuiCol_HeaderHovered] = ImVec4(0.85f, 0.20f, 0.20f, 0.55f);
            colors[ImGuiCol_HeaderActive] = ImVec4(0.85f, 0.20f, 0.20f, 0.70f);
            colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.25f, 0.60f);
            colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.12f, 0.15f, 1.0f);
            colors[ImGuiCol_TabHovered] = ImVec4(0.85f, 0.20f, 0.20f, 0.60f);
            colors[ImGuiCol_TabActive] = ImVec4(0.85f, 0.20f, 0.20f, 0.80f);
            colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
            colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.0f);

            ImGui_ImplWin32_Init(gameWindow);
            ImGui_ImplDX11_Init(pDevice, pContext);

            // Hook WndProc
            oWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(gameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));

            initialized = true;
        }
        // Run skin changer on the game's render thread (critical for regen call)
        SkinChanger::Tick();

        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Render menu
        if (showMenu)
            RenderMenu();

        // Render overlay graphics (always, even when menu is hidden)
        ESP::Render();
        Aimbot::RenderFOV();
        BulletTracer::Render();

        // End frame and render
        ImGui::Render();

        pContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // === FORCE THIRD PERSON (Restored Code Patch) ===
        // The Anti-Detection update removed the code patch which caused online servers to jitter the camera.
        // We restore the original method: patching the camera reset branch, then setting 0x229 once.
        static uintptr_t tpResetAddr = 0;
        if (!tpResetAddr && Game::clientBase)
            tpResetAddr = Game::FindPattern(L"client.dll", Offsets::sig_ThirdPersonReset);

        // Toggle third-person state by hotkey while in-game.
        if (SkinChanger::thirdPersonToggleKey != 0 && (GetAsyncKeyState(SkinChanger::thirdPersonToggleKey) & 1))
            SkinChanger::thirdPerson = !SkinChanger::thirdPerson;

        if (tpResetAddr)
        {
            static bool lastTpState = false;
            // Handle the assembly patch statefully (only write to .text section on change)
            if (SkinChanger::thirdPerson != lastTpState)
            {
                DWORD old;
                VirtualProtect(reinterpret_cast<void*>(tpResetAddr), 16, PAGE_EXECUTE_READWRITE, &old);
                if (SkinChanger::thirdPerson)
                    *reinterpret_cast<uint8_t*>(tpResetAddr + 7) = 0xEB; // JMP (skip reset)
                else
                    *reinterpret_cast<uint8_t*>(tpResetAddr + 7) = 0x75; // JNE (restore reset)
                VirtualProtect(reinterpret_cast<void*>(tpResetAddr), 16, old, &old);

                lastTpState = SkinChanger::thirdPerson;
            }

            // Write the camera state continuously.
            // When combined with the patch above, this survives respawns and doesn't jitter online.
            if (Game::clientBase)
            {
                Game::Write<bool>(Game::clientBase + Offsets::dwCSGOInput + 0x229, SkinChanger::thirdPerson);
            }
        }

        return oPresent(pSwapChain, SyncInterval, Flags);
    }

    // ResizeBuffers hook (handle window resize)
    inline HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
    {
        CleanupRenderTarget();
        HRESULT hr = oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
        CreateRenderTarget(pSwapChain);
        return hr;
    }

    // === Vtable pointer swap (no MinHook, no trampoline) ===
    // Overwrites function pointers in the COM vtable directly.
    // This requires VirtualProtect since the vtable is in .rdata,
    // BUT it's in dxgi.dll (not client.dll), which VAC doesn't checksum.
    inline void SwapVTableEntry(uintptr_t* pEntry, void* pNewFn, void** ppOriginal)
    {
        DWORD oldProtect;
        VirtualProtect(pEntry, sizeof(uintptr_t), PAGE_READWRITE, &oldProtect);
        *ppOriginal = reinterpret_cast<void*>(*pEntry);
        *pEntry = reinterpret_cast<uintptr_t>(pNewFn);
        VirtualProtect(pEntry, sizeof(uintptr_t), oldProtect, &oldProtect);
    }

    inline void RestoreVTableEntry(uintptr_t* pEntry, void* pOriginalFn)
    {
        if (!pEntry || !pOriginalFn) return;
        DWORD oldProtect;
        VirtualProtect(pEntry, sizeof(uintptr_t), PAGE_READWRITE, &oldProtect);
        *pEntry = reinterpret_cast<uintptr_t>(pOriginalFn);
        VirtualProtect(pEntry, sizeof(uintptr_t), oldProtect, &oldProtect);
    }

    // Setup: create dummy swap chain, get vtable, swap entries
    inline bool SetupDXHooks()
    {
        DXGI_SWAP_CHAIN_DESC sd{};
        sd.BufferCount = 1;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = GetForegroundWindow();
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        IDXGISwapChain* pDummy = nullptr;
        ID3D11Device* pDev = nullptr;
        ID3D11DeviceContext* pCtx = nullptr;
        D3D_FEATURE_LEVEL fl;

        if (FAILED(D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION,
            &sd, &pDummy, &pDev, &fl, &pCtx)))
            return false;

        // Get the vtable from the dummy swap chain
        uintptr_t* vtable = *reinterpret_cast<uintptr_t**>(pDummy);

        // Save pointers to the vtable entries we'll patch
        pVTablePresentEntry = &vtable[8];    // Present = index 8
        pVTableResizeEntry  = &vtable[13];   // ResizeBuffers = index 13

        // Swap the vtable entries
        SwapVTableEntry(pVTablePresentEntry, &hkPresent, reinterpret_cast<void**>(&oPresent));
        SwapVTableEntry(pVTableResizeEntry, &hkResizeBuffers, reinterpret_cast<void**>(&oResizeBuffers));

        // Clean up dummy objects (vtable persists in dxgi.dll)
        pDummy->Release();
        pDev->Release();
        pCtx->Release();

        return true;
    }

    // Cleanup
    inline void Shutdown()
    {
        if (initialized)
        {
            // Restore WndProc
            if (oWndProc && gameWindow)
                SetWindowLongPtrW(gameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(oWndProc));

            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();

            CleanupRenderTarget();

            if (pContext) { pContext->Release(); pContext = nullptr; }
            if (pDevice) { pDevice->Release(); pDevice = nullptr; }

            initialized = false;
        }

        // Restore original vtable entries
        RestoreVTableEntry(pVTablePresentEntry, oPresent);
        RestoreVTableEntry(pVTableResizeEntry, oResizeBuffers);
        oPresent = nullptr;
        oResizeBuffers = nullptr;
    }
}
