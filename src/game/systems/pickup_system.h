#pragma once

#include "ecs/registry.h"

#include <vector>

namespace vector::game {

struct PickupTuning;

class PickupSystem {
public:
    struct Event {
        enum Kind { Spawned, Acquired, Activated, Despawned, ActivationDenied } kind;
        ecs::Entity ship;  // for Acquired/Activated; null otherwise
    };

    void tick(ecs::Registry& reg, const PickupTuning& tune, float dt);

    const std::vector<Event>& events_this_tick() const { return events_; }

private:
    std::vector<Event> events_;
};

}  // namespace vector::game
