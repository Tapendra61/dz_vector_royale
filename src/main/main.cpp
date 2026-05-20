// VECTOR — Phase 0 entry point.
// Opens a window, runs a fixed-timestep simulation loop decoupled from
// rendering, and shows a debug overlay. No gameplay yet — just the skeleton
// every later phase will hang off of (roadmap §4).

#include "core/config.h"
#include "core/event_bus.h"
#include "core/logging.h"
#include "core/service_locator.h"
#include "core/time.h"
#include "platform/iplatform.h"

#include <raylib.h>

#if VECTOR_ENABLE_IMGUI
#  include <imgui.h>
#  include <rlImGui.h>
#endif

#include <cmath>
#include <cstdio>

namespace {

constexpr int kInitialWidth  = 1280;
constexpr int kInitialHeight = 720;

struct SimState {
    double t        = 0.0;
    float  hue_deg  = 0.0f;  // placeholder: rotates the background tint each tick
};

void simulate(SimState& s) {
    s.t       += vector::core::kSimDt;
    s.hue_deg  = std::fmod(s.hue_deg + 30.0f * static_cast<float>(vector::core::kSimDt), 360.0f);
}

Color tint_from_hue(float hue_deg) {
    return ColorFromHSV(hue_deg, 0.15f, 0.10f);
}

}  // namespace

int main(int /*argc*/, char** /*argv*/) {
    using namespace vector;

    core::init_logging("vector");
    VECTOR_INFO("VECTOR Phase 0 — boot");

    auto platform = platform::create_platform();
    platform->mountAssets();
    VECTOR_INFO("platform: {}", platform->name());

    if (auto cfg = core::Config::load_from_file(platform->resolveAsset("config.json"))) {
        VECTOR_INFO("config: loaded {} keys", cfg->raw().size());
    }

    core::ServiceLocator services;
    core::EventBus       events;
    services.provide(&events);
    services.provide(platform.get());

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(kInitialWidth, kInitialHeight, "VECTOR — Hello");
    SetExitKey(KEY_NULL);
    SetTargetFPS(0);  // we drive timing ourselves

#if VECTOR_ENABLE_IMGUI
    rlImGuiSetup(true);
#endif

    core::FixedTimestep clock;
    SimState            sim;

    bool show_overlay = true;

    while (!WindowShouldClose()) {
        const auto step = clock.step();
        for (int i = 0; i < step.sim_ticks; ++i) simulate(sim);

        if (IsKeyPressed(KEY_F1)) show_overlay = !show_overlay;

        BeginDrawing();
        ClearBackground(tint_from_hue(sim.hue_deg));

        const int   w   = GetScreenWidth();
        const int   h   = GetScreenHeight();
        const char* msg = "Hello, VECTOR";
        const int   fs  = 48;
        const int   tw  = MeasureText(msg, fs);
        DrawText(msg, (w - tw) / 2, (h - fs) / 2, fs, RAYWHITE);

        const char* sub = "Phase 0 — fixed-timestep loop running. F1 toggles overlay.";
        DrawText(sub, (w - MeasureText(sub, 18)) / 2, (h / 2) + fs, 18, GRAY);

#if VECTOR_ENABLE_IMGUI
        rlImGuiBegin();
        if (show_overlay) {
            ImGui::SetNextWindowPos(ImVec2(12, 12), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(280, 180), ImGuiCond_FirstUseEver);
            ImGui::Begin("VECTOR debug", &show_overlay);
            ImGui::Text("platform   : %s", platform->name().c_str());
            ImGui::Text("render FPS : %d", GetFPS());
            ImGui::Text("frame dt   : %.2f ms", step.frame_dt * 1000.0);
            ImGui::Text("sim ticks  : %llu",
                        static_cast<unsigned long long>(clock.tick()));
            ImGui::Text("ticks/frame: %d", step.sim_ticks);
            ImGui::Text("alpha      : %.3f", step.alpha);
            ImGui::Text("sim time   : %.2f s", sim.t);
            ImGui::Separator();
            ImGui::TextDisabled("F1: toggle  |  ESC: quit");
            ImGui::End();
        }
        rlImGuiEnd();
#else
        DrawFPS(12, 12);
#endif

        EndDrawing();
    }

#if VECTOR_ENABLE_IMGUI
    rlImGuiShutdown();
#endif
    CloseWindow();

    VECTOR_INFO("VECTOR shutting down");
    core::shutdown_logging();
    return 0;
}
