#include "game/world.h"

#include "core/logging.h"
#include "game/ai/bot_brain.h"
#include "game/components/ai.h"
#include "game/components/bullet.h"
#include "game/components/emitter_state.h"
#include "game/components/health.h"
#include "game/components/intent.h"
#include "game/components/inventory.h"
#include "game/components/physics.h"
#include "game/components/pickup.h"
#include "game/components/render_tag.h"
#include "game/components/ship.h"
#include "game/components/status_effects.h"
#include "game/components/team.h"
#include "game/components/transform.h"
#include "game/components/weapon.h"
#include "game/systems/ai_system.h"

#include <raymath.h>

#include <algorithm>
#include <random>

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

std::mt19937& prng() {
    thread_local std::mt19937 g{std::random_device{}() ^ 0xCAFEBABEu};
    return g;
}

Vector2 random_spawn(const Arena& arena, Vector2 keep_away_from, float min_dist) {
    std::uniform_real_distribution<float> rx(50.0f, arena.width  - 50.0f);
    std::uniform_real_distribution<float> ry(50.0f, arena.height - 50.0f);
    for (int attempt = 0; attempt < 12; ++attempt) {
        const Vector2 p{rx(prng()), ry(prng())};
        const float dx = p.x - keep_away_from.x;
        const float dy = p.y - keep_away_from.y;
        if (dx * dx + dy * dy >= min_dist * min_dist) return p;
    }
    return Vector2{rx(prng()), ry(prng())};
}

}  // namespace

World::World(const Tuning& tuning)
    : tuning_(tuning),
      arena_(Arena::from_tuning(tuning.arena)),
      hash_(2.0f * std::max(tuning.ship.radius, tuning.pickup.radius)),
      ai_(new AISystem()) {

    spawn_player();
    spawn_bots(8);  // 1v8 Phase 2 default; tune in JSON later.

    // Spread pickup spawn points across the arena instead of hugging the
    // centre — the big-arena experience needs reasons to move.
    const Vector2 c   = arena_.center();
    const float   dx  = arena_.width  * 0.32f;
    const float   dy  = arena_.height * 0.32f;
    spawn_pickup_point({c.x,        c.y - dy},  PowerUpType::RepairPack);
    spawn_pickup_point({c.x,        c.y + dy},  PowerUpType::RepairPack);
    spawn_pickup_point({c.x - dx,   c.y - dy},  PowerUpType::RepairPack);
    spawn_pickup_point({c.x + dx,   c.y - dy},  PowerUpType::RepairPack);
    spawn_pickup_point({c.x - dx,   c.y + dy},  PowerUpType::RepairPack);
    spawn_pickup_point({c.x + dx,   c.y + dy},  PowerUpType::RepairPack);
    spawn_pickup_point({c.x - dx,   c.y     },  PowerUpType::RepairPack);
    spawn_pickup_point({c.x + dx,   c.y     },  PowerUpType::RepairPack);

    VECTOR_INFO("world: player + 8 bots + 8 pickup spawn points (FFA)");
}

World::~World() { delete ai_; }

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
    reg_.emplace<TeamComponent>(player_, TeamComponent{Team::Player});
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
    reg_.emplace<IntentComponent>(player_);
    reg_.emplace<StatusEffectComponent>(player_);
    reg_.emplace<EmitterStateComponent>(player_);
    reg_.emplace<RenderComponent>(player_, RenderComponent{Shape::TriangleShip, SKYBLUE, tuning_.ship.radius});
    give_spawn_protection(reg_, player_, tuning_.ship.spawn_protect_s);
}

void World::spawn_bots(int count) {
    const Vector2 player_pos = arena_.center();
    const BotArchetype mix[] = {
        BotArchetype::Brawler, BotArchetype::Brawler,
        BotArchetype::Sniper,  BotArchetype::Runner,
        BotArchetype::Brawler, BotArchetype::Sniper,
        BotArchetype::Runner,  BotArchetype::Brawler,
    };
    for (int i = 0; i < count; ++i) {
        const auto archetype = mix[i % (sizeof(mix) / sizeof(mix[0]))];
        const auto preset    = ai::preset_for(archetype);

        auto e = reg_.create();
        const Vector2 pos = random_spawn(arena_, player_pos, 600.0f);
        reg_.emplace<TransformComponent>(e, TransformComponent{pos, 0.0f});
        reg_.emplace<PrevTransformComponent>(e);
        reg_.emplace<VelocityComponent>(e);
        reg_.emplace<RigidBodyComponent>(e, RigidBodyComponent{
            tuning_.ship.linear_damping, tuning_.ship.angular_damping, tuning_.ship.max_speed
        });
        reg_.emplace<ColliderComponent>(e, ColliderComponent{tuning_.ship.radius});
        reg_.emplace<ShipComponent>(e, ShipComponent{tuning_.ship.thrust_accel, tuning_.ship.angular_accel});
        reg_.emplace<TeamComponent>(e, TeamComponent{Team::Bot});
        reg_.emplace<HealthComponent>(e, HealthComponent{
            tuning_.ship.hp_max, tuning_.ship.hp_max,
            tuning_.ship.hp_regen_rate, tuning_.ship.hp_regen_delay, 999.0f
        });
        reg_.emplace<WeaponComponent>(e, WeaponComponent{
            tuning_.weapon.fire_rate * 0.6f,
            tuning_.weapon.bullet_speed,
            tuning_.weapon.bullet_lifetime,
            tuning_.weapon.bullet_damage,
            tuning_.weapon.bullet_radius, 0.0f
        });
        reg_.emplace<InventoryComponent>(e);
        reg_.emplace<IntentComponent>(e);
        reg_.emplace<StatusEffectComponent>(e);
        reg_.emplace<EmitterStateComponent>(e);
        reg_.emplace<AIControllerComponent>(e, preset.ctrl_defaults);

        // Color-code archetype so visual distinction is immediate.
        Color tint;
        switch (archetype) {
            case BotArchetype::Sniper:  tint = Color{200, 160, 255, 255}; break;
            case BotArchetype::Brawler: tint = Color{255, 120, 120, 255}; break;
            case BotArchetype::Runner:  tint = Color{255, 200, 100, 255}; break;
        }
        reg_.emplace<RenderComponent>(e, RenderComponent{Shape::TriangleShip, tint, tuning_.ship.radius});

        // Stagger BT ticks so 8 bots don't all evaluate on the same frame.
        std::uniform_real_distribution<float> st(0.0f, 0.10f);
        reg_.get<AIControllerComponent>(e).bt_tick_in_s = st(prng());

        give_spawn_protection(reg_, e, tuning_.ship.spawn_protect_s);
    }
}

void World::spawn_pickup_point(Vector2 pos, PowerUpType type) {
    auto e = reg_.create();
    reg_.emplace<TransformComponent>(e, TransformComponent{pos, 0.0f});
    reg_.emplace<PickupSpawnPoint>(e, PickupSpawnPoint{
        tuning_.pickup.spawn_cooldown,
        tuning_.pickup.spawn_jitter,
        0.0f,
        type
    });
}

void World::handle_deaths() {
    for (const auto& d : health_.deaths_this_tick()) {
        if (!reg_.valid(d.entity)) continue;

        // Bots: respawn at a random safe location after instant heal (Phase
        // 2 keeps the dogfight going; proper respawn timer lands in Phase 4).
        if (reg_.any_of<AIControllerComponent>(d.entity)) {
            auto& hp  = reg_.get<HealthComponent>(d.entity);
            hp.current = hp.max;
            hp.time_since_dmg = 999.0f;
            auto& tr = reg_.get<TransformComponent>(d.entity);
            tr.position = random_spawn(arena_, arena_.center(), 500.0f);
            auto& vel = reg_.get<VelocityComponent>(d.entity);
            vel = {};
            give_spawn_protection(reg_, d.entity, tuning_.ship.spawn_protect_s);
            continue;
        }

        if (d.entity == player_) {
            auto& hp = reg_.get<HealthComponent>(player_);
            hp.current = hp.max;
            hp.time_since_dmg = 999.0f;
            give_spawn_protection(reg_, player_, tuning_.ship.spawn_protect_s);
            auto& tr = reg_.get<TransformComponent>(player_);
            tr.position = arena_.center();
            auto& vel = reg_.get<VelocityComponent>(player_);
            vel = {};
            VECTOR_INFO("player down — respawn at center (Phase 2 stub)");
        }
    }
}

int World::bullet_count() const {
    return static_cast<int>(reg_.view<const BulletComponent>().size());
}

int World::bot_count() const {
    return static_cast<int>(reg_.view<const AIControllerComponent>().size());
}

void World::tick(float dt) {
    ai_->tick(reg_, arena_, tuning_.ship, tuning_.weapon, dt);

    movement_.tick(reg_, dt);
    weapons_.tick(reg_, tuning_.weapon, tuning_.ship, dt);

    physics_.tick(reg_, arena_, dt, tuning_.arena.wall_bounce);
    collision_.tick(reg_, hash_);
    health_.tick(reg_, dt);
    pickups_.tick(reg_, tuning_.pickup, dt);
    lifetime_.tick(reg_, dt);

    handle_deaths();
}

}  // namespace vector::game
