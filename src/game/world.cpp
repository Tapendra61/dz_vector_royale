#include "game/world.h"

#include "core/logging.h"
#include "game/components/bullet.h"
#include "game/components/health.h"
#include "game/components/inventory.h"
#include "game/components/physics.h"
#include "game/components/pickup.h"
#include "game/components/render_tag.h"
#include "game/components/ship.h"
#include "game/components/status_effects.h"
#include "game/components/transform.h"
#include "game/components/weapon.h"

#include <raymath.h>

#include <algorithm>

namespace vector::game {

namespace {

void give_spawn_protection(ecs::Registry& reg, ecs::Entity e, float seconds) {
    auto& sx = reg.get_or_emplace<StatusEffectComponent>(e);
    for (auto& slot : sx.effects) {
        if (slot.type == StatusEffectType::None) {
            slot.type           = StatusEffectType::Invulnerable;
            slot.time_remaining = seconds;
            return;
        }
    }
}

}  // namespace

World::World(const Tuning& tuning)
    : tuning_(tuning),
      arena_(Arena::from_tuning(tuning.arena)),
      hash_(2.0f * std::max(tuning.ship.radius, tuning.pickup.radius)) {

    spawn_player();

    // Phase 1 sanity: a few dummies so HP/damage feel can be exercised
    // without AI (roadmap §5.4 — "two ships duel without instant death").
    spawn_target_dummy({arena_.center().x + 300.0f, arena_.center().y - 200.0f});
    spawn_target_dummy({arena_.center().x - 400.0f, arena_.center().y + 250.0f});
    spawn_target_dummy({arena_.center().x + 250.0f, arena_.center().y + 320.0f});

    // Pickup spawn points — all RepairPack for Phase 1.
    spawn_pickup_point({arena_.center().x,           arena_.center().y - 280.0f}, PowerUpType::RepairPack);
    spawn_pickup_point({arena_.center().x + 500.0f,  arena_.center().y + 50.0f},  PowerUpType::RepairPack);
    spawn_pickup_point({arena_.center().x - 500.0f,  arena_.center().y + 50.0f},  PowerUpType::RepairPack);
    spawn_pickup_point({arena_.center().x,           arena_.center().y + 360.0f}, PowerUpType::RepairPack);

    VECTOR_INFO("world: spawned player + 3 dummies + 4 pickup spawn points");
}

void World::spawn_player() {
    player_ = reg_.create();
    reg_.emplace<TransformComponent>(player_, TransformComponent{arena_.center(), 0.0f});
    reg_.emplace<PrevTransformComponent>(player_);
    reg_.emplace<VelocityComponent>(player_);
    reg_.emplace<RigidBodyComponent>(player_, RigidBodyComponent{
        tuning_.ship.linear_damping, tuning_.ship.angular_damping, tuning_.ship.max_speed
    });
    reg_.emplace<ColliderComponent>(player_, ColliderComponent{tuning_.ship.radius});
    reg_.emplace<ShipComponent>(player_, ShipComponent{tuning_.ship.thrust_accel, tuning_.ship.angular_accel});
    reg_.emplace<LocalPlayerTag>(player_);
    reg_.emplace<HealthComponent>(player_, HealthComponent{
        tuning_.ship.hp_max, tuning_.ship.hp_max,
        tuning_.ship.hp_regen_rate, tuning_.ship.hp_regen_delay, 999.0f
    });
    reg_.emplace<WeaponComponent>(player_, WeaponComponent{
        tuning_.weapon.fire_rate, tuning_.weapon.bullet_speed,
        tuning_.weapon.bullet_lifetime, tuning_.weapon.bullet_damage,
        tuning_.weapon.bullet_radius, 0.0f
    });
    reg_.emplace<InventoryComponent>(player_);
    reg_.emplace<StatusEffectComponent>(player_);
    reg_.emplace<RenderComponent>(player_, RenderComponent{Shape::TriangleShip, SKYBLUE, tuning_.ship.radius});
    give_spawn_protection(reg_, player_, tuning_.ship.spawn_protect_s);
}

void World::spawn_target_dummy(Vector2 pos) {
    auto e = reg_.create();
    reg_.emplace<TransformComponent>(e, TransformComponent{pos, 0.0f});
    reg_.emplace<PrevTransformComponent>(e);
    reg_.emplace<VelocityComponent>(e);
    reg_.emplace<ColliderComponent>(e, ColliderComponent{tuning_.ship.radius});
    reg_.emplace<ShipComponent>(e, ShipComponent{0.0f, 0.0f});
    reg_.emplace<TargetDummyTag>(e);
    reg_.emplace<HealthComponent>(e, HealthComponent{
        tuning_.ship.hp_max, tuning_.ship.hp_max,
        tuning_.ship.hp_regen_rate, tuning_.ship.hp_regen_delay, 999.0f
    });
    reg_.emplace<StatusEffectComponent>(e);
    reg_.emplace<RenderComponent>(e, RenderComponent{Shape::TriangleShip, Color{220, 80, 80, 255}, tuning_.ship.radius});
}

void World::spawn_pickup_point(Vector2 pos, PowerUpType type) {
    auto e = reg_.create();
    reg_.emplace<TransformComponent>(e, TransformComponent{pos, 0.0f});
    reg_.emplace<PickupSpawnPoint>(e, PickupSpawnPoint{
        tuning_.pickup.spawn_cooldown,
        tuning_.pickup.spawn_jitter,
        0.0f,  // first one spawns immediately
        type
    });
}

void World::handle_deaths() {
    for (const auto& d : health_.deaths_this_tick()) {
        if (!reg_.valid(d.entity)) continue;

        if (auto* dummy = reg_.try_get<TargetDummyTag>(d.entity)) {
            // Respawn dummies in place after a short delay — useful for
            // sustained damage tuning in Phase 1.
            auto& hp = reg_.get<HealthComponent>(d.entity);
            hp.current        = hp.max;
            hp.time_since_dmg = 999.0f;
            give_spawn_protection(reg_, d.entity, tuning_.ship.spawn_protect_s);
            VECTOR_INFO("dummy down (killer={}) — instant respawn", d.killer_id);
            (void)dummy;
            continue;
        }

        if (d.entity == player_) {
            // Phase 1 keeps the local player alive on "death" by full-healing
            // and reapplying spawn protection — death/respawn flow belongs to
            // Phase 4 (modes). The HP system still proves the pipeline.
            auto& hp = reg_.get<HealthComponent>(player_);
            hp.current        = hp.max;
            hp.time_since_dmg = 999.0f;
            give_spawn_protection(reg_, player_, tuning_.ship.spawn_protect_s);
            VECTOR_INFO("player down — respawn at center (Phase 1 stub)");
            auto& tr = reg_.get<TransformComponent>(player_);
            tr.position = arena_.center();
            auto& vel = reg_.get<VelocityComponent>(player_);
            vel = {};
        }
    }
}

int World::bullet_count() const {
    return static_cast<int>(reg_.view<const BulletComponent>().size());
}

void World::tick(const InputIntent& intent, float dt) {
    if (reg_.valid(player_)) {
        movement_.apply_player_intent(reg_, player_, intent, tuning_.ship, dt);
        weapons_.tick(reg_, player_, intent, tuning_.weapon, tuning_.ship, dt);
    }

    physics_.tick(reg_, arena_, dt, tuning_.arena.wall_bounce);
    collision_.tick(reg_, hash_);
    health_.tick(reg_, dt);
    pickups_.tick(reg_, player_, intent, tuning_.pickup, dt);
    lifetime_.tick(reg_, dt);

    handle_deaths();
}

}  // namespace vector::game
