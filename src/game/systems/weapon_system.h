#pragma once

#include "ecs/registry.h"

#include <raylib.h>

#include <vector>

namespace vector::game {

struct WeaponTuning;
struct ShipTuning;

// Iterates every ship with a WeaponComponent + IntentComponent. Bullets
// are spawned identically for player and AI (roadmap §6.1). Records a
// FireEvent per shot so main can route distance-attenuated SFX.
class WeaponSystem {
public:
    struct FireEvent {
        ecs::Entity shooter;
        Vector2     position;
    };

    void tick(ecs::Registry& reg,
              const WeaponTuning& weapon_tune,
              const ShipTuning& ship_tune,
              float dt);

    const std::vector<FireEvent>& fires_this_tick() const { return fires_; }

private:
    std::vector<FireEvent> fires_;
};

}  // namespace vector::game
