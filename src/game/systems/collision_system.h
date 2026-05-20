#pragma once

#include "ecs/registry.h"

namespace vector::game {

class SpatialHash;

// Bullet→ship collision. Pickup→ship is handled separately by PickupSystem
// because the resolution is different (consume vs. damage).
class CollisionSystem {
public:
    void tick(ecs::Registry& reg, SpatialHash& hash);
};

}  // namespace vector::game
