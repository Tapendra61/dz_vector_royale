#pragma once

#include "ecs/registry.h"

#include <vector>

namespace vector::game {

// Owns HP regen, damage application, status-effect timers, and emits a
// list of dead entities each tick for the world to act on (respawn,
// drop souls, end the match, etc.).
class HealthSystem {
public:
    struct DeathRecord {
        ecs::Entity entity;
        unsigned    killer_id;
    };

    void tick(ecs::Registry& reg, float dt);

    const std::vector<DeathRecord>& deaths_this_tick() const { return deaths_; }

private:
    std::vector<DeathRecord> deaths_;
};

}  // namespace vector::game
