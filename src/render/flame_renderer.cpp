#include "render/flame_renderer.h"

#include "core/logging.h"
#include "game/components/intent.h"
#include "game/components/ship.h"
#include "game/components/status_effects.h"
#include "game/components/transform.h"

#include <raymath.h>
#include <rlgl.h>

#include <cmath>

namespace vector::render {

FlameRenderer::FlameRenderer() = default;

FlameRenderer::~FlameRenderer() {
    if (ready_) {
        UnloadShader(shader_);
        UnloadTexture(blank_);
    }
}

bool FlameRenderer::init(const std::filesystem::path& shader_dir) {
    shader_ = LoadShader(nullptr, (shader_dir / "flame.fs").string().c_str());
    if (shader_.id == 0) {
        VECTOR_WARN("flame_renderer: shader failed to compile, flame disabled");
        return false;
    }
    u_time_loc_      = GetShaderLocation(shader_, "u_time");
    u_intensity_loc_ = GetShaderLocation(shader_, "u_intensity");

    Image img = GenImageColor(2, 2, WHITE);
    blank_ = LoadTextureFromImage(img);
    UnloadImage(img);

    ready_ = true;
    VECTOR_INFO("flame_renderer: ready");
    return true;
}

void FlameRenderer::draw(const ecs::Registry& reg, float ship_radius, float time_s) {
    if (!ready_) return;

    SetShaderValue(shader_, u_time_loc_, &time_s, SHADER_UNIFORM_FLOAT);

    rlSetBlendMode(BLEND_ADDITIVE);

    auto view = reg.view<const game::TransformComponent,
                          const game::IntentComponent,
                          const game::ShipComponent>();
    for (auto e : view) {
        const auto& tr     = view.get<const game::TransformComponent>(e);
        const auto& intent = view.get<const game::IntentComponent>(e);
        if (intent.value.thrust <= 0.02f) continue;

        if (auto* sx = reg.try_get<const game::StatusEffectComponent>(e)) {
            if (sx->has(game::StatusEffectType::Frozen) ||
                sx->has(game::StatusEffectType::Stunned)) continue;
        }

        const float intensity = intent.value.thrust;
        const float cos_r     = std::cos(tr.rotation);
        const float sin_r     = std::sin(tr.rotation);
        const float plume_len = ship_radius * (3.2f + intensity * 1.6f);
        const float plume_w   = ship_radius * 1.55f;

        // Anchor at the ship's back; rotate the quad so its long axis
        // points opposite to ship facing (i.e., trailing behind).
        const Vector2 tail{tr.position.x - cos_r * ship_radius,
                           tr.position.y - sin_r * ship_radius};
        const float rot_deg = tr.rotation * RAD2DEG + 180.0f;

        // Each draw needs its own uniform; flush the batch in between so
        // intensities don't smear across ships.
        BeginShaderMode(shader_);
        SetShaderValue(shader_, u_intensity_loc_, &intensity, SHADER_UNIFORM_FLOAT);
        Rectangle src{0.0f, 0.0f,
                      static_cast<float>(blank_.width),
                      static_cast<float>(blank_.height)};
        Rectangle dst{tail.x, tail.y, plume_len, plume_w};
        DrawTexturePro(blank_, src, dst,
                       Vector2{0.0f, plume_w * 0.5f},
                       rot_deg, WHITE);
        EndShaderMode();
    }

    rlSetBlendMode(BLEND_ALPHA);
}

}  // namespace vector::render
