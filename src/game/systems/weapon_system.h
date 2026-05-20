#pragma once

#include "ecs/registry.h"
#include "game/input/input_intent.h"

namespace vector::game {

struct WeaponTuning;
struct ShipTuning;

// Fires a bullet from the player ship when the fire-cooldown allows it.
// Phase 1 keeps weapons player-only; AI shooting is wired in Phase 2.
class WeaponSystem {
public:
    void tick(ecs::Registry& reg,
              ecs::Entity ship,
              const InputIntent& intent,
              const WeaponTuning& weapon_tune,
              const ShipTuning& ship_tune,
              float dt);
};

}  // namespace vector::game
