#pragma once

#include "ecs/registry.h"

namespace vector::game {

// Reads every Ship+Intent pair and converts the per-tick InputIntent into
// angular + linear acceleration. Runs *before* PhysicsSystem each tick.
// AI and human-controlled ships are indistinguishable from this system's
// POV — they both have an `IntentComponent` filled by their controller.
class MovementSystem {
public:
    void tick(ecs::Registry& reg, float dt);
};

}  // namespace vector::game
