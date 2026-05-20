#pragma once

#include "ecs/registry.h"
#include "game/arena.h"
#include "game/components/pickup.h"
#include "game/data/tuning.h"
#include "game/input/input_intent.h"
#include "game/systems/collision_system.h"
#include "game/systems/health_system.h"
#include "game/systems/lifetime_system.h"
#include "game/systems/movement_system.h"
#include "game/systems/physics_system.h"
#include "game/systems/pickup_system.h"
#include "game/systems/spatial_hash.h"
#include "game/systems/weapon_system.h"

namespace vector::game {

// Composition root for Phase 1: owns the entt registry, all systems, the
// arena, the tuning data, and runs one fixed-timestep tick per call. The
// renderer reads `registry()` after the loop drains.
class World {
public:
    explicit World(const Tuning& tuning);

    void tick(const InputIntent& intent, float dt);

    ecs::Registry&  registry()       { return reg_; }
    const Arena&    arena()    const { return arena_; }
    const Tuning&   tuning()   const { return tuning_; }
    ecs::Entity     player()   const { return player_; }
    int             bullet_count() const;

    const std::vector<PickupSystem::Event>&       pickup_events() const { return pickups_.events_this_tick(); }
    const std::vector<HealthSystem::DeathRecord>& deaths()        const { return health_.deaths_this_tick(); }

private:
    void spawn_player();
    void spawn_target_dummy(Vector2 pos);
    void spawn_pickup_point(Vector2 pos, PowerUpType type);
    void handle_deaths();

    Tuning          tuning_;
    Arena           arena_;
    ecs::Registry   reg_;
    ecs::Entity     player_ = ecs::null_entity;

    MovementSystem  movement_;
    PhysicsSystem   physics_;
    WeaponSystem    weapons_;
    CollisionSystem collision_;
    HealthSystem    health_;
    PickupSystem    pickups_;
    LifetimeSystem  lifetime_;
    SpatialHash     hash_;
};

}  // namespace vector::game
