#pragma once
#include <Windows.h>
#include <cstdint>
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <cstdlib>
#include "game.h"
#include "offsets.h"
#include "bullet_tracer.h"
#include "bhop.h"
#include "triggerbot.h"
#include "minhook-master/include/MinHook.h"

// ============================================
// CS2 Silent Aim — CreateMove signature hook
// Hooks CCSGOInput::CreateMove via direct signature scan in client.dll.
//
// How it works:
// 1. BEFORE calling original CreateMove: overwrite dwViewAngles with aim angle
// 2. Original CreateMove reads those angles → builds CUserCmd with aim angles → server gets them
// 3. AFTER original returns: restore dwViewAngles → visual camera stays unchanged
// ============================================

namespace Aimbot
{
    using Game::Vector3;
    using Game::QAngle;

    struct AimbotConfig
    {
        bool  enabled    = false;
        int   fovType    = 0;        // 0 = Angle (Degrees), 1 = Screen (Pixels)
        float fov        = 5.0f;     // For Angle FOV
        float screenFov  = 100.0f;   // For Screen FOV
        int   targetBone = 6;        // 6=head
        int   aimKey     = 0;        // 0=auto (Mouse1)
        bool  autoShoot  = false;
        bool  silentAim  = true;
        bool  teamCheck  = true;     // skip same-team (disable for FFA/DM)
        bool  targetTeammates = false; // optional for DM/custom modes
        bool  visCheck   = false;    // prefer visible targets (uses m_bSpotted, may be unreliable)
        bool  legitMode  = false;    // humanized aim assist
        float legitSmooth = 8.0f;    // larger = slower smoothing
        float legitMaxStep = 1.8f;   // max angle step per tick
        int   legitReactionMs = 60;  // human-like reaction delay
        float legitJitter = 0.08f;   // micro randomness for natural feel
    };

    inline AimbotConfig config;

    // ===== Debug =====
    inline bool debugEnabled     = true;
    inline int  debugTickCounter = 0;
    inline float drawFovRadius   = 0.0f; // Stores active FOV radius for drawing

    // ===== Render FOV Circle =====
    inline void RenderFOV()
    {
        if (!config.enabled) return;
        
        ImDrawList* draw = ImGui::GetBackgroundDrawList();
        if (!draw) return;
        
        ImVec2 center = ImGui::GetIO().DisplaySize;
        center.x *= 0.5f;
        center.y *= 0.5f;

        if (config.fovType == 1) // Screen FOV
        {
            draw->AddCircle(center, config.screenFov, IM_COL32(255, 255, 255, 60), 64, 1.0f);
        }
        else if (config.fovType == 0) // Angle FOV visualization approximation (for active weapon)
        {
            if (drawFovRadius > 0.0f) {
                draw->AddCircle(center, drawFovRadius, IM_COL32(255, 255, 255, 60), 64, 1.0f);
            }
        }
    }

    // ===== Constants =====
    constexpr float PI      = 3.14159265358979323846f;
    constexpr float RAD2DEG = 180.0f / PI;

    // ===== Angle Math =====
    inline void ClampAngles(QAngle& a)
    {
        if (a.pitch >  89.f) a.pitch =  89.f;
        if (a.pitch < -89.f) a.pitch = -89.f;
        while (a.yaw >  180.f) a.yaw -= 360.f;
        while (a.yaw < -180.f) a.yaw += 360.f;
        a.roll = 0.f;
    }

    inline QAngle CalcAngle(const Vector3& src, const Vector3& dst)
    {
        Vector3 d = dst - src;
        float hyp = d.Length2D();
        QAngle a;
        a.pitch = -atan2f(d.z, hyp) * RAD2DEG;
        a.yaw   =  atan2f(d.y, d.x) * RAD2DEG;
        a.roll  = 0.f;
        ClampAngles(a);
        return a;
    }

    inline float GetFOV(const QAngle& view, const QAngle& aim)
    {
        float dp = aim.pitch - view.pitch;
        float dy = aim.yaw - view.yaw;
        while (dy >  180.f) dy -= 360.f;
        while (dy < -180.f) dy += 360.f;
        return sqrtf(dp * dp + dy * dy);
    }

    inline float NormalizeYawDelta(float d)
    {
        while (d > 180.f) d -= 360.f;
        while (d < -180.f) d += 360.f;
        return d;
    }

    inline float RandUnit()
    {
        return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }

    inline QAngle ComputeLegitAngleStep(const QAngle& current, const QAngle& target)
    {
        float dPitch = target.pitch - current.pitch;
        float dYaw = NormalizeYawDelta(target.yaw - current.yaw);

        float smooth = std::clamp(config.legitSmooth, 1.0f, 30.0f);
        float maxStep = std::clamp(config.legitMaxStep, 0.1f, 8.0f);
        float jitter = std::clamp(config.legitJitter, 0.0f, 0.4f);

        // Gradual movement toward target with speed clamp.
        float stepPitch = dPitch / smooth;
        float stepYaw = dYaw / smooth;
        stepPitch = std::clamp(stepPitch, -maxStep, maxStep);
        stepYaw = std::clamp(stepYaw, -maxStep, maxStep);

        // Add tiny randomized micro-correction to reduce robotic movement.
        stepPitch += (RandUnit() - 0.5f) * jitter;
        stepYaw += (RandUnit() - 0.5f) * jitter;

        QAngle out{
            current.pitch + stepPitch,
            current.yaw + stepYaw,
            0.0f
        };
        ClampAngles(out);
        return out;
    }

    inline float GetScreenDistance(const Vector3& targetPos)
    {
        float outX, outY;
        float pos[3] = { targetPos.x, targetPos.y, targetPos.z };
        ImVec2 size = ImGui::GetIO().DisplaySize;
        if (Game::WorldToScreen(pos, outX, outY, size.x, size.y))
        {
            float dx = outX - (size.x * 0.5f);
            float dy = outY - (size.y * 0.5f);
            return sqrtf(dx * dx + dy * dy);
        }
        return FLT_MAX;
    }

    // ===== Visibility Check — Raycast =====
    // Uses the game's built-in TraceShape to check line of sight
    
    // Offsets discovered via IDA reverse engineering:
    constexpr std::ptrdiff_t TRACE_MANAGER_OFF   = 0x203C928;  // global CGameTraceManager*
    constexpr std::ptrdiff_t CTRACE_FILTER_VTBL   = 0x1928FB8;  // CTraceFilter::vftable
    constexpr std::ptrdiff_t TRACE_SHAPE_OFF      = 0x98F3F0;   // TraceShape function

    // Fallback: m_bSpotted offsets
    constexpr std::ptrdiff_t m_entitySpottedState = 0x1F58;
    constexpr std::ptrdiff_t m_bSpotted_off       = 0x8;

    // TraceShape function signature:
    // bool TraceShape(CGameTraceManager* mgr, __int64 shapeConfig, Vector3* start, Vector3* end, CTraceFilter* filter, GameTrace* result)
    using TraceShapeFn = bool(__fastcall*)(void* mgr, void* shapeConfig, float* start, float* end, void* filter, void* result);

    inline bool IsVisibleRaycast(const Vector3& eyePos, const Vector3& targetPos, uintptr_t localPawn, uintptr_t targetPawn, bool doDebug)
    {
        if (!Game::clientBase) return true; // assume visible if can't check

        // Get trace manager pointer
        void** pTraceMgr = reinterpret_cast<void**>(Game::clientBase + TRACE_MANAGER_OFF);
        void* traceMgr = *pTraceMgr;
        if (!traceMgr)
        {
            if (doDebug) printf("[Trace DBG] traceMgr is NULL\n");
            return true; // fallback: assume visible
        }

        // Get function pointer
        auto fnTraceShape = reinterpret_cast<TraceShapeFn>(Game::clientBase + TRACE_SHAPE_OFF);

        // Shape config (16 bytes, zero = point trace / ray)
        __declspec(align(16)) uint8_t shapeConfig[16] = {};

        // Setup CTraceFilter (from CS2 decompilation pattern)
        __declspec(align(16)) uint8_t filter[72] = {};
        *reinterpret_cast<uintptr_t*>(&filter[0]) = Game::clientBase + CTRACE_FILTER_VTBL; // vtable
        
        // Offset 0x8: collision mask (0x1C3003 = MASK_SHOT)
        *reinterpret_cast<uint64_t*>(&filter[0x8]) = 0x1C3003; 

        // Offset 0x10 - 0x1F: zeros
        *reinterpret_cast<uint64_t*>(&filter[0x10]) = 0;
        *reinterpret_cast<uint64_t*>(&filter[0x18]) = 0;

        // Skip handles (using -1 to not skip anything initially)
        *reinterpret_cast<int32_t*>(&filter[0x20]) = -1;
        *reinterpret_cast<int32_t*>(&filter[0x24]) = -1;
        *reinterpret_cast<int32_t*>(&filter[0x28]) = -1;
        *reinterpret_cast<int32_t*>(&filter[0x2C]) = -1;
        *reinterpret_cast<int16_t*>(&filter[0x30]) = 0;
        
        // Magic values from decompiled trace functions
        *reinterpret_cast<int32_t*>(&filter[0x32]) = -65536; // 0xFFFF0000
        *reinterpret_cast<int16_t*>(&filter[0x36]) = 3840;   // 0x0F00
        *reinterpret_cast<uintptr_t*>(&filter[0x38]) = localPawn;    // CS2 usually sets local pawn pointer here
        filter[0x40] = 4;    // some flag
        filter[0x41] = 0x49; // some flag
        filter[0x48] = 0;    // some flag

        // Game trace result (256 bytes should be enough)
        __declspec(align(16)) uint8_t traceResult[256] = {};

        // Prepare start and end as float[3]
        float start[3] = { eyePos.x, eyePos.y, eyePos.z };
        float end[3]   = { targetPos.x, targetPos.y, targetPos.z };

        // Call TraceShape
        bool hit = fnTraceShape(traceMgr, shapeConfig, start, end, filter, traceResult);

        // Check trace fraction at offset +172 (float)
        float fraction = *reinterpret_cast<float*>(&traceResult[172]);

        // Offset 0x90 (144) usually points to the CBaseEntity that was hit
        uintptr_t hitEntity = *reinterpret_cast<uintptr_t*>(&traceResult[144]);

        static int traceDbgCounter = 0;
        if (traceDbgCounter++ % 64 == 0)
        {
            printf("[Trace DBG] Start(%.1f,%.1f,%.1f) End(%.1f,%.1f,%.1f) Hit=%d Fraction=%.3f HitEnt=0x%IX\n",
                eyePos.x, eyePos.y, eyePos.z, targetPos.x, targetPos.y, targetPos.z, hit, fraction, hitEntity);
        }

        // If fraction is close to 1.0, it didn't hit anything before the target
        // Alternatively, if it explicitly hit the target pawn, it's visible.
        return fraction >= 0.97f || hitEntity == targetPawn;
    }

    // Fallback visibility using m_bSpotted (less reliable)
    inline bool IsVisibleSpotted(uintptr_t pawn)
    {
        uint8_t spotted = Game::Read<uint8_t>(pawn + m_entitySpottedState + m_bSpotted_off);
        return spotted != 0;
    }

    inline bool IsVisible(uintptr_t pawn)
    {
        // Use m_bSpotted as fallback
        return IsVisibleSpotted(pawn);
    }

    // Removed GetEntityOrigin and GetBonePosition (moved to game.h)

    inline Vector3 GetEyePosition(uintptr_t pawn)
    {
        return Game::GetEntityOrigin(pawn) + Game::Read<Vector3>(pawn + Offsets::m_vecViewOffset);
    }

    // ===== Target Selection =====
    struct AimTarget
    {
        uintptr_t pawn = 0;
        Vector3   pos;
        QAngle    angle;
        float     fov = FLT_MAX;
    };

    inline AimTarget GetBestTarget(const Vector3& eye, const QAngle& viewAngles, bool doDebug)
    {
        AimTarget best;
        float bestScore = FLT_MAX;

        uintptr_t localCtrl = Game::Read<uintptr_t>(Game::clientBase + Offsets::dwLocalPlayerController);
        if (!localCtrl)
        {
            if (doDebug) printf("[Aim DBG] No local controller\n");
            return best;
        }

        uint32_t localHandle = Game::Read<uint32_t>(localCtrl + Offsets::m_hPlayerPawn);
        uintptr_t localPawn = Game::GetEntityByHandle(localHandle);
        if (!localPawn)
        {
            if (doDebug) printf("[Aim DBG] No local pawn\n");
            return best;
        }
        int localTeam = Game::Read<uint8_t>(localPawn + Offsets::m_iTeamNum);
        if (doDebug) printf("[Aim DBG] LocalPawn=0x%IX team=%d teamCheck=%d visCheck=%d\n",
            localPawn, localTeam, config.teamCheck, config.visCheck);

        uintptr_t entList = Game::Read<uintptr_t>(Game::clientBase + Offsets::dwEntityList);
        if (!entList)
        {
            if (doDebug) printf("[Aim DBG] Entity list null\n");
            return best;
        }

        int enemiesFound = 0;

        for (int i = 1; i <= 64; ++i)
        {
            uint32_t chunkIndex = i >> 9;
            uint32_t entryIndex = i & 0x1FF;

            uintptr_t chunkAddr = entList + 0x8 * chunkIndex + 0x10;
            uintptr_t listEntry = Game::Read<uintptr_t>(chunkAddr);
            if (!listEntry) continue;

            uintptr_t ctrl = Game::Read<uintptr_t>(listEntry + Game::ENTITY_IDENTITY_SIZE * entryIndex);
            if (!ctrl) continue;

            uint32_t pH = Game::Read<uint32_t>(ctrl + Offsets::m_hPlayerPawn);
            if (!pH || pH == 0xFFFFFFFF) continue;

            uintptr_t pawn = Game::GetEntityByHandle(pH);
            if (!pawn || pawn == localPawn) continue;

            // Alive check
            int health = Game::Read<int32_t>(pawn + Offsets::m_iHealth);
            uint8_t lifeState = Game::Read<uint8_t>(pawn + Offsets::m_lifeState);
            if (health <= 0 || lifeState != 0) continue;

            // Team check (can be disabled for FFA/DM)
            int team = Game::Read<uint8_t>(pawn + Offsets::m_iTeamNum);
            if (config.teamCheck && team == localTeam && !config.targetTeammates) continue;

            enemiesFound++;

            // Get target bone position
            Vector3 bonePos = Game::GetBonePosition(pawn, config.targetBone);
            if (bonePos.IsZero()) continue;

            QAngle aimAng = CalcAngle(eye, bonePos);
            
            float fovVal = FLT_MAX;
            if (config.fovType == 0) // Angle
            {
                fovVal = GetFOV(viewAngles, aimAng);
                if (fovVal > config.fov) continue;
            }
            else // Screen
            {
                fovVal = GetScreenDistance(bonePos);
                if (fovVal > config.screenFov) continue;
            }

            // Calculate distance
            Vector3 delta = bonePos - eye;
            float dist = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);

            // Visibility check using raycast
            bool visible = true;
            if (config.visCheck)
                visible = IsVisibleRaycast(eye, bonePos, localPawn, pawn, doDebug);

            if (doDebug && enemiesFound <= 5)
            {
                printf("[Aim DBG] Enemy %d: hp=%d team=%d fov=%.1f dist=%.0f vis=%d\n",
                    i, health, team, fovVal, dist, visible);
            }

            // Immediately skip if visibility check fails
            if (config.visCheck && !visible)
                continue;

            // Weighted score: lower = better
            // FOV is primary factor, distance is secondary
            float score = fovVal + (dist * 0.01f);

            if (score < bestScore)
            {
                bestScore  = score;
                best.pawn  = pawn;
                best.pos   = bonePos;
                best.angle = aimAng;
                best.fov   = fovVal;
            }
        }

        if (doDebug) printf("[Aim DBG] Enemies: %d, best fov: %.1f\n",
            enemiesFound, best.fov == FLT_MAX ? -1.f : best.fov);

        return best;
    }

    // ===== CreateMove Hook (MinHook) =====
    // Signature: 48 8B C4 4C 89 40 18 48 89 48 08 55 53 41 54 41 55
    // Found at sub_180C69B80 in client.dll (IDA)
    // a1 = CCSGOInput* this, a2 = slot, a3 = command context
    using CreateMoveFn = double(__fastcall*)(__int64 a1, unsigned int a2, __int64 a3);
    inline CreateMoveFn oCreateMove = nullptr;

    // Frame history layout (from IDA decompilation of sub_180C64BB0):
    // CCSGOInput + 0xBC8 = frame history count (int)
    // CCSGOInput + 0xBD0 = frame history array ptr
    // Each entry is 96 bytes (0x60), with:
    //   DWORD[0] = tick count
    //   DWORD[4] = pitch (float, offset +0x10)
    //   DWORD[5] = yaw   (float, offset +0x14) 
    //   DWORD[6] = roll  (float, offset +0x18)
    constexpr int FRAME_HISTORY_COUNT_OFF = 0xBC8;
    constexpr int FRAME_HISTORY_ARRAY_OFF = 0xBD0;
    constexpr int FRAME_ENTRY_SIZE        = 96;   // 0x60
    constexpr int PITCH_OFF_IN_ENTRY      = 0x10; // DWORD[4]
    constexpr int YAW_OFF_IN_ENTRY        = 0x14; // DWORD[5]

    inline double ExecuteCreateMoveWithTracing(__int64 a1, unsigned int a2, __int64 a3, bool shotFired, const Vector3& eyePos, float& outPitch, float& outYaw, QAngle& viewAngles, uintptr_t localPawn, bool doDebug)
    {
        float bulletPitch = viewAngles.pitch;
        float bulletYaw   = viewAngles.yaw;

        // === AIMBOT LOGIC ===
        if (!config.enabled)
        {
            outPitch = bulletPitch;
            outYaw   = bulletYaw;
            return oCreateMove(a1, a2, a3);
        }

        // Check aim key
        bool shouldAim = false;
        if (config.aimKey == 0)
            shouldAim = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        else
            shouldAim = (GetAsyncKeyState(config.aimKey) & 0x8000) != 0;

        if (!shouldAim || !localPawn || eyePos.IsZero())
        {
            outPitch = bulletPitch;
            outYaw   = bulletYaw;
            return oCreateMove(a1, a2, a3);
        }

        if (doDebug)
            printf("[Aim] Aiming: eye=(%.1f,%.1f,%.1f) view=(%.1f,%.1f)\n",
                eyePos.x, eyePos.y, eyePos.z, viewAngles.pitch, viewAngles.yaw);

        // Find best target
        AimTarget target = GetBestTarget(eyePos, viewAngles, doDebug);
        if (target.pawn == 0)
        {
            outPitch = bulletPitch;
            outYaw   = bulletYaw;
            return oCreateMove(a1, a2, a3);
        }

        if (doDebug)
            printf("[Aim] TARGET FOUND: fov=%.1f aim=(%.1f,%.1f)\n",
                target.fov, target.angle.pitch, target.angle.yaw);

        // The aimbot will override bullet direction
        bulletPitch = target.angle.pitch;
        bulletYaw   = target.angle.yaw;
        
        outPitch = bulletPitch;
        outYaw   = bulletYaw;

        // If no target, just return
        if (target.pawn == 0)
        {
            return oCreateMove(a1, a2, a3);
        }

        // === LEGIT MODE (humanized visible assist) ===
        if (config.legitMode)
        {
            static ULONGLONG lastAcquireMs = 0;
            static uintptr_t lastTargetPawn = 0;
            ULONGLONG nowMs = GetTickCount64();

            if (lastTargetPawn != target.pawn)
            {
                lastTargetPawn = target.pawn;
                lastAcquireMs = nowMs;
            }
            int reactionMs = config.legitReactionMs < 0 ? 0 : config.legitReactionMs;
            if ((nowMs - lastAcquireMs) >= static_cast<ULONGLONG>(reactionMs))
            {
                QAngle smoothed = ComputeLegitAngleStep(viewAngles, target.angle);
                outPitch = smoothed.pitch;
                outYaw = smoothed.yaw;

                // Apply after CreateMove so game input code doesn't overwrite it in the same tick.
                double result = oCreateMove(a1, a2, a3);
                uintptr_t viewAnglesAddr = Game::clientBase + Offsets::dwViewAngles;
                Game::SafeWrite<QAngle>(viewAnglesAddr, smoothed);
                return result;
            }
            else
            {
                outPitch = viewAngles.pitch;
                outYaw = viewAngles.yaw;
            }

            // Keep normal command path untouched.
            return oCreateMove(a1, a2, a3);
        }

        // === SILENT AIM ===
        if (config.silentAim)
        {
            // Read frame history from CCSGOInput (a1)
            int entryCount = *reinterpret_cast<int*>(a1 + FRAME_HISTORY_COUNT_OFF);
            uintptr_t entryArray = *reinterpret_cast<uintptr_t*>(a1 + FRAME_HISTORY_ARRAY_OFF);

            if (doDebug)
                printf("[Aim] SILENT: entryCount=%d entryArray=0x%IX\n", entryCount, entryArray);

            // Save original angles and overwrite BEFORE CreateMove
            struct SavedAngles { float pitch; float yaw; };
            SavedAngles saved[64] = {};
            int safeCount = (entryCount > 0 && entryCount <= 64) ? entryCount : 0;

            if (entryArray && safeCount > 0)
            {
                for (int i = 0; i < safeCount; i++)
                {
                    uintptr_t entry = entryArray + FRAME_ENTRY_SIZE * i;
                    float* pPitch = reinterpret_cast<float*>(entry + PITCH_OFF_IN_ENTRY);
                    float* pYaw   = reinterpret_cast<float*>(entry + YAW_OFF_IN_ENTRY);

                    saved[i].pitch = Game::SafeReadFloat(pPitch);
                    saved[i].yaw   = Game::SafeReadFloat(pYaw);

#ifdef _DEBUG
                    if (doDebug && i == 0)
                        printf("[Aim] SILENT: BEFORE CM: entry[0] old=(%.2f,%.2f) -> new=(%.2f,%.2f)\n",
                            saved[i].pitch, saved[i].yaw, target.angle.pitch, target.angle.yaw);
#endif

                    Game::SafeWriteFloat(pPitch, target.angle.pitch);
                    Game::SafeWriteFloat(pYaw,   target.angle.yaw);
                }
            }

            // We're silently aiming, bullet direction overridden

            // Call original CreateMove — it reads our overwritten angles
            double result = oCreateMove(a1, a2, a3);

            // Restore original angles — camera stays unchanged
            if (entryArray && safeCount > 0)
            {
                for (int i = 0; i < safeCount; i++)
                {
                    uintptr_t entry = entryArray + FRAME_ENTRY_SIZE * i;
                    float* pPitch = reinterpret_cast<float*>(entry + PITCH_OFF_IN_ENTRY);
                    float* pYaw   = reinterpret_cast<float*>(entry + YAW_OFF_IN_ENTRY);
                    Game::SafeWriteFloat(pPitch, saved[i].pitch);
                    Game::SafeWriteFloat(pYaw,   saved[i].yaw);
                }
            }

            return result;
        }
        else
        {
            // Visible aimbot — set dwViewAngles (using SafeWrite for anti-detection)
            uintptr_t viewAnglesAddr = Game::clientBase + Offsets::dwViewAngles;
            Game::SafeWrite<QAngle>(viewAnglesAddr, target.angle);
            return oCreateMove(a1, a2, a3);
        }
    }

    inline double __fastcall hkCreateMove(__int64 a1, unsigned int a2, __int64 a3)
    {
        // Debug: confirm hook is running
        debugTickCounter++;
        bool doDebug = debugEnabled && (debugTickCounter % 512 == 1);

        if (doDebug)
            printf("[Aim] CreateMove tick #%d | enabled=%d slot=%u\n",
                debugTickCounter, config.enabled, a2);

        if (!Game::clientBase)
            return oCreateMove(a1, a2, a3);

        // === ALWAYS: read local pawn + eye pos for tracer ===
        uintptr_t localPawn = Game::GetLocalPlayerPawn();
        uintptr_t viewAnglesAddr = Game::clientBase + Offsets::dwViewAngles;
        QAngle viewAngles = Game::Read<QAngle>(viewAnglesAddr);
        Vector3 eyePos = localPawn ? GetEyePosition(localPawn) : Vector3{0,0,0};

        // === ALWAYS: detect shots for bullet tracer (works without aimbot) ===
        // Must happen EXACTLY ONCE at the top before logic executes to properly consume lastShotsFired toggle
        bool shotFired = localPawn && BulletTracer::DetectShot(localPawn);

        // === MISC: Bunny Hop ===
        if (localPawn) Bhop::Tick(localPawn);

        // === MISC: Triggerbot ===
        if (localPawn) Triggerbot::Tick(localPawn);

        // Variables that aimbot will assign to if taking over.
        float finalPitch = viewAngles.pitch;
        float finalYaw = viewAngles.yaw;

        double result = ExecuteCreateMoveWithTracing(a1, a2, a3, shotFired, eyePos, finalPitch, finalYaw, viewAngles, localPawn, doDebug);

        // Fire tracer ONLY exactly once, right before exiting, using whatever angle we eventually ended up executing CM with
        if (shotFired && !eyePos.IsZero())
        {
            BulletTracer::AddTraceFromAngles(
                eyePos.x, eyePos.y, eyePos.z,
                finalPitch, finalYaw);
        }

        return result;
    }

    // ===== Hook Setup (MinHook-based) =====
    inline bool Init()
    {
#ifdef _DEBUG
        printf("[Aimbot] Scanning for CreateMove in client.dll...\n");
        printf("[Aimbot] clientBase: 0x%IX\n", Game::clientBase);
#endif

        // Scan for CreateMove by signature in client.dll
        uintptr_t createMoveAddr = Game::FindPattern(
            L"client.dll", Offsets::sig_CreateMove_client);

        if (!createMoveAddr)
        {
#ifdef _DEBUG
            printf("[Aimbot] FAIL: CreateMove signature not found!\n");
            printf("[Aimbot] Signature: %s\n", Offsets::sig_CreateMove_client);
#endif
            return false;
        }

#ifdef _DEBUG
        printf("[Aimbot] CreateMove found at: 0x%IX\n", createMoveAddr);
#endif

        // Detour with MinHook
        MH_STATUS status = MH_CreateHook(
            reinterpret_cast<void*>(createMoveAddr),
            &hkCreateMove,
            reinterpret_cast<void**>(&oCreateMove));

        if (status != MH_OK)
        {
#ifdef _DEBUG
            printf("[Aimbot] FAIL: MH_CreateHook returned %d\n", (int)status);
#endif
            return false;
        }

        status = MH_EnableHook(reinterpret_cast<void*>(createMoveAddr));
        if (status != MH_OK)
        {
#ifdef _DEBUG
            printf("[Aimbot] FAIL: MH_EnableHook returned %d\n", (int)status);
#endif
            return false;
        }

#ifdef _DEBUG
        printf("[Aimbot] CreateMove hooked successfully!\n");
#endif
        return true;
    }

    inline void Shutdown()
    {
        MH_DisableHook(MH_ALL_HOOKS);
#ifdef _DEBUG
        printf("[Aimbot] Shutdown\n");
#endif
    }
}
