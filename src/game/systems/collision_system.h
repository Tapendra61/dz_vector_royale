#pragma once

#include "ecs/registry.h"

#include <raylib.h>

#include <vector>

namespace vector::game {

class SpatialHash;

// Bullet→ship collision. Pickup→ship is handled separately by PickupSystem.
class CollisionSystem {
public:
    struct Hit {
        ecs::Entity target;
        Vector2     position;
        Vector2     incoming_dir;
    };

    void tick(ecs::Registry& reg, SpatialHash& hash);
    const std::vector<Hit>& hits_this_tick() const { return hits_; }

private:
    std::vector<Hit> hits_;
};

}  // namespace vector::game
