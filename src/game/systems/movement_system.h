#pragma once

#include "ecs/registry.h"
#include "game/input/input_intent.h"

namespace vector::game {

struct ShipTuning;

// Translates a per-tick InputIntent into angular + linear acceleration on
// the player-controlled ship. Runs *before* PhysicsSystem each tick.
class MovementSystem {
public:
    void apply_player_intent(ecs::Registry& reg,
                             ecs::Entity ship,
                             const InputIntent& intent,
                             const ShipTuning& tuning,
                             float dt);
};

}  // namespace vector::game
