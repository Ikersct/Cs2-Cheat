#pragma once
#include "imgui-master/imgui.h"
#include "skin_changer.h"
#include "esp.h"
#include "aimbot.h"
#include "bhop.h"
#include "triggerbot.h"
#include "paint_kits.h"
#include "config_system.h"
#include <cstdlib>
#include <ctime>

// ============================================
// ImGui Menu — Epstein Rage Skin Changer
// ============================================

namespace Menu
{
    // --- State ---
    inline int selectedTab = 0;
    inline int selectedWeaponIdx = 0;
    inline char searchBuffer[128] = "";

    // Weapons list for the combo box
    struct WeaponEntry
    {
        int defIndex;
        const char* name;
    };

    inline WeaponEntry weaponList[] = {
        { 7,  "AK-47" },
        { 8,  "AUG" },
        { 9,  "AWP" },
        { 1,  "Desert Eagle" },
        { 2,  "Dual Berettas" },
        { 10, "FAMAS" },
        { 3,  "Five-SeveN" },
        { 13, "Galil AR" },
        { 11, "G3SG1" },
        { 4,  "Glock-18" },
        { 14, "M249" },
        { 16, "M4A4" },
        { 60, "M4A1-S" },
        { 17, "MAC-10" },
        { 27, "MAG-7" },
        { 33, "MP7" },
        { 34, "MP9" },
        { 23, "MP5-SD" },
        { 28, "Negev" },
        { 35, "Nova" },
        { 19, "P90" },
        { 32, "P2000" },
        { 36, "P250" },
        { 26, "PP-Bizon" },
        { 64, "R8 Revolver" },
        { 29, "Sawed-Off" },
        { 38, "SCAR-20" },
        { 39, "SG 553" },
        { 40, "SSG 08" },
        { 30, "Tec-9" },
        { 24, "UMP-45" },
        { 61, "USP-S" },
        { 25, "XM1014" },
        { 63, "CZ75-Auto" },
        // Knives
        { 500, "Bayonet" },
        { 505, "Flip Knife" },
        { 506, "Gut Knife" },
        { 507, "Karambit" },
        { 508, "M9 Bayonet" },
        { 509, "Huntsman Knife" },
        { 512, "Falchion Knife" },
        { 514, "Bowie Knife" },
        { 515, "Butterfly Knife" },
        { 516, "Shadow Daggers" },
        { 517, "Paracord Knife" },
        { 519, "Ursus Knife" },
        { 520, "Navaja Knife" },
        { 521, "Nomad Knife" },
        { 522, "Stiletto Knife" },
        { 523, "Talon Knife" },
        { 525, "Skeleton Knife" },
        { 526, "Kukri Knife" },
    };
    inline constexpr int weaponListCount = sizeof(weaponList) / sizeof(weaponList[0]);

    // --- Helper: get or create skin config ---
    inline SkinChanger::SkinConfig& GetSkinConfig(int defIndex)
    {
        return SkinChanger::weaponSkins[defIndex];
    }
}

// ============================================
// RenderMenu — called from hooks.h Present hook
// ============================================

#include <d3d11.h>
#include "stb_image.h"

namespace Menu
{
    inline ID3D11ShaderResourceView* bgTextureView = nullptr;
    inline bool bgLoaded = false;
    inline bool bgLoadAttempted = false;
    inline bool flipBackground = false;

    inline void LoadBackgroundImage(ID3D11Device* device)
    {
        if (bgLoaded || bgLoadAttempted || !device) return;
        bgLoadAttempted = true;
        
        int w, h, channels;
        stbi_set_flip_vertically_on_load(false); // Do not flip on load automatically
        unsigned char* data = stbi_load("bg.jpg", &w, &h, &channels, 4);
        if (!data) return;
        
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = w;
        desc.Height = h;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = data;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        
        ID3D11Texture2D* pTexture = nullptr;
        if (SUCCEEDED(device->CreateTexture2D(&desc, &subResource, &pTexture)))
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            ZeroMemory(&srvDesc, sizeof(srvDesc));
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            device->CreateShaderResourceView(pTexture, &srvDesc, &bgTextureView);
            pTexture->Release();
            bgLoaded = true;
        }
        stbi_image_free(data);
    }
    
    inline void ApplyGrayTheme()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        style.Colors[ImGuiCol_Text]                  = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
        style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
        style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.06f, 0.06f, 0.07f, 0.58f);
        style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.08f, 0.08f, 0.09f, 0.28f);
        style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.08f, 0.08f, 0.10f, 0.70f);
        style.Colors[ImGuiCol_Border]                = ImVec4(0.28f, 0.28f, 0.30f, 0.30f);
        style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.16f, 0.16f, 0.18f, 0.45f);
        style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.20f, 0.20f, 0.23f, 0.55f);
        style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.24f, 0.24f, 0.28f, 0.65f);
        style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.06f, 0.06f, 0.08f, 0.70f);
        style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.09f, 0.09f, 0.11f, 0.85f);
        style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.04f, 0.04f, 0.05f, 0.55f);
        style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.10f, 0.10f, 0.12f, 0.55f);
        style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.45f, 0.45f, 0.48f, 0.95f);
        style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.58f, 0.58f, 0.62f, 1.00f);
        style.Colors[ImGuiCol_Button]                = ImVec4(0.20f, 0.20f, 0.22f, 0.45f);
        style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.27f, 0.27f, 0.30f, 0.70f);
        style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.33f, 0.33f, 0.37f, 0.85f);
        style.Colors[ImGuiCol_Header]                = ImVec4(0.26f, 0.26f, 0.29f, 0.35f);
        style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.34f, 0.34f, 0.37f, 0.65f);
        style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.40f, 0.40f, 0.44f, 0.85f);
        style.Colors[ImGuiCol_Separator]             = ImVec4(0.32f, 0.32f, 0.35f, 0.35f);
        style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.42f, 0.42f, 0.45f, 0.50f);
        style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.52f, 0.52f, 0.55f, 0.65f);
        style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
        style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.30f, 0.30f, 0.30f, 0.67f);
        style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.40f, 0.40f, 0.40f, 0.95f);
        style.Colors[ImGuiCol_Tab]                   = ImVec4(0.16f, 0.16f, 0.18f, 0.65f);
        style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.26f, 0.26f, 0.30f, 0.78f);
        style.Colors[ImGuiCol_TabActive]             = ImVec4(0.24f, 0.24f, 0.28f, 0.85f);
        style.Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.10f, 0.10f, 0.12f, 0.55f);
        style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.15f, 0.15f, 0.18f, 0.65f);

        style.WindowRounding = 4.0f;
        style.FrameRounding = 3.0f;
        style.PopupRounding = 3.0f;
        style.ScrollbarRounding = 4.0f;
        style.TabRounding = 3.0f;
        style.WindowBorderSize = 0.0f;
        style.ChildBorderSize = 0.0f;
        style.FrameBorderSize = 0.0f;
    }
}

namespace Hooks
{
    // Need to access global device from hooks.h (which implements this file)
    extern ID3D11Device* pDevice;

    inline void RenderMenu()
    {
        using namespace Menu;
        using namespace SkinChanger;

        if (!bgLoaded && pDevice)
        {
            LoadBackgroundImage(pDevice);
        }

        ApplyGrayTheme();

        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        
        ImGui::Begin("Epstein Rage v1.3", &Hooks::showMenu, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Draw Background if loaded.
        if (bgTextureView)
        {
            if (flipBackground) {
                // Upside down rendering coordinates
                drawList->AddImage(bgTextureView, winPos, ImVec2(winPos.x + winSize.x, winPos.y + winSize.y), ImVec2(0, 1), ImVec2(1, 0));
            } else {
                // Standard rendering coordinates
                drawList->AddImage(bgTextureView, winPos, ImVec2(winPos.x + winSize.x, winPos.y + winSize.y), ImVec2(0, 0), ImVec2(1, 1));
            }
        }

        // Inner Padding - move cursor right to give text breathing room
        ImGui::SetCursorPos(ImVec2(15, 30));
        
        // Restore internal padding so child windows look normal
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        ImGui::BeginChild("MainArea", ImVec2(winSize.x - 30, winSize.y - 45), false);

        if (ImGui::BeginTabBar("MainTabs", ImGuiTabBarFlags_NoTooltip))
        {
            // --------------- RAGE TAB ---------------
            if (ImGui::BeginTabItem("Rage"))
            {
                ImGui::Spacing();
                ImGui::Columns(2, "rageAimbotLayout", false);
                
                // Left Column
                ImGui::BeginChild("AimbotLeft", ImVec2(0, 0), true);
                ImGui::Text("Main Aimbot");
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::Checkbox("Enable Aimbot", &Aimbot::config.enabled);
                ImGui::Checkbox("Legit Mode (Humanized)", &Aimbot::config.legitMode);
                
                const char* keys[] = { "Auto", "Mouse2", "Mouse4", "Mouse5", "Shift", "Alt" };
                int keyIdx = 0;
                if (Aimbot::config.aimKey == VK_RBUTTON) keyIdx = 1;
                else if (Aimbot::config.aimKey == VK_XBUTTON1) keyIdx = 2;
                else if (Aimbot::config.aimKey == VK_XBUTTON2) keyIdx = 3;
                else if (Aimbot::config.aimKey == VK_SHIFT) keyIdx = 4;
                else if (Aimbot::config.aimKey == VK_MENU) keyIdx = 5;
                if (ImGui::Combo("Aim Key", &keyIdx, keys, IM_ARRAYSIZE(keys)))
                {
                    const int vks[] = { 0, VK_RBUTTON, VK_XBUTTON1, VK_XBUTTON2, VK_SHIFT, VK_MENU };
                    Aimbot::config.aimKey = vks[keyIdx];
                }
                
                ImGui::Checkbox("Auto Shoot", &Aimbot::config.autoShoot);
                if (!Aimbot::config.legitMode)
                    ImGui::Checkbox("Silent Aim", &Aimbot::config.silentAim);
                else
                {
                    bool disabledSilent = false;
                    ImGui::BeginDisabled();
                    ImGui::Checkbox("Silent Aim (disabled in legit mode)", &disabledSilent);
                    ImGui::EndDisabled();
                }
                ImGui::Checkbox("Team Check", &Aimbot::config.teamCheck);
                if (Aimbot::config.teamCheck)
                    ImGui::Checkbox("Target Teammates (DM)", &Aimbot::config.targetTeammates);
                
                ImGui::EndChild();

                ImGui::NextColumn();
                
                // Right Column
                ImGui::BeginChild("AimbotRight", ImVec2(0, 0), true);
                ImGui::Text("Configuration");
                ImGui::Separator();
                ImGui::Spacing();
                
                const char* fovTypes[] = { "Angle (Degrees)", "Screen (Pixels)" };
                ImGui::Combo("FOV Mode", &Aimbot::config.fovType, fovTypes, IM_ARRAYSIZE(fovTypes));
                if (Aimbot::config.fovType == 0)
                    ImGui::SliderFloat("FOV", &Aimbot::config.fov, Aimbot::config.legitMode ? 1.0f : 1.0f, Aimbot::config.legitMode ? 15.0f : 180.0f, "%.1f deg");
                else
                    ImGui::SliderFloat("FOV", &Aimbot::config.screenFov, 10.0f, Aimbot::config.legitMode ? 250.0f : 1000.0f, "%.0f px");
                
                const char* bones[] = { "Head", "Neck", "Chest", "Pelvis" };
                int boneIdx = 0;
                if (Aimbot::config.targetBone == 5) boneIdx = 1;
                else if (Aimbot::config.targetBone == 4) boneIdx = 2;
                else if (Aimbot::config.targetBone == 3) boneIdx = 3;
                if (ImGui::Combo("Hitbox", &boneIdx, bones, IM_ARRAYSIZE(bones)))
                {
                    const int boneIds[] = { 6, 5, 4, 3 };
                    Aimbot::config.targetBone = boneIds[boneIdx];
                }

                if (Aimbot::config.legitMode)
                {
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Text("Legit Assist Tuning");
                    ImGui::SliderFloat("Smoothing", &Aimbot::config.legitSmooth, 2.0f, 25.0f, "%.1f");
                    ImGui::SliderFloat("Max Step (deg/tick)", &Aimbot::config.legitMaxStep, 0.2f, 4.0f, "%.2f");
                    ImGui::SliderInt("Reaction Delay (ms)", &Aimbot::config.legitReactionMs, 0, 300);
                    ImGui::SliderFloat("Micro Jitter", &Aimbot::config.legitJitter, 0.0f, 0.30f, "%.2f");
                    ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.7f, 1.0f), "Tip: smaller FOV + higher smoothing looks most legit.");
                    if (ImGui::Button("Apply Balanced Legit Preset"))
                    {
                        Aimbot::config.fovType = 0;
                        Aimbot::config.fov = 3.5f;
                        Aimbot::config.legitSmooth = 10.5f;
                        Aimbot::config.legitMaxStep = 1.2f;
                        Aimbot::config.legitReactionMs = 75;
                        Aimbot::config.legitJitter = 0.06f;
                    }
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Triggerbot");
                ImGui::Spacing();
                
                ImGui::Checkbox("Enable Triggerbot", &Triggerbot::config.enabled);
                
                const char* tbKeys[] = { "Auto", "Mouse2", "Mouse4", "Mouse5", "Shift", "Alt" };
                int tbKeyIdx = 0;
                if (Triggerbot::config.key == VK_RBUTTON) tbKeyIdx = 1;
                else if (Triggerbot::config.key == VK_XBUTTON1) tbKeyIdx = 2;
                else if (Triggerbot::config.key == VK_XBUTTON2) tbKeyIdx = 3;
                else if (Triggerbot::config.key == VK_SHIFT) tbKeyIdx = 4;
                else if (Triggerbot::config.key == VK_MENU) tbKeyIdx = 5;
                if (ImGui::Combo("Trigger Key", &tbKeyIdx, tbKeys, IM_ARRAYSIZE(tbKeys)))
                {
                    const int tbvks[] = { 0, VK_RBUTTON, VK_XBUTTON1, VK_XBUTTON2, VK_SHIFT, VK_MENU };
                    Triggerbot::config.key = tbvks[tbKeyIdx];
                    Triggerbot::config.useKey = (tbvks[tbKeyIdx] != 0);
                }
                ImGui::Checkbox("Team Check (Trigger)", &Triggerbot::config.teamCheck);
                ImGui::SliderInt("Delay (ms)", &Triggerbot::config.delayMs, 0, 200);

                ImGui::EndChild();
                
                ImGui::Columns(1);
                ImGui::EndTabItem();
            }

            // --------------- VISUALS TAB ---------------
            if (ImGui::BeginTabItem("Visuals"))
            {
                ImGui::Spacing();
                
                ImGui::BeginChild("VisualsMain", ImVec2(0, 0), true);
                ImGui::Text("ESP");
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::Checkbox("Enable ESP", &ESP::config.enabled);
                if (ESP::config.enabled)
                {
                    ImGui::Checkbox("Box", &ESP::config.bBox);
                    if (ESP::config.bBox)
                    {
                        const char* modes[] = { "Normal", "Corners" };
                        ImGui::Combo("Box Style", &ESP::config.boxMode, modes, IM_ARRAYSIZE(modes));
                    }
                    ImGui::ColorEdit4("Box Color", ESP::config.boxColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                    ImGui::Checkbox("Skeleton", &ESP::config.bSkeleton);
                    ImGui::ColorEdit4("Skeleton Color", ESP::config.skeletonColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                    ImGui::Checkbox("Health Bar", &ESP::config.bHealthBar);
                    ImGui::Checkbox("Name", &ESP::config.bName);
                    ImGui::Checkbox("Distance", &ESP::config.bDistance);
                    ImGui::Checkbox("Team Check", &ESP::config.teamCheck);
                if (ESP::config.teamCheck)
                    ImGui::Checkbox("Show Teammates (Deathmatch)", &ESP::config.showTeammates);
                    ImGui::Spacing();
                }
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::Text("Other");
                ImGui::Checkbox("Flip Background Image", &Menu::flipBackground);
                ImGui::Checkbox("Bunny Hop", &Bhop::config.enabled);
                ImGui::Checkbox("Bullet Tracers", &BulletTracer::config.enabled);
                ImGui::Checkbox("Third Person Camera", &thirdPerson);    
                const char* tpKeys[] = { "V", "C", "X", "Mouse4", "Mouse5" };
                int tpKeyIdx = 0;
                if (thirdPersonToggleKey == 'C') tpKeyIdx = 1;
                else if (thirdPersonToggleKey == 'X') tpKeyIdx = 2;
                else if (thirdPersonToggleKey == VK_XBUTTON1) tpKeyIdx = 3;
                else if (thirdPersonToggleKey == VK_XBUTTON2) tpKeyIdx = 4;
                if (ImGui::Combo("Thirdperson Toggle Key", &tpKeyIdx, tpKeys, IM_ARRAYSIZE(tpKeys)))
                {
                    const int keyMap[] = { 'V', 'C', 'X', VK_XBUTTON1, VK_XBUTTON2 };
                    thirdPersonToggleKey = keyMap[tpKeyIdx];
                }
                
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::Text("Extras");
                ImGui::Checkbox("Bomb Timer", &ESP::config.bBombTimer);
                ImGui::Checkbox("Spectator List", &ESP::config.bSpectators);
                ImGui::Checkbox("Glow ESP", &ESP::config.bGlow);
                if (ESP::config.bGlow)
                {
                    ImGui::ColorEdit4("Glow Color", ESP::config.glowColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                }

                ImGui::EndChild();
                
                ImGui::EndTabItem();
            }

            // --------------- SKINS TAB ---------------
            if (ImGui::BeginTabItem("Skins"))
            {
                ImGui::Spacing();
                ImGui::Columns(2, "skinsLayout", false);
                
                // Left Column
                ImGui::BeginChild("WeaponsLeft", ImVec2(0, 0), true);
                ImGui::Text("Inventory");
                ImGui::Separator();
                ImGui::Spacing();
                
                if (ImGui::Button("Force Update", ImVec2(-1, 0))) {
                    forceUpdate.store(true);
                }
                if (ImGui::Button("Clear All Skins", ImVec2(-1, 0))) {
                    std::lock_guard<std::mutex> lock(configMutex);
                    weaponSkins.clear();
                    forceUpdate.store(true);
                }
                if (ImGui::Button("Randomize All", ImVec2(-1, 0))) {
                    std::lock_guard<std::mutex> lock(configMutex);
                    if (!g_PaintKits.empty()) {
                        srand((unsigned int)time(NULL));
                        for (int i = 0; i < weaponListCount; i++) {
                            int defIdx = weaponList[i].defIndex;
                            SkinConfig& cfg = weaponSkins[defIdx];
                            cfg.enabled = true;
                            int randomIdx = rand() % g_PaintKits.size();
                            cfg.paintKit = g_PaintKits[randomIdx].id;
                            cfg.wear = 0.001f;
                            cfg.seed = 0;
                            cfg.statTrak = -1;
                        }
                        forceUpdate.store(true);
                    }
                }
                
                ImGui::Spacing();
                for (int i = 0; i < weaponListCount; i++)
                {
                    bool hasConfig = weaponSkins.count(weaponList[i].defIndex) > 0 && weaponSkins[weaponList[i].defIndex].enabled;
                    if (hasConfig) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.9f, 0.5f, 1.0f));
                    
                    if (ImGui::Selectable(weaponList[i].name, selectedWeaponIdx == i))
                        selectedWeaponIdx = i;
                        
                    if (hasConfig) ImGui::PopStyleColor();
                }
                ImGui::EndChild();
                
                ImGui::NextColumn();
                
                // Right Column
                ImGui::BeginChild("WeaponsRight", ImVec2(0, 0), true);
                if (selectedWeaponIdx >= 0 && selectedWeaponIdx < weaponListCount)
                {
                    int defIdx = weaponList[selectedWeaponIdx].defIndex;
                    SkinConfig& cfg = GetSkinConfig(defIdx);
                    
                    ImGui::Text("Configure %s", weaponList[selectedWeaponIdx].name);
                    ImGui::Separator();
                    ImGui::Spacing();
                    
                    ImGui::Checkbox("Enabled", &cfg.enabled);
                    
                    // Advanced Skin Selector
                    static int selectedRarity = 0; // 0 = All
                    const char* rarityNames[] = { "All", "Consumer (Gray)", "Industrial (Light Blue)", "Mil-Spec (Blue)", "Restricted (Purple)", "Classified (Pink)", "Covert (Red)", "Contraband (Gold)" };
                    ImGui::Combo("Filter Rarity", &selectedRarity, rarityNames, IM_ARRAYSIZE(rarityNames));
                    
                    std::string previewName = "Custom ID: " + std::to_string(cfg.paintKit);
                    for (const auto& pk : g_PaintKits) {
                        if (pk.id == cfg.paintKit) {
                            previewName = pk.name;
                            break;
                        }
                    }
                    
                    if (ImGui::BeginCombo("Paint Kit", previewName.c_str())) {
                        bool customSelected = false;
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
                        ImGui::Selectable("--- Custom ID ---", customSelected);
                        ImGui::PopStyleColor();
                        
                        for (const auto& pk : g_PaintKits) {
                            if (selectedRarity != 0 && (int)pk.rarity != selectedRarity - 1)
                                continue;
                                
                            bool isSelected = (cfg.paintKit == pk.id);
                            ImGui::PushStyleColor(ImGuiCol_Text, GetRarityColor(pk.rarity));
                            if (ImGui::Selectable(pk.name, isSelected)) {
                                cfg.paintKit = pk.id;
                            }
                            ImGui::PopStyleColor();
                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                    
                    ImGui::InputInt("Custom Paint Kit ID", &cfg.paintKit);
                    if (cfg.paintKit < 0) cfg.paintKit = 0;
                    
                    ImGui::SliderFloat("Wear", &cfg.wear, 0.0f, 1.0f, "%.4f");
                    ImGui::InputInt("Seed", &cfg.seed);
                    
                    bool useStatTrak = cfg.statTrak >= 0;
                    if (ImGui::Checkbox("StatTrak", &useStatTrak)) cfg.statTrak = useStatTrak ? 0 : -1;
                    if (useStatTrak) {
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(100);
                        ImGui::InputInt("##statval", &cfg.statTrak);
                    }
                }
                ImGui::EndChild();
                
                ImGui::Columns(1);
                ImGui::EndTabItem();
            }

            // --------------- SETTINGS TAB ---------------
            if (ImGui::BeginTabItem("Settings"))
            {
                ImGui::Spacing();
                ImGui::BeginChild("SettingsMain", ImVec2(0, 0), true);
                ImGui::Text("Config");
                ImGui::Spacing();

                ImGui::InputText("Config Name", ConfigSystem::configName, IM_ARRAYSIZE(ConfigSystem::configName));
                if (ImGui::Button("Save Config", ImVec2(160, 0)))
                    ConfigSystem::Save();
                ImGui::SameLine();
                if (ImGui::Button("Load Config", ImVec2(160, 0)))
                    ConfigSystem::Load();
                ImGui::SameLine();
                if (ImGui::Button("Refresh", ImVec2(120, 0)))
                    ConfigSystem::RefreshConfigList();

                if (ConfigSystem::configList.empty())
                    ConfigSystem::RefreshConfigList();

                const char* currentCfg = ConfigSystem::selectedConfigIndex >= 0
                    ? ConfigSystem::configList[ConfigSystem::selectedConfigIndex].c_str()
                    : "<no configs found>";
                if (ImGui::BeginCombo("Saved Configs", currentCfg))
                {
                    for (int i = 0; i < static_cast<int>(ConfigSystem::configList.size()); ++i)
                    {
                        bool selected = (ConfigSystem::selectedConfigIndex == i);
                        if (ImGui::Selectable(ConfigSystem::configList[i].c_str(), selected))
                        {
                            ConfigSystem::selectedConfigIndex = i;
                            strncpy_s(ConfigSystem::configName, ConfigSystem::configList[i].c_str(), _TRUNCATE);
                        }
                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                ImGui::Spacing();
                // Keep settings page minimal: no path/status text noise.
                ImGui::Spacing();

                if (ImGui::Button("Apply Balanced Legit Preset", ImVec2(260, 0)))
                {
                    Aimbot::config.fovType = 0;
                    Aimbot::config.fov = 3.5f;
                    Aimbot::config.legitMode = true;
                    Aimbot::config.legitSmooth = 10.5f;
                    Aimbot::config.legitMaxStep = 1.2f;
                    Aimbot::config.legitReactionMs = 75;
                    Aimbot::config.legitJitter = 0.06f;
                    ConfigSystem::statusMessage = "Applied balanced legit preset";
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(); // Inner padding

        ImGui::PopStyleVar(); // WindowPadding (0,0)
        ImGui::End();
    }
}
