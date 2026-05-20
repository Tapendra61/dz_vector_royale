#include "game/systems/collision_system.h"

#include "game/components/bullet.h"
#include "game/components/health.h"
#include "game/components/physics.h"
#include "game/components/ship.h"
#include "game/components/status_effects.h"
#include "game/components/transform.h"
#include "game/systems/spatial_hash.h"

#include <raymath.h>

#include <cmath>
#include <cstdint>
#include <vector>

namespace vector::game {

void CollisionSystem::tick(ecs::Registry& reg, SpatialHash& hash) {
    hits_.clear();
    hash.clear();
    // Index everything with a collider AND a ShipComponent (the targets).
    reg.view<TransformComponent, ColliderComponent, ShipComponent>().each(
        [&](ecs::Entity e, const TransformComponent& tr, const ColliderComponent& col, const ShipComponent&) {
            hash.insert(e, tr.position, col.radius);
        });

    std::vector<ecs::Entity> candidates;
    std::vector<ecs::Entity> doomed_bullets;

    reg.view<BulletComponent, TransformComponent, ColliderComponent>().each(
        [&](ecs::Entity bullet, const BulletComponent& bc, const TransformComponent& btr, const ColliderComponent& bcol) {
            candidates.clear();
            hash.query(btr.position, bcol.radius, candidates);
            for (auto target : candidates) {
                if (!reg.valid(target)) continue;
                if (static_cast<std::uint32_t>(target) == bc.owner_id) continue;

                if (auto* sx = reg.try_get<StatusEffectComponent>(target)) {
                    if (sx->has(StatusEffectType::Invulnerable)) continue;
                }

                const auto& ttr = reg.get<TransformComponent>(target);
                const auto& tco = reg.get<ColliderComponent>(target);
                const float dx = ttr.position.x - btr.position.x;
                const float dy = ttr.position.y - btr.position.y;
                const float r  = tco.radius + bcol.radius;
                if (dx * dx + dy * dy <= r * r) {
                    auto& dmg = reg.get_or_emplace<DamageEvent>(target);
                    dmg.amount += bc.damage;
                    dmg.source  = bc.owner_id;
                    const auto& bvel = reg.get<VelocityComponent>(bullet);
                    Vector2 in_dir = bvel.linear;
                    const float l2 = in_dir.x * in_dir.x + in_dir.y * in_dir.y;
                    if (l2 > 1e-6f) {
                        const float inv = 1.0f / std::sqrt(l2);
                        in_dir.x *= inv;
                        in_dir.y *= inv;
                    }
                    hits_.push_back({target, btr.position, in_dir});
                    doomed_bullets.push_back(bullet);
                    break;
                }
            }
        });

    for (auto e : doomed_bullets) if (reg.valid(e)) reg.destroy(e);
}

}  // namespace vector::game
