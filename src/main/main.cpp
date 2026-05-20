// VECTOR — Phase 3 entry point.
//   * Phase 0/1: window + fixed-timestep loop, World owns ECS + systems
//   * Phase 2: AI bots drive ships through the unified IntentComponent
//   * Phase 3: post-FX pipeline (bloom + vignette + low-HP tint), data-driven
//     particle system, trauma-based screen shake, thrust audio rumble.

#include "audio/audio_system.h"
#include "core/config.h"
#include "core/event_bus.h"
#include "core/logging.h"
#include "core/service_locator.h"
#include "core/time.h"
#include "game/data/tuning.h"
#include "game/components/bullet.h"
#include "game/components/health.h"
#include "game/components/intent.h"
#include "game/components/inventory.h"
#include "game/components/pickup.h"
#include "game/components/ship.h"
#include "game/components/transform.h"
#include "game/components/physics.h"
#include "game/components/render_tag.h"
#include "game/input/gamepad_input.h"
#include "game/input/keyboard_mouse_input.h"
#include "game/systems/pickup_system.h"
#include "game/world.h"
#include "platform/iplatform.h"
#include "render/camera.h"
#include "render/graphics_settings.h"
#include "render/hud.h"
#include "render/particles/emitter_spec.h"
#include "render/particles/particle_system.h"
#include "render/post_fx.h"
#include "render/sprite_renderer.h"

#include <raylib.h>
#include <raymath.h>

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

void route_pickup_audio(const std::vector<vector::game::PickupSystem::Event>& events,
                        vector::audio::AudioSystem& audio,
                        vector::render::ParticleSystem& particles,
                        const vector::render::EmitterLibrary& specs,
                        vector::ecs::Registry& reg) {
    using K = vector::game::PickupSystem::Event::Kind;
    for (const auto& e : events) {
        switch (e.kind) {
            case K::Acquired:
                audio.play(vector::audio::Cue::PickupAcquired);
                if (reg.valid(e.ship)) {
                    const auto& tr = reg.get<vector::game::TransformComponent>(e.ship);
                    if (auto* s = specs.find("pickup_sparkle"))
                        particles.burst(*s, tr.position, {0.0f, -1.0f});
                }
                break;
            case K::Activated:
                audio.play(vector::audio::Cue::PowerUpActivated);
                if (reg.valid(e.ship)) {
                    const auto& tr = reg.get<vector::game::TransformComponent>(e.ship);
                    if (auto* s = specs.find("heal_burst"))
                        particles.burst(*s, tr.position, {0.0f, -1.0f});
                }
                break;
            default: break;
        }
    }
}

}  // namespace

int main(int /*argc*/, char** /*argv*/) {
    using namespace vector;

    core::init_logging("vector");
    VECTOR_INFO("VECTOR Phase 3 — boot");

    auto platform = platform::create_platform();
    platform->mountAssets();

    const auto tuning = game::Tuning::load(platform->resolveAsset("data/tuning.json"));

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(kInitialWidth, kInitialHeight, "VECTOR — Phase 3");
    SetExitKey(KEY_NULL);
    SetTargetFPS(0);

#if VECTOR_ENABLE_IMGUI
    rlImGuiSetup(true);
#endif

    audio::AudioSystem     audio;
    game::World            world(tuning);
    render::FollowCamera   camera;
    render::SpriteRenderer sprites;
    render::HUD            hud;

    render::GraphicsSettings gfx = render::GraphicsSettings::for_preset(render::GraphicsPreset::High);
    render::ParticleSystem   particles(8192);
    particles.set_max_active(gfx.particle_cap);

    render::EmitterLibrary specs;
    specs.load(platform->resolveAsset("data/particles.json"));

    render::PostFX postfx;
    postfx.init(GetScreenWidth(), GetScreenHeight(),
                platform->resolveAsset("shaders"));

    camera.onWindowResize(GetScreenWidth(), GetScreenHeight());

    game::KeyboardMouseInput kbm;
    game::GamepadInput       gpad;

    core::FixedTimestep clock;
    int                 last_bullet_count = 0;
    bool                show_overlay      = true;
    float               heartbeat_timer   = 0.0f;
    float               bullet_trail_acc  = 0.0f;
    float               thrust_emit_acc   = 0.0f;

    while (!WindowShouldClose()) {
        if (IsWindowResized()) {
            camera.onWindowResize(GetScreenWidth(), GetScreenHeight());
            postfx.resize(GetScreenWidth(), GetScreenHeight());
        }
        if (IsKeyPressed(KEY_F1)) show_overlay = !show_overlay;
        if (IsKeyPressed(KEY_F2)) {
            // Cycle graphics preset Low → Med → High → Low.
            const auto p = gfx.preset;
            gfx = render::GraphicsSettings::for_preset(
                p == render::GraphicsPreset::Low    ? render::GraphicsPreset::Medium :
                p == render::GraphicsPreset::Medium ? render::GraphicsPreset::High   :
                                                       render::GraphicsPreset::Low);
            particles.set_max_active(gfx.particle_cap);
        }

        auto& reg = world.registry();
        const auto player = world.player();
        Vector2 player_pos{0, 0};
        Vector2 player_vel{0, 0};
        if (reg.valid(player)) {
            player_pos = reg.get<game::TransformComponent>(player).position;
            player_vel = reg.get<game::VelocityComponent>(player).linear;
        }

        const game::InputIntent intent = gpad.isAvailable()
            ? gpad.poll(player_pos, camera.raw())
            : kbm.poll(player_pos, camera.raw());

        if (reg.valid(player)) reg.get<game::IntentComponent>(player).value = intent;

        const auto step = clock.step();
        for (int i = 0; i < step.sim_ticks; ++i) {
            world.tick(static_cast<float>(core::kSimDt));
        }

        // --- Fx routing: convert this frame's gameplay events to VFX/SFX. ---

        // Bullet firing → fire sfx + per-bullet trail emission.
        const int now_bullets = world.bullet_count();
        if (now_bullets > last_bullet_count) audio.play(audio::Cue::Fire, 0.85f);
        last_bullet_count = now_bullets;

        // Continuous trail behind each live bullet.
        if (auto* trail = specs.find("bullet_trail")) {
            reg.view<game::BulletComponent, game::TransformComponent, game::VelocityComponent>().each(
                [&](const game::BulletComponent&, const game::TransformComponent& tr, const game::VelocityComponent& v) {
                    Vector2 dir{-v.linear.x, -v.linear.y};
                    particles.continuous(*trail, tr.position, dir, static_cast<float>(step.frame_dt), bullet_trail_acc);
                });
        }

        // Thrust exhaust behind every ship that's actually thrusting.
        if (auto* exhaust = specs.find("thrust_exhaust")) {
            reg.view<game::ShipComponent, game::IntentComponent, game::TransformComponent>().each(
                [&](const game::ShipComponent&, const game::IntentComponent& ic, const game::TransformComponent& tr) {
                    if (ic.value.thrust <= 0.05f) return;
                    const float    cosr = std::cos(tr.rotation);
                    const float    sinr = std::sin(tr.rotation);
                    const Vector2 tail{tr.position.x - cosr * tuning.ship.radius,
                                        tr.position.y - sinr * tuning.ship.radius};
                    const Vector2 back{-cosr, -sinr};
                    float local_acc = 0.0f;
                    auto spec = *exhaust;
                    spec.rate_per_second *= ic.value.thrust;
                    particles.continuous(spec, tail, back, static_cast<float>(step.frame_dt), thrust_emit_acc);
                    (void)local_acc;
                });
        }

        // Hits → spark + screen shake.
        for (const auto& h : world.hits()) {
            if (auto* s = specs.find("hit_spark")) {
                Vector2 outward{-h.incoming_dir.x, -h.incoming_dir.y};
                if (outward.x == 0.0f && outward.y == 0.0f) outward = {0.0f, -1.0f};
                particles.burst(*s, h.position, outward);
            }
            audio.play(audio::Cue::Hit, 0.5f);
            if (h.target == player) camera.add_trauma(0.45f);
            else                    camera.add_trauma(0.12f);
        }

        // Deaths → explosion + large shake.
        for (const auto& d : world.deaths()) {
            if (!reg.valid(d.entity)) continue;
            const auto& tr = reg.get<game::TransformComponent>(d.entity);
            if (auto* s = specs.find("ship_explosion")) {
                particles.burst(*s, tr.position, {0.0f, -1.0f});
            }
            audio.play(audio::Cue::Explosion, 0.85f);
            camera.add_trauma(d.entity == player ? 0.9f : 0.35f);
        }

        // Pickup events → sparkle / heal burst + chimes.
        route_pickup_audio(world.pickup_events(), audio, particles, specs, reg);

        // Continuous pickup ambient sparkle.
        if (auto* sparkle = specs.find("pickup_sparkle")) {
            float pickup_acc = 0.0f;
            reg.view<game::PickupComponent, game::TransformComponent>().each(
                [&](const game::PickupComponent&, const game::TransformComponent& tr) {
                    auto spec = *sparkle;
                    spec.rate_per_second *= 0.4f;
                    particles.continuous(spec, tr.position, {0.0f, -1.0f},
                                         static_cast<float>(step.frame_dt), pickup_acc);
                });
        }

        // Thrust audio rumble follows the player's current thrust input.
        audio.set_thrust(intent.thrust);

        // Low HP heartbeat (kept from Phase 1; bloom composite also reacts).
        bool low_hp = false;
        float low_hp_strength = 0.0f;
        if (reg.valid(player)) {
            const auto& hp = reg.get<game::HealthComponent>(player);
            low_hp = hp.fraction() > 0.0f && hp.fraction() < 0.25f;
            if (low_hp) low_hp_strength = 1.0f - hp.fraction() / 0.25f;
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

        // --- Update render-side systems. ---
        particles.update(static_cast<float>(step.frame_dt));
        camera.update(player_pos, player_vel, static_cast<float>(step.frame_dt));
        audio.tick(static_cast<float>(step.frame_dt));

        // --- Render: scene → post-FX → composite. ---
        BeginDrawing();
        ClearBackground(Color{14, 18, 28, 255});

        if (postfx.ready()) {
            postfx.begin_scene();
            ClearBackground(Color{14, 18, 28, 255});
        }

        BeginMode2D(camera.raw());
        sprites.drawArena(world.arena());
        particles.draw();
        sprites.drawEntities(reg, static_cast<float>(step.alpha));
        EndMode2D();

        if (postfx.ready()) {
            postfx.end_scene();
            postfx.present(low_hp_strength, gfx.bloom);
        }

        hud.drawScreen(world, GetScreenWidth(), GetScreenHeight());

#if VECTOR_ENABLE_IMGUI
        rlImGuiBegin();
        if (show_overlay) {
            ImGui::SetNextWindowPos(ImVec2(12, 12), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(320, 300), ImGuiCond_FirstUseEver);
            ImGui::Begin("VECTOR debug", &show_overlay);
            ImGui::Text("platform   : %s", platform->name().c_str());
            ImGui::Text("render FPS : %d", GetFPS());
            ImGui::Text("frame dt   : %.2f ms", step.frame_dt * 1000.0);
            ImGui::Text("sim tick   : %llu", static_cast<unsigned long long>(clock.tick()));
            ImGui::Text("ticks/frame: %d", step.sim_ticks);
            ImGui::Text("alpha      : %.3f", step.alpha);
            ImGui::Text("bullets    : %d", world.bullet_count());
            ImGui::Text("bots       : %d", world.bot_count());
            ImGui::Text("particles  : %zu / %zu",
                        particles.active_count(), particles.pool_capacity());
            const char* preset_name =
                gfx.preset == render::GraphicsPreset::Low    ? "Low" :
                gfx.preset == render::GraphicsPreset::Medium ? "Medium" : "High";
            ImGui::Text("preset     : %s  (F2 to cycle)", preset_name);
            ImGui::Text("bloom      : %s", gfx.bloom ? "on" : "off");
            ImGui::Separator();
            if (reg.valid(player)) {
                const auto& hp  = reg.get<game::HealthComponent>(player);
                const auto& inv = reg.get<game::InventoryComponent>(player);
                ImGui::Text("HP   : %.0f / %.0f", hp.current, hp.max);
                ImGui::Text("Q    : %s", game::name_of(inv.primary));
                ImGui::Text("E    : %s", game::name_of(inv.special));
            }
            ImGui::Separator();
            ImGui::TextDisabled("Mouse aim/thrust  |  LMB fire  |  E special");
            ImGui::TextDisabled("F1 overlay  |  F2 graphics preset");
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
