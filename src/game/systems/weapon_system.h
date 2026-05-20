#pragma once

#include "ecs/registry.h"

namespace vector::game {

struct WeaponTuning;
struct ShipTuning;

// Iterates every ship with a WeaponComponent + IntentComponent. Bullets
// are spawned identically for player and AI (roadmap §6.1).
class WeaponSystem {
public:
    void tick(ecs::Registry& reg,
              const WeaponTuning& weapon_tune,
              const ShipTuning& ship_tune,
              float dt);
};

}  // namespace vector::game
