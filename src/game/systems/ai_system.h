#pragma once

#include "ecs/registry.h"

#include <raylib.h>

namespace vector::game {

struct Arena;
struct ShipTuning;
struct WeaponTuning;

namespace ai { class Node; }

// Drives every entity with an AIControllerComponent. Per tick:
//   1. Perception: build per-bot Blackboard from spatial-hash neighbors.
//   2. BT pass (10 Hz, staggered) — writes high-level intent into ctrl.
//   3. Steering pass (every tick) — turns ctrl into a desired velocity and
//      converts that to InputIntent on the bot's IntentComponent.
// AI ships then flow through the *same* Movement/Weapon/Pickup systems as
// the human player (roadmap §6.1).
class AISystem {
public:
    AISystem();
    ~AISystem();

    AISystem(const AISystem&)            = delete;
    AISystem& operator=(const AISystem&) = delete;

    void tick(ecs::Registry& reg,
              const Arena& arena,
              const ShipTuning& ship_tune,
              const WeaponTuning& weapon_tune,
              float dt);

private:
    struct TreeBundle;
    TreeBundle* trees_;
    float       bt_period_s_ {0.10f};   // 10 Hz BT cadence (roadmap §6.4)
};

}  // namespace vector::game
