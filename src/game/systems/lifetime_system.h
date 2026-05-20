#pragma once

#include "ecs/registry.h"

namespace vector::game {

// Decrements every LifetimeComponent and destroys entities at <= 0.
class LifetimeSystem {
public:
    void tick(ecs::Registry& reg, float dt);
};

}  // namespace vector::game
