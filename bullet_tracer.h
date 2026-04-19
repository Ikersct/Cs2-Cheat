#pragma once
#include <Windows.h>
#include <vector>
#include <mutex>
#include <cmath>
#include "game.h"
#include "offsets.h"
#include "imgui-master/imgui.h"

// ============================================
// Bullet Trace Renderer — Shows ACTUAL bullet path
// Detects shots via m_iShotsFired, computes ray from aim angles,
// draws animated tracer following the real bullet direction
// ============================================

namespace BulletTracer
{
    // ===== Config =====
    struct TracerConfig
    {
        bool  enabled     = true;
        float trailLife   = 2.5f;     // seconds before full fade
        float bulletSpeed = 8000.0f;  // visual bullet travel speed (units/sec) 
        float thickness   = 2.0f;
        float rayLength   = 8000.0f;  // how far the ray extends
    };

    inline TracerConfig config;

    // ViewMatrix and WorldToScreen moved to game.h

    // ===== Math helpers =====
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float DEG2RAD = PI / 180.0f;

    // Convert pitch/yaw angles to a unit direction vector
    inline void AngleToDir(float pitch, float yaw, float out[3])
    {
        float cp = cosf(pitch * DEG2RAD);
        float sp = sinf(pitch * DEG2RAD);
        float cy = cosf(yaw * DEG2RAD);
        float sy = sinf(yaw * DEG2RAD);
        out[0] = cp * cy;   // forward X
        out[1] = cp * sy;   // forward Y
        out[2] = -sp;       // forward Z (pitch up = negative Z in Source)
    }

    // ===== Trace Data =====
    struct Trace
    {
        float startPos[3];  // eye position when shot fired
        float endPos[3];    // computed endpoint (eye + dir * rayLength)
        float spawnTime;
        float totalDist;
    };

    inline std::vector<Trace> traces;
    inline std::mutex traceMutex;
    inline int lastShotsFired = 0; // track m_iShotsFired changes

    inline float GetTime()
    {
        return static_cast<float>(GetTickCount64()) / 1000.0f;
    }

    // Called from CreateMove — adds trace using actual aim angles
    // eyeX/Y/Z = eye position, pitch/yaw = the ACTUAL angles sent to server
    inline void AddTraceFromAngles(float eyeX, float eyeY, float eyeZ,
                                   float pitch, float yaw)
    {
        if (!config.enabled) return;

        float dir[3];
        AngleToDir(pitch, yaw, dir);

        Trace t;
        t.startPos[0] = eyeX;
        t.startPos[1] = eyeY;
        t.startPos[2] = eyeZ;
        t.endPos[0] = eyeX + dir[0] * config.rayLength;
        t.endPos[1] = eyeY + dir[1] * config.rayLength;
        t.endPos[2] = eyeZ + dir[2] * config.rayLength;
        t.spawnTime = GetTime();

        float dx = t.endPos[0] - eyeX;
        float dy = t.endPos[1] - eyeY;
        float dz = t.endPos[2] - eyeZ;
        t.totalDist = sqrtf(dx * dx + dy * dy + dz * dz);

        std::lock_guard<std::mutex> lock(traceMutex);
        traces.push_back(t);
    }

    // Check if player just fired a shot (m_iShotsFired increased)
    // Returns true on the tick where a new shot is detected
    inline bool DetectShot(uintptr_t localPawn)
    {
        if (!localPawn) return false;

        int currentShots = Game::Read<int>(localPawn + Offsets::m_iShotsFired);

        if (currentShots > lastShotsFired && lastShotsFired >= 0)
        {
            lastShotsFired = currentShots;
            return true;
        }
        lastShotsFired = currentShots;
        return false;
    }

    // Helper: lerp along the trace
    inline void LerpPos(const Trace& t, float frac, float out[3])
    {
        out[0] = t.startPos[0] + (t.endPos[0] - t.startPos[0]) * frac;
        out[1] = t.startPos[1] + (t.endPos[1] - t.startPos[1]) * frac;
        out[2] = t.startPos[2] + (t.endPos[2] - t.startPos[2]) * frac;
    }

    // ===== Render (called from Present hook) =====
    inline void Render()
    {
        if (!config.enabled) return;
        if (!Game::clientBase) return;

        float now = GetTime();
        ImDrawList* draw = ImGui::GetBackgroundDrawList();
        if (!draw) return;

        std::lock_guard<std::mutex> lock(traceMutex);

        // Cleanup expired traces
        float maxAge = config.trailLife + 2.0f;
        traces.erase(
            std::remove_if(traces.begin(), traces.end(),
                [now, maxAge](const Trace& t) { return (now - t.spawnTime) > maxAge; }),
            traces.end());

        for (const auto& t : traces)
        {
            float age = now - t.spawnTime;

            // Bullet travel progress (0 → 1)
            float travelTime = t.totalDist / config.bulletSpeed;
            if (travelTime < 0.01f) travelTime = 0.01f;
            float bulletFrac = age / travelTime;
            if (bulletFrac > 1.0f) bulletFrac = 1.0f;

            // Trail fade after bullet arrives
            float trailAge = age - travelTime;
            float trailAlpha = 1.0f;
            if (trailAge > 0.0f)
            {
                trailAlpha = 1.0f - (trailAge / config.trailLife);
                if (trailAlpha <= 0.0f) continue;
                trailAlpha *= trailAlpha; // quadratic fade
            }

            // === Draw segmented trail from start to bullet tip ===
            constexpr int SEGMENTS = 16;
            ImVec2 pts[SEGMENTS + 1];
            bool   ok[SEGMENTS + 1] = {};

            for (int s = 0; s <= SEGMENTS; s++)
            {
                float segFrac = (float)s / (float)SEGMENTS * bulletFrac;
                float pos3d[3];
                LerpPos(t, segFrac, pos3d);
                ok[s] = Game::WorldToScreen(pos3d, pts[s].x, pts[s].y, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
            }

            for (int s = 0; s < SEGMENTS; s++)
            {
                if (!ok[s] || !ok[s + 1]) continue;

                float segFrac = (float)s / (float)SEGMENTS;
                // Gradient: dim tail → bright head
                float brightness = 0.2f + 0.8f * segFrac;
                int alpha = (int)(trailAlpha * brightness * 220.0f);
                if (alpha <= 0) continue;
                if (alpha > 255) alpha = 255;

                // Main white trace line
                ImU32 lineCol = IM_COL32(255, 255, 255, alpha);
                draw->AddLine(pts[s], pts[s + 1], lineCol, config.thickness);

                // Soft glow
                int glowA = (int)(trailAlpha * brightness * 40.0f);
                if (glowA > 255) glowA = 255;
                ImU32 glowCol = IM_COL32(180, 200, 255, glowA);
                draw->AddLine(pts[s], pts[s + 1], glowCol, config.thickness * 3.5f);
            }

            // === Bullet head (bright dot while still flying) ===
            if (bulletFrac < 1.0f)
            {
                float headPos[3];
                LerpPos(t, bulletFrac, headPos);
                ImVec2 headScr;
                if (Game::WorldToScreen(headPos, headScr.x, headScr.y, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y))
                {
                    int ha = (int)(trailAlpha * 255.0f);
                    draw->AddCircleFilled(headScr, 5.0f, IM_COL32(255, 255, 255, ha));
                    draw->AddCircleFilled(headScr, 2.5f, IM_COL32(255, 255, 200, ha));
                }
            }

            // === Impact point (after bullet arrives) ===
            if (bulletFrac >= 1.0f && trailAlpha > 0.05f)
            {
                ImVec2 impScr;
                if (Game::WorldToScreen(t.endPos, impScr.x, impScr.y, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y))
                {
                    float sz = 4.0f * trailAlpha;
                    draw->AddCircleFilled(impScr, sz, IM_COL32(255, 200, 100, (int)(trailAlpha * 200.0f)));
                    draw->AddCircle(impScr, sz * 1.8f, IM_COL32(255, 255, 255, (int)(trailAlpha * 120.0f)), 0, 1.5f);
                }
            }
        }
    }
}
