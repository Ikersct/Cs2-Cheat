#pragma once
#include <Windows.h>
#include <Psapi.h>
#include <cstdint>
#include <map>
#include <set>
#include <vector>
#include <mutex>
#include <atomic>
#include <cstdio>
#include "offsets.h"
#include "game.h"

// ============================================
// Skin Changer — Working version
// Follows FemboyChanger pattern:
//   attributes + RegenerateWeaponSkins + cleanup
// ============================================

namespace SkinChanger
{
    struct SkinConfig
    {
        int paintKit = 0;
        float wear = 0.001f;
        int seed = 0;
        int statTrak = -1;
        bool enabled = false;
    };



    // Chams config for local player arms
    struct ChamsConfig
    {
        bool enabled = false;
        float color[4] = { 0.0f, 1.0f, 0.4f, 1.0f }; // RGBA (green default)
        bool useGlow = false;
        float glowColor[3] = { 0.0f, 1.0f, 0.4f };    // RGB for glow
        int  renderMode = 0;  // 0=Normal, 1=TransColor, 2=TransAdd, 3=Glow
    };

    #pragma pack(push, 1)
    struct CEconItemAttribute
    {
        uintptr_t vtable;       // 0x00
        uintptr_t owner;        // 0x08
        char pad_0010[32];      // 0x10
        uint16_t defIndex;      // 0x30
        char pad_0032[2];       // 0x32
        float value;            // 0x34
        float initValue;        // 0x38
        int32_t refundableCurrency; // 0x3C
        bool setBonus;          // 0x40
        char pad_0041[7];       // 0x41
    }; // 0x48

    struct CPtrGameVector
    {
        uint64_t size;
        uintptr_t ptr;
    };
    #pragma pack(pop)

    // --- Global State ---
    inline std::map<int, SkinConfig> weaponSkins;

    inline std::atomic<bool> forceUpdate = false;
    inline std::atomic<bool> running = false;
    inline bool thirdPerson = false;  // 3rd person camera toggle
    inline int thirdPersonToggleKey = 'V';
    inline ChamsConfig chamsConfig;   // local player arms chams
    inline uintptr_t regenAddr = 0;
    inline bool regenPatched = false;
    inline std::mutex configMutex;
    inline int tickCounter = 0;

    // Track what's been applied to avoid re-applying
    inline uintptr_t lastAppliedWeapon = 0;
    inline int lastAppliedKit = 0;


    // --- Weapon DefIndex ---
    enum WeaponDefIndex : uint16_t
    {
        WEAPON_KNIFE_CT = 42, WEAPON_KNIFE_T = 59,
    };

    inline const char* GetWeaponName(int d) {
        switch (d) {
        case 1: return "Deagle"; case 4: return "Glock"; case 7: return "AK-47";
        case 8: return "AUG"; case 9: return "AWP"; case 10: return "FAMAS";
        case 16: return "M4A4"; case 19: return "P90"; case 32: return "P2000";
        case 42: return "CT Knife"; case 59: return "T Knife";
        case 60: return "M4A1-S"; case 61: return "USP-S";
        default: return "Weapon";
        }
    }

    // --- Attribute Helpers ---
    inline CEconItemAttribute MakeAttribute(uint16_t def, float value)
    {
        CEconItemAttribute attr{};
        attr.defIndex = def;
        attr.value = value;
        attr.initValue = value;
        return attr;
    }

    // Static attribute buffers — allocated ONCE each, reused every apply.
    inline CEconItemAttribute* g_attrBuffer = nullptr;      // for weapons


    inline void CreateAttributes(uintptr_t item, int paintKit, int seed, float wear)
    {
        if (paintKit <= 0) return;

        uintptr_t attrListAddr = item + Offsets::m_AttributeList + Offsets::m_Attributes;
        CPtrGameVector existing = Game::Read<CPtrGameVector>(attrListAddr);
        if (existing.size > 0 || existing.ptr != 0) return;

        if (!g_attrBuffer)
        {
            g_attrBuffer = reinterpret_cast<CEconItemAttribute*>(
                VirtualAlloc(nullptr, sizeof(CEconItemAttribute) * 3,
                    MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
            if (!g_attrBuffer) return;
            printf("[DBG] Weapon attr buffer at 0x%llX\n", (uintptr_t)g_attrBuffer);
        }

        g_attrBuffer[0] = MakeAttribute(6, static_cast<float>(paintKit));
        g_attrBuffer[1] = MakeAttribute(7, static_cast<float>(seed));
        g_attrBuffer[2] = MakeAttribute(8, wear);

        CPtrGameVector newList;
        newList.size = 3;
        newList.ptr = reinterpret_cast<uintptr_t>(g_attrBuffer);
        Game::Write<CPtrGameVector>(attrListAddr, newList);
    }



    inline void RemoveAttributes(uintptr_t item)
    {
        uintptr_t attrListAddr = item + Offsets::m_AttributeList + Offsets::m_Attributes;
        CPtrGameVector existing = Game::Read<CPtrGameVector>(attrListAddr);
        if (existing.size == 0) return;

        // Zero the vector — the buffer stays valid and allocated
        CPtrGameVector empty{};
        Game::Write<CPtrGameVector>(attrListAddr, empty);
    }

    // --- Init RegenerateWeaponSkins ---
    inline void InitRegen()
    {
        if (regenAddr != 0) return;

        HMODULE clientModule = GetModuleHandleW(L"client.dll");
        if (!clientModule) return;
        MODULEINFO modInfo{};
        if (!GetModuleInformation(GetCurrentProcess(), clientModule, &modInfo, sizeof(modInfo))) return;

        regenAddr = Game::SigScan(
            reinterpret_cast<uintptr_t>(clientModule), modInfo.SizeOfImage,
            "48 83 EC ? E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 8B 10"
        );

        if (regenAddr)
        {
            printf("[+] RegenerateWeaponSkins at 0x%llX\n", regenAddr);

            uint16_t combinedOffset = static_cast<uint16_t>(
                Offsets::m_AttributeManager + Offsets::m_Item +
                Offsets::m_AttributeList + Offsets::m_Attributes
            );

            DWORD oldProtect;
            if (VirtualProtect(reinterpret_cast<void*>(regenAddr + 0x52), 2, PAGE_EXECUTE_READWRITE, &oldProtect))
            {
                *reinterpret_cast<uint16_t*>(regenAddr + 0x52) = combinedOffset;
                VirtualProtect(reinterpret_cast<void*>(regenAddr + 0x52), 2, oldProtect, &oldProtect);
                regenPatched = true;
                printf("[+] Patched +0x52 = 0x%X (VirtualProtect OK)\n", combinedOffset);
            }
            else
                printf("[-] VirtualProtect FAILED\n");
        }
        else
            printf("[-] RegenerateWeaponSkins NOT FOUND\n");
    }

    // --- Call regen DIRECTLY on game thread (not CreateThread!) ---
    // Game's internal functions are NOT thread-safe. Calling from wrong thread corrupts state.
    inline void CallRegen()
    {
        if (!regenAddr || !regenPatched) return;

        printf("[DBG] CallRegen: direct call on game thread at 0x%llX\n", regenAddr);
        __try {
            typedef void(__fastcall* RegenFn)();
            auto fn = reinterpret_cast<RegenFn>(regenAddr);
            fn();
            printf("[DBG] Regen call returned OK\n");
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            printf("[!] RegenerateWeaponSkins CRASHED (caught by SEH)\n");
        }
    }



    // --- Apply skin to a weapon ---
    inline void ApplyAndRegen(uintptr_t weapon, const SkinConfig& skin, uint16_t defIndex)
    {
        uintptr_t item = weapon + Offsets::m_AttributeManager + Offsets::m_Item;

        printf("[SKIN] === APPLY START === weapon=0x%llX def=%d kit=%d\n", weapon, defIndex, skin.paintKit);

        // Save original values so we can restore
        uint32_t origItemIDHigh = Game::Read<uint32_t>(item + Offsets::m_iItemIDHigh);

        printf("[DBG] Step1: writing ItemIDHigh=-1\n");
        Game::Write<uint32_t>(item + Offsets::m_iItemIDHigh, 0xFFFFFFFF);

        printf("[DBG] Step2: writing fallback values\n");
        Game::Write<int32_t>(weapon + Offsets::m_nFallbackPaintKit, skin.paintKit);
        Game::Write<float>(weapon + Offsets::m_flFallbackWear, skin.wear);
        Game::Write<int32_t>(weapon + Offsets::m_nFallbackSeed, skin.seed);
        Game::Write<int32_t>(weapon + Offsets::m_nFallbackStatTrak, skin.statTrak);

        printf("[DBG] Step3: creating attributes\n");
        CreateAttributes(item, skin.paintKit, skin.seed, skin.wear);

        printf("[DBG] Step4: calling regen\n");
        CallRegen();

        printf("[DBG] Step5: FULL RESET — restoring weapon to clean state\n");

        // RESET EVERYTHING — make weapon look untouched to game's destructor
        RemoveAttributes(item);
        Game::Write<uint32_t>(item + Offsets::m_iItemIDHigh, origItemIDHigh);
        Game::Write<int32_t>(weapon + Offsets::m_nFallbackPaintKit, 0);
        Game::Write<float>(weapon + Offsets::m_flFallbackWear, 0.0f);
        Game::Write<int32_t>(weapon + Offsets::m_nFallbackSeed, 0);
        Game::Write<int32_t>(weapon + Offsets::m_nFallbackStatTrak, -1);

        printf("[SKIN] === APPLY DONE (weapon reset to clean state) ===\n");
    }

    // Track apply count for debugging
    inline int applyCount = 0;
    inline int ticksSinceApply = 0;

    // --- Main Tick ---
    inline void TickInner()
    {
        tickCounter++;
        ticksSinceApply++;

        uintptr_t localPawn = Game::GetLocalPlayerPawn();
        if (!localPawn)
        {
            if (lastAppliedWeapon != 0)
                printf("[DBG] Pawn gone, resetting tracking\n");
            lastAppliedWeapon = 0;
            lastAppliedKit = 0;
            return;
        }

        // Check if player is ALIVE
        uint8_t lifeState = Game::Read<uint8_t>(localPawn + 0x35C);
        int32_t health = Game::Read<int32_t>(localPawn + 0x354);

        // Log state changes periodically
        if (tickCounter % 200 == 1)
            printf("[DBG] tick=%d pawn=0x%llX life=%d hp=%d wep=0x%llX kit=%d\n",
                tickCounter, localPawn, lifeState, health, lastAppliedWeapon, lastAppliedKit);

        if (lifeState != 0 || health <= 0)
        {
            if (lastAppliedWeapon != 0)
                printf("[DBG] DEAD: lifeState=%d hp=%d — resetting tracking\n", lifeState, health);
            lastAppliedWeapon = 0;
            lastAppliedKit = 0;
            return;
        }

        InitRegen();

        bool force = forceUpdate.load();

        std::lock_guard<std::mutex> lock(configMutex);

        // ===== WEAPON SKINS =====
        uintptr_t activeWeapon = Game::Read<uintptr_t>(localPawn + Offsets::m_pClippingWeapon);
        if (activeWeapon)
        {
            uintptr_t item = activeWeapon + Offsets::m_AttributeManager + Offsets::m_Item;
            uint16_t defIndex = Game::Read<uint16_t>(item + Offsets::m_iItemDefinitionIndex);

            bool isWeapon = (defIndex > 0 && defIndex < 70) || (defIndex >= 500 && defIndex < 600);
            if (isWeapon && defIndex != 31)
            {
                int lookupIndex = defIndex;
                if (defIndex == WEAPON_KNIFE_CT || defIndex == WEAPON_KNIFE_T)
                {
                    for (auto& [key, cfg] : weaponSkins)
                        if (key >= 500 && key < 600 && cfg.enabled) { lookupIndex = key; break; }
                }

                auto it = weaponSkins.find(lookupIndex);
                if (it != weaponSkins.end() && it->second.enabled && it->second.paintKit > 0)
                {
                    const SkinConfig& skin = it->second;
                    bool needsApply = force
                        || (activeWeapon != lastAppliedWeapon)
                        || (skin.paintKit != lastAppliedKit);

                    if (needsApply)
                    {
                        // Triple-check alive right before modifying
                        lifeState = Game::Read<uint8_t>(localPawn + 0x35C);
                        health = Game::Read<int32_t>(localPawn + 0x354);
                        if (lifeState == 0 && health > 0)
                        {
                            applyCount++;
                            printf("[DBG] Apply #%d: life=%d hp=%d weapon=0x%llX def=%d kit=%d\n",
                                applyCount, lifeState, health, activeWeapon, defIndex, skin.paintKit);

                            Game::Write<uint32_t>(item + Offsets::m_iItemIDHigh, 0);
                            ApplyAndRegen(activeWeapon, skin, defIndex);
                            lastAppliedWeapon = activeWeapon;
                            lastAppliedKit = skin.paintKit;
                            ticksSinceApply = 0;
                        }
                        else
                        {
                            printf("[DBG] Skipped apply — died between checks! life=%d hp=%d\n", lifeState, health);
                        }
                    }
                }
            }
        }

        // ===== THIRD PERSON CAMERA =====
        // NOTE: ThirdPerson write moved to hooks.h hkPresent (end-of-frame)
        // to prevent jitter in online play. The game resets CCSGOInput+0x229
        // every server tick, so we write it after all game logic, right before Present.

        // ===== CHAMS =====
        // NOTE: m_clrRender does NOT work for viewmodels in CS2.
        // Real chams require hooking DrawObject in scenesystem.dll.
        // Implementation pending scenesystem.dll IDA analysis.



        if (force) forceUpdate.store(false);
    }

    // SEH wrapper
    inline void Tick()
    {
        __try {
            TickInner();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            printf("[!] SEH CAUGHT crash in Tick! apply#=%d ticksSince=%d\n", applyCount, ticksSinceApply);
            lastAppliedWeapon = 0;
            lastAppliedKit = 0;
        }
    }
}
