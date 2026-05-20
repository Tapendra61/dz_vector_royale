#pragma once

#include "ecs/registry.h"
#include "game/input/input_intent.h"

#include <vector>

namespace vector::game {

struct PickupTuning;

// Spawns / despawns pickups, handles ship-on-pickup overlap, and resolves
// activation. Phase 1 only implements RepairPack — but the pipeline is the
// final one (roadmap §5.3).
class PickupSystem {
public:
    struct Event {
        enum Kind { Spawned, Acquired, Activated, Despawned, ActivationDenied } kind;
        ecs::Entity ship;     // for Acquired/Activated; ignored otherwise
    };

    void tick(ecs::Registry& reg,
              ecs::Entity player_ship,
              const InputIntent& intent,
              const PickupTuning& tune,
              float dt);

    const std::vector<Event>& events_this_tick() const { return events_; }

private:
    std::vector<Event> events_;
};

}  // namespace vector::game
