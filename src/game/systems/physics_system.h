#pragma once

#include "ecs/registry.h"

namespace vector::game {

struct Arena;

// Integrates velocity into position, applies damping, enforces arena
// bounds (bounce or clamp). Runs once per sim tick (60 Hz).
class PhysicsSystem {
public:
    void tick(ecs::Registry& reg, const Arena& arena, float dt, float wall_bounce);
};

}  // namespace vector::game
