// VECTOR — Phase 2 entry point. Adds AI bots through the unified
// IntentComponent pipeline (roadmap §6.1).
//   * Window + fixed-timestep loop from Phase 0
//   * World owns the ECS + systems
//   * KeyboardMouseInput produces InputIntent each frame (gamepad if plugged)
//   * Renderer interpolates between sim ticks via alpha
//   * AudioSystem reacts to PickupSystem events

#include "audio/audio_system.h"
#include "core/config.h"
#include "core/event_bus.h"
#include "core/logging.h"
#include "core/service_locator.h"
#include "core/time.h"
#include "game/data/tuning.h"
#include "game/components/health.h"
#include "game/components/intent.h"
#include "game/components/inventory.h"
#include "game/components/transform.h"
#include "game/components/physics.h"
#include "game/input/gamepad_input.h"
#include "game/input/keyboard_mouse_input.h"
#include "game/systems/pickup_system.h"
#include "game/world.h"
#include "platform/iplatform.h"
#include "render/camera.h"
#include "render/hud.h"
#include "render/sprite_renderer.h"

#include <raylib.h>

#if VECTOR_ENABLE_IMGUI
#  include <imgui.h>
#  include <rlImGui.h>
#endif

#include <cmath>
#include <cstdio>
#include <memory>

namespace {

constexpr int kInitialWidth  = 1280;
constexpr int kInitialHeight = 720;

void handle_window_resize(vector::render::FollowCamera& cam) {
    if (IsWindowResized()) cam.onWindowResize(GetScreenWidth(), GetScreenHeight());
}

void route_pickup_audio(const std::vector<vector::game::PickupSystem::Event>& events,
                        vector::audio::AudioSystem& audio) {
    using K = vector::game::PickupSystem::Event::Kind;
    for (const auto& e : events) {
        switch (e.kind) {
            case K::Acquired:           audio.play(vector::audio::Cue::PickupAcquired);  break;
            case K::Activated:          audio.play(vector::audio::Cue::PowerUpActivated); break;
            case K::ActivationDenied:   /* no-op (no rejection sfx authored yet) */      break;
            default: break;
        }
    }
}

}  // namespace

int main(int /*argc*/, char** /*argv*/) {
    using namespace vector;

    core::init_logging("vector");
    VECTOR_INFO("VECTOR Phase 2 — boot");

    auto platform = platform::create_platform();
    platform->mountAssets();

    // Tuning data lives in assets/data/tuning.json; defaults if missing.
    const auto tuning = game::Tuning::load(platform->resolveAsset("data/tuning.json"));

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(kInitialWidth, kInitialHeight, "VECTOR — Phase 2");
    SetExitKey(KEY_NULL);
    SetTargetFPS(0);

#if VECTOR_ENABLE_IMGUI
    rlImGuiSetup(true);
#endif

    audio::AudioSystem    audio;
    game::World           world(tuning);
    render::FollowCamera  camera;
    render::SpriteRenderer sprites;
    render::HUD           hud;

    camera.onWindowResize(GetScreenWidth(), GetScreenHeight());

    game::KeyboardMouseInput  kbm;
    game::GamepadInput        gpad;

    core::FixedTimestep clock;
    int                 last_bullet_count = 0;
    bool                show_overlay      = true;
    int                 last_inv_special  = 0;
    bool                last_low_hp_state = false;
    float               heartbeat_timer   = 0.0f;

    bool fire_held_last_tick = false;

    while (!WindowShouldClose()) {
        handle_window_resize(camera);

        const auto& reg = world.registry();
        const auto player = world.player();
        Vector2 player_pos{0, 0};
        Vector2 player_vel{0, 0};
        if (reg.valid(player)) {
            const auto& tr  = reg.get<game::TransformComponent>(player);
            const auto& v   = reg.get<game::VelocityComponent>(player);
            player_pos = tr.position;
            player_vel = v.linear;
        }

        // Poll input: prefer gamepad if connected, otherwise KB/M.
        const game::InputIntent intent = gpad.isAvailable()
            ? gpad.poll(player_pos, camera.raw())
            : kbm.poll(player_pos, camera.raw());

        // Write the player's intent into its IntentComponent so the
        // Movement/Weapon/Pickup systems treat the human exactly like a bot.
        if (auto& mut_reg = world.registry(); mut_reg.valid(player)) {
            mut_reg.get<game::IntentComponent>(player).value = intent;
        }

        if (IsKeyPressed(KEY_F1)) show_overlay = !show_overlay;

        // Fixed-timestep sim — N catch-up ticks per frame.
        const auto step = clock.step();
        for (int i = 0; i < step.sim_ticks; ++i) {
            world.tick(static_cast<float>(core::kSimDt));
        }

        // Audio: fire each time a bullet was actually born this frame.
        const int now_bullets = world.bullet_count();
        if (now_bullets > last_bullet_count) audio.play(audio::Cue::Fire, 0.85f);
        last_bullet_count = now_bullets;
        (void)fire_held_last_tick;

        // Audio: pickup events from this batch of sim ticks.
        route_pickup_audio(world.pickup_events(), audio);

        // Audio: low-HP heartbeat tick.
        bool low_hp = false;
        if (reg.valid(player)) {
            const auto& hp = reg.get<game::HealthComponent>(player);
            low_hp = hp.fraction() > 0.0f && hp.fraction() < 0.25f;
        }
        if (low_hp) {
            heartbeat_timer -= static_cast<float>(step.frame_dt);
            if (heartbeat_timer <= 0.0f) {
                audio.play(audio::Cue::LowHpHeartbeat, 0.7f);
                heartbeat_timer = 0.55f;
            }
        } else {
            heartbeat_timer = 0.0f;
        }
        last_low_hp_state = low_hp;
        (void)last_inv_special;

        // Render.
        camera.update(player_pos, player_vel, static_cast<float>(step.frame_dt));

        BeginDrawing();
        ClearBackground(Color{14, 18, 28, 255});

        BeginMode2D(camera.raw());
        sprites.drawArena(world.arena());
        sprites.drawEntities(const_cast<ecs::Registry&>(reg),
                             static_cast<float>(step.alpha));
        EndMode2D();

        hud.drawScreen(world, GetScreenWidth(), GetScreenHeight());

#if VECTOR_ENABLE_IMGUI
        rlImGuiBegin();
        if (show_overlay) {
            ImGui::SetNextWindowPos(ImVec2(12, 12), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(300, 240), ImGuiCond_FirstUseEver);
            ImGui::Begin("VECTOR debug", &show_overlay);
            ImGui::Text("platform   : %s", platform->name().c_str());
            ImGui::Text("render FPS : %d", GetFPS());
            ImGui::Text("frame dt   : %.2f ms", step.frame_dt * 1000.0);
            ImGui::Text("sim tick   : %llu",
                        static_cast<unsigned long long>(clock.tick()));
            ImGui::Text("ticks/frame: %d", step.sim_ticks);
            ImGui::Text("alpha      : %.3f", step.alpha);
            ImGui::Text("bullets    : %d", world.bullet_count());
            ImGui::Text("bots       : %d", world.bot_count());
            ImGui::Separator();
            if (reg.valid(player)) {
                const auto& hp  = reg.get<game::HealthComponent>(player);
                const auto& inv = reg.get<game::InventoryComponent>(player);
                ImGui::Text("HP   : %.0f / %.0f", hp.current, hp.max);
                ImGui::Text("regen: %s", hp.is_in_combat() ? "no (in combat)" : "yes");
                ImGui::Text("Q    : %s", game::name_of(inv.primary));
                ImGui::Text("E    : %s", game::name_of(inv.special));
            }
            ImGui::Separator();
            ImGui::TextDisabled("WASD/arrows steer  |  Mouse aim  |  LMB fire");
            ImGui::TextDisabled("E activate special  |  Shift boost  |  F1 hide");
            ImGui::End();
        }
        rlImGuiEnd();
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
