#include "game/systems/weapon_system.h"

#include "game/components/bullet.h"
#include "game/components/emitter_state.h"
#include "game/components/intent.h"
#include "game/components/physics.h"
#include "game/components/render_tag.h"
#include "game/components/status_effects.h"
#include "game/components/transform.h"
#include "game/components/weapon.h"
#include "game/data/tuning.h"

#include <raymath.h>

#include <cmath>
#include <cstdint>

namespace vector::game {

void WeaponSystem::tick(ecs::Registry& reg,
                       const WeaponTuning& /*weapon_tune*/,
                       const ShipTuning& ship_tune,
                       float dt) {
    auto view = reg.view<WeaponComponent, IntentComponent, TransformComponent>();
    for (auto e : view) {
        auto& wpn   = view.get<WeaponComponent>(e);
        const auto& intent_c = view.get<IntentComponent>(e);
        const auto& tr  = view.get<TransformComponent>(e);

        if (wpn.cooldown > 0.0f) wpn.cooldown -= dt;

        // Spawn protection inhibits firing — applies equally to player and AI.
        if (auto* sx = reg.try_get<StatusEffectComponent>(e)) {
            if (sx->has(StatusEffectType::Invulnerable)) continue;
            if (sx->has(StatusEffectType::Frozen) ||
                sx->has(StatusEffectType::Stunned)) continue;
        }

        if (!intent_c.value.fire || wpn.cooldown > 0.0f) continue;
        wpn.cooldown = 1.0f / wpn.fire_rate;

        const Vector2 facing{std::cos(tr.rotation), std::sin(tr.rotation)};

        auto b = reg.create();
        reg.emplace<TransformComponent>(b, TransformComponent{
            Vector2{tr.position.x + facing.x * (ship_tune.radius + 4.0f),
                    tr.position.y + facing.y * (ship_tune.radius + 4.0f)},
            tr.rotation
        });
        reg.emplace<PrevTransformComponent>(b);
        reg.emplace<VelocityComponent>(b, VelocityComponent{
            Vector2{facing.x * wpn.bullet_speed, facing.y * wpn.bullet_speed},
            0.0f
        });
        reg.emplace<ColliderComponent>(b, ColliderComponent{wpn.bullet_radius});
        reg.emplace<BulletComponent>(b, BulletComponent{
            wpn.bullet_damage,
            static_cast<unsigned>(static_cast<std::uint32_t>(e))
        });
        reg.emplace<LifetimeComponent>(b, LifetimeComponent{wpn.bullet_lifetime});
        reg.emplace<RenderComponent>(b, RenderComponent{Shape::Circle, YELLOW, wpn.bullet_radius});
        reg.emplace<EmitterStateComponent>(b);
    }
}

}  // namespace vector::game
