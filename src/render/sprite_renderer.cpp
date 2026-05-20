#include "render/sprite_renderer.h"

#include "game/arena.h"
#include "game/components/health.h"
#include "game/components/pickup.h"
#include "game/components/render_tag.h"
#include "game/components/status_effects.h"
#include "game/components/transform.h"

#include <raymath.h>

#include <cmath>

namespace vector::render {

namespace {

inline Vector2 lerp(Vector2 a, Vector2 b, float t) {
    return Vector2{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

inline float lerp_angle(float a, float b, float t) {
    float d = b - a;
    while (d >  PI) d -= 2.0f * PI;
    while (d < -PI) d += 2.0f * PI;
    return a + d * t;
}

void draw_triangle_ship(Vector2 pos, float rot, float size, Color color) {
    // Elongated triangle pointing along +X at rotation 0.
    const Vector2 nose { pos.x + std::cos(rot) * size * 1.6f,
                         pos.y + std::sin(rot) * size * 1.6f };
    const float left_a  = rot + 2.5f;
    const float right_a = rot - 2.5f;
    const Vector2 left  { pos.x + std::cos(left_a)  * size,
                          pos.y + std::sin(left_a)  * size };
    const Vector2 right { pos.x + std::cos(right_a) * size,
                          pos.y + std::sin(right_a) * size };
    DrawTriangle(nose, left, right, color);
    DrawTriangleLines(nose, left, right, Fade(WHITE, 0.7f));
}

void draw_pickup(Vector2 pos, float size, Color color, float bob) {
    const float r = size + std::sin(bob) * 2.0f;
    DrawCircleV(pos, r + 4.0f, Fade(color, 0.25f));
    DrawCircleV(pos, r,         color);
    DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y),
                    r + 6.0f, Fade(WHITE, 0.4f));
    const char* mark = "+";
    const int   fs   = 18;
    const int   tw   = MeasureText(mark, fs);
    DrawText(mark, static_cast<int>(pos.x) - tw / 2,
                   static_cast<int>(pos.y) - fs / 2, fs, WHITE);
}

void draw_hp_bar(Vector2 pos, float fraction) {
    if (fraction >= 0.999f) return;
    const float w = 28.0f, h = 4.0f;
    const Vector2 origin{pos.x - w * 0.5f, pos.y - 26.0f};
    DrawRectangleV(origin, {w, h}, Fade(BLACK, 0.55f));
    DrawRectangleV(origin, {w * std::max(0.0f, fraction), h},
                   fraction > 0.5f ? GREEN : (fraction > 0.25f ? YELLOW : RED));
}

}  // namespace

void SpriteRenderer::drawArena(const game::Arena& arena) {
    const auto b = arena.bounds();
    DrawRectangleLinesEx(b, 3.0f, Fade(SKYBLUE, 0.55f));
    // Subtle grid so motion is readable.
    constexpr float step = 200.0f;
    for (float x = b.x; x <= b.x + b.width;  x += step)
        DrawLineV({x, b.y}, {x, b.y + b.height}, Fade(GRAY, 0.15f));
    for (float y = b.y; y <= b.y + b.height; y += step)
        DrawLineV({b.x, y}, {b.x + b.width, y}, Fade(GRAY, 0.15f));
}

void SpriteRenderer::drawEntities(ecs::Registry& reg, float alpha) {
    // Pickups first (so ships render over them).
    reg.view<game::TransformComponent, game::PickupComponent, game::RenderComponent>().each(
        [&](const game::TransformComponent& tr, const game::PickupComponent& p, const game::RenderComponent& rc) {
            draw_pickup(tr.position, rc.size, rc.color, p.bob_phase);
        });

    // Everything else, interpolated between prev/current transform.
    reg.view<game::TransformComponent, game::PrevTransformComponent, game::RenderComponent>().each(
        [&](ecs::Entity e, const game::TransformComponent& tr, const game::PrevTransformComponent& prev,
            const game::RenderComponent& rc) {
            if (reg.any_of<game::PickupComponent>(e)) return;

            const Vector2 pos = lerp(prev.position, tr.position, alpha);
            const float   rot = lerp_angle(prev.rotation, tr.rotation, alpha);

            Color color = rc.color;
            if (auto* sx = reg.try_get<game::StatusEffectComponent>(e)) {
                if (sx->has(game::StatusEffectType::Invulnerable)) {
                    color = Fade(color, 0.45f);
                }
            }

            switch (rc.shape) {
                case game::Shape::TriangleShip: draw_triangle_ship(pos, rot, rc.size, color); break;
                case game::Shape::Circle:       DrawCircleV(pos, rc.size, color);             break;
                case game::Shape::Pickup:       draw_pickup(pos, rc.size, color, 0.0f);       break;
            }

            if (auto* hp = reg.try_get<game::HealthComponent>(e)) {
                draw_hp_bar(pos, hp->fraction());
            }
        });
}

}  // namespace vector::render
