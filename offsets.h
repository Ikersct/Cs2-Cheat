#pragma once
#include <cstddef>
#include <cstdint>

// ============================================
// All offsets sourced from project SDK (cs2-dumper 2026-04-18)
// Files: sdk/client_dll.hpp and sdk/offsets.hpp
// ============================================

namespace Offsets
{
    // === Global Offsets (sdk/offsets.hpp - client.dll) ===
    constexpr std::ptrdiff_t dwEntityList                          = 0x24B3268;
    constexpr std::ptrdiff_t dwLocalPlayerController               = 0x22F8028;
    constexpr std::ptrdiff_t dwLocalPlayerPawn                     = 0x206D9E0;
    constexpr std::ptrdiff_t dwPlantedC4                           = 0x231BAB0;
    constexpr std::ptrdiff_t dwViewAngles                          = 0x231E9B8;
    constexpr std::ptrdiff_t dwCSGOInput                           = 0x231E330;
    constexpr std::ptrdiff_t dwGameRules                           = 0x2311ED0;
    constexpr std::ptrdiff_t dwGlobalVars                          = 0x2062540;
    constexpr std::ptrdiff_t dwViewMatrix                          = 0x2313F10;
    constexpr std::ptrdiff_t dwGameEntitySystem_highestEntityIndex  = 0x20A0;

    // === C_BaseEntity (client_dll.hpp:3282+) ===
    constexpr std::ptrdiff_t m_pGameSceneNode  = 0x338;  // CGameSceneNode*
    constexpr std::ptrdiff_t m_hOwnerEntity    = 0x528;  // CHandle<C_BaseEntity>

    // === C_BasePlayerPawn (client_dll.hpp:3427) ===
    constexpr std::ptrdiff_t m_pWeaponServices = 0x13D8; // CPlayer_WeaponServices*

    // === CPlayer_WeaponServices (client_dll.hpp:6828-6829) ===
    constexpr std::ptrdiff_t m_hMyWeapons      = 0x48;   // C_NetworkUtlVectorBase<CHandle<C_BasePlayerWeapon>>
    constexpr std::ptrdiff_t m_hActiveWeapon    = 0x60;   // CHandle<C_BasePlayerWeapon>

    // === CCSPlayerController (client_dll.hpp:3997) ===
    constexpr std::ptrdiff_t m_pInventoryServices = 0x810; // CCSPlayerController_InventoryServices*
    constexpr std::ptrdiff_t m_sSanitizedPlayerName = 0x860; // CUtlString
    constexpr std::ptrdiff_t m_hPlayerPawn        = 0x90C; // CHandle<C_CSPlayerPawn>

    // === CCSPlayerController_InventoryServices (client_dll.hpp:379) ===
    constexpr std::ptrdiff_t m_unMusicID = 0x58; // uint16

    // === C_CSPlayerPawn (client_dll.hpp:2344+) ===
    constexpr std::ptrdiff_t m_bNeedToReApplyGloves = 0x188D; // bool
    constexpr std::ptrdiff_t m_EconGloves           = 0x1890; // C_EconItemView
    constexpr std::ptrdiff_t m_hHudModelArms        = 0x2400; // CHandle<C_CS2HudModelArms>
    constexpr std::ptrdiff_t m_pClippingWeapon      = 0x3DC0; // C_CSWeaponBase*
    constexpr std::ptrdiff_t m_nEconGlovesChanged   = 0x1D00; // uint8

    // === C_BaseModelEntity (client_dll.hpp:3098+) — Render overrides ===
    constexpr std::ptrdiff_t m_nRenderMode            = 0xB60;  // RenderMode_t (uint8)
    constexpr std::ptrdiff_t m_clrRender              = 0xB80;  // Color (RGBA, 4 bytes)
    constexpr std::ptrdiff_t m_Glow                   = 0xCC0;  // CGlowProperty
    constexpr std::ptrdiff_t m_flGlowBackfaceMult     = 0xD18;  // float32
    constexpr std::ptrdiff_t m_ClientOverrideTint     = 0xE40;  // Color (RGBA, 4 bytes)
    constexpr std::ptrdiff_t m_bUseClientOverrideTint = 0xE44;  // bool

    // === CSkeletonInstance (client_dll.hpp:3572) ===
    constexpr std::ptrdiff_t m_materialGroup = 0x434; // CUtlStringToken

    // === CSkeletonInstance (client_dll.hpp:3566) ===
    constexpr std::ptrdiff_t m_modelState = 0x160; // CModelState

    // === CModelState (client_dll.hpp:4596) ===
    constexpr std::ptrdiff_t m_MeshGroupMask = 0x220; // uint64

    // === C_EconEntity (client_dll.hpp:5739+) ===
    constexpr std::ptrdiff_t m_AttributeManager       = 0x1378; // C_AttributeContainer
    constexpr std::ptrdiff_t m_OriginalOwnerXuidLow   = 0x1848; // uint32
    constexpr std::ptrdiff_t m_OriginalOwnerXuidHigh  = 0x184C; // uint32
    constexpr std::ptrdiff_t m_nFallbackPaintKit      = 0x1850; // int32
    constexpr std::ptrdiff_t m_nFallbackSeed          = 0x1854; // int32
    constexpr std::ptrdiff_t m_flFallbackWear         = 0x1858; // float32
    constexpr std::ptrdiff_t m_nFallbackStatTrak      = 0x185C; // int32

    // === C_AttributeContainer (client_dll.hpp:1823) ===
    constexpr std::ptrdiff_t m_Item = 0x50; // C_EconItemView

    // === C_EconItemView (client_dll.hpp:1619+) ===
    constexpr std::ptrdiff_t m_iItemDefinitionIndex               = 0x1BA; // uint16
    constexpr std::ptrdiff_t m_iEntityQuality                     = 0x1BC; // int32
    constexpr std::ptrdiff_t m_iItemIDHigh                        = 0x1D0; // uint32
    constexpr std::ptrdiff_t m_iItemIDLow                         = 0x1D4; // uint32
    constexpr std::ptrdiff_t m_iAccountID                         = 0x1D8; // uint32
    constexpr std::ptrdiff_t m_bInitialized                       = 0x1E8; // bool
    constexpr std::ptrdiff_t m_bRestoreCustomMaterialAfterPrecache = 0x1B8; // bool
    constexpr std::ptrdiff_t m_AttributeList                      = 0x208; // CAttributeList
    constexpr std::ptrdiff_t m_NetworkedDynamicAttributes         = 0x280; // CAttributeList
    constexpr std::ptrdiff_t m_szCustomName                       = 0x2F8; // char[161]
    constexpr std::ptrdiff_t m_szCustomNameOverride               = 0x399; // char[161]

    // === CAttributeList (client_dll.hpp:5066) ===
    constexpr std::ptrdiff_t m_Attributes = 0x8; // C_UtlVectorEmbeddedNetworkVar<CEconItemAttribute>

    // === CEconItemAttribute (client_dll.hpp:182-186) ===
    constexpr std::ptrdiff_t m_iAttributeDefinitionIndex = 0x30; // uint16
    constexpr std::ptrdiff_t m_flValue                   = 0x34; // float32
    constexpr std::ptrdiff_t m_flInitialValue            = 0x38; // float32
    constexpr std::ptrdiff_t m_nRefundableCurrency       = 0x3C; // int32
    constexpr std::ptrdiff_t m_bSetBonus                 = 0x40; // bool

    // === Aimbot / Entity Offsets (client_dll.hpp) ===
    constexpr std::ptrdiff_t m_iHealth       = 0x354;  // C_BaseEntity, int32
    constexpr std::ptrdiff_t m_lifeState     = 0x35C;  // C_BaseEntity, uint8 (0=alive)
    constexpr std::ptrdiff_t m_iTeamNum      = 0x3F3;  // C_BaseEntity, uint8
    constexpr std::ptrdiff_t m_vecAbsOrigin  = 0xD0;   // CGameSceneNode, VectorWS
    constexpr std::ptrdiff_t m_vecViewOffset = 0xD58;  // C_BasePlayerPawn, CNetworkViewOffsetVector
    constexpr std::ptrdiff_t m_BoneArray     = 0x80;   // CModelState (not in SDK, community-confirmed)
    constexpr std::ptrdiff_t m_flSimulationTime = 0x3C0; // C_BaseEntity, float32
    constexpr std::ptrdiff_t m_bPawnIsAlive   = 0x914;  // CCSPlayerController, bool
    constexpr std::ptrdiff_t m_iIDEntIndex    = 0x3EAC; // C_CSPlayerPawnBase, CEntityIndex
    constexpr std::ptrdiff_t m_iShotsFired    = 0x270C; // C_CSPlayerPawn, int32
    constexpr std::ptrdiff_t m_fFlags         = 0x400;  // C_BaseEntity, uint32

    // === Visuals (Bomb, Glow, Spectator) ===
    constexpr std::ptrdiff_t m_bBombTicking       = 0x1170; // C_PlantedC4, bool
    constexpr std::ptrdiff_t m_flC4Blow           = 0x11A0; // C_PlantedC4, GameTime_t
    constexpr std::ptrdiff_t m_flDefuseCountDown  = 0x11C0; // C_PlantedC4, GameTime_t
    constexpr std::ptrdiff_t m_flDefuseLength     = 0x11BC; // C_PlantedC4, float32
    
    constexpr std::ptrdiff_t m_hObserverPawn      = 0x910;  // CCSPlayerController, CHandle<CCSObserverPawn>
    constexpr std::ptrdiff_t m_pObserverServices  = 0x13F0; // C_BasePlayerPawn, CPlayer_ObserverServices*
    constexpr std::ptrdiff_t m_hObserverTarget    = 0x4C;   // CPlayer_ObserverServices, CHandle<C_BaseEntity>
    constexpr std::ptrdiff_t m_iObserverMode      = 0x48;   // CPlayer_ObserverServices, uint8
    
    constexpr std::ptrdiff_t m_pCameraServices    = 0x1410; // C_BasePlayerPawn, CPlayer_CameraServices*

    constexpr std::ptrdiff_t m_bGlowing           = 0x51;   // CGlowProperty, bool
    constexpr std::ptrdiff_t m_glowColorOverride  = 0x40;   // CGlowProperty, Color
    constexpr std::ptrdiff_t m_iGlowType          = 0x30;   // CGlowProperty, int32

    // CCSGOInput vtable index for CreateMove (kept for reference, not used by current hook)
    constexpr int CCSGOInput_CreateMoveIdx = 21;

    // === Signature Patterns ===
    // CreateMove (client.dll) — CCSGOInput::CreateMove, found via IDA at sub_180C69B80
    constexpr const char* sig_CreateMove_client = "48 8B C4 4C 89 40 18 48 89 48 08 55 53 41 54 41 55";

    // Third person sv_cheats bypass (client.dll)
    // Patches jnz -> jmp at +7 to skip reset block
    constexpr const char* sig_ThirdPersonReset = "48 8B 40 08 44 38 20 75 10 44 88 67 01";

    // DrawObject (scenesystem.dll) — main render function for scene objects
    constexpr const char* sig_DrawObject = "48 8B C4 53 57 41 54 48 81 EC D0 00 00 00 49 63 F9 49";

    // LoadKV3 call site (tier0.dll) — resolves LEA rel32 target to get LoadKV3 function
    constexpr const char* sig_LoadKV3 = "48 8D 0D ? ? ? ? FF 15 ? ? ? ? 49 8B 06";

    // CreateMaterial (materialsystem2.dll) — creates CMaterial2 from KV3 data
    constexpr const char* sig_CreateMaterial = "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 8B F1 48 8B DA";
}
