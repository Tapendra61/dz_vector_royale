#include "game/systems/weapon_system.h"

#include "game/components/bullet.h"
#include "game/components/physics.h"
#include "game/components/render_tag.h"
#include "game/components/status_effects.h"
#include "game/components/transform.h"
#include "game/components/weapon.h"
#include "game/data/tuning.h"

#include <raymath.h>

#include <cmath>

namespace vector::game {

void WeaponSystem::tick(ecs::Registry& reg,
                       ecs::Entity ship,
                       const InputIntent& intent,
                       const WeaponTuning& weapon_tune,
                       const ShipTuning& ship_tune,
                       float dt) {
    if (!reg.valid(ship)) return;

    auto* wpn = reg.try_get<WeaponComponent>(ship);
    if (!wpn) return;

    if (wpn->cooldown > 0.0f) wpn->cooldown -= dt;

    // Spawn protection inhibits firing.
    if (auto* sx = reg.try_get<StatusEffectComponent>(ship)) {
        if (sx->has(StatusEffectType::Invulnerable)) return;
    }

    if (!intent.fire || wpn->cooldown > 0.0f) return;
    wpn->cooldown = 1.0f / wpn->fire_rate;

    const auto& tr = reg.get<TransformComponent>(ship);
    const Vector2 facing{std::cos(tr.rotation), std::sin(tr.rotation)};

    auto b = reg.create();
    reg.emplace<TransformComponent>(b, TransformComponent{
        Vector2{tr.position.x + facing.x * (ship_tune.radius + 4.0f),
                tr.position.y + facing.y * (ship_tune.radius + 4.0f)},
        tr.rotation
    });
    reg.emplace<PrevTransformComponent>(b);
    reg.emplace<VelocityComponent>(b, VelocityComponent{
        Vector2{facing.x * wpn->bullet_speed, facing.y * wpn->bullet_speed},
        0.0f
    });
    reg.emplace<ColliderComponent>(b, ColliderComponent{wpn->bullet_radius});
    reg.emplace<BulletComponent>(b, BulletComponent{wpn->bullet_damage,
                                                    static_cast<unsigned>(static_cast<std::uint32_t>(ship))});
    reg.emplace<LifetimeComponent>(b, LifetimeComponent{wpn->bullet_lifetime});
    reg.emplace<RenderComponent>(b, RenderComponent{Shape::Circle, YELLOW, wpn->bullet_radius});
}

}  // namespace vector::game
