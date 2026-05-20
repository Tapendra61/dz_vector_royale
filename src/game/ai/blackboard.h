#pragma once

#include "ecs/registry.h"

#include <raylib.h>

namespace vector::game {
struct AIControllerComponent;
struct HealthComponent;
struct InventoryComponent;
struct TransformComponent;
struct VelocityComponent;
}

namespace vector::game::ai {

// Aggregated view of "what this bot knows right now". Built fresh each BT
// tick and handed to every Node::evaluate call. The BT only writes back to
// `ctrl` (the AIControllerComponent), never directly to other components.
struct Blackboard {
    ecs::Registry*               reg;
    ecs::Entity                  self;

    TransformComponent*          self_tr;
    VelocityComponent*           self_vel;
    HealthComponent*             self_hp;
    InventoryComponent*          self_inv;
    AIControllerComponent*       ctrl;

    // Filled by AISystem's perception pass.
    ecs::Entity                  best_target          {ecs::null_entity};
    Vector2                      best_target_pos      {0, 0};
    Vector2                      best_target_vel      {0, 0};
    float                        best_target_distance {1e9f};

    ecs::Entity                  nearest_pickup       {ecs::null_entity};
    Vector2                      nearest_pickup_pos   {0, 0};
    float                        nearest_pickup_dist  {1e9f};

    Rectangle                    arena_bounds         {0, 0, 0, 0};
    float                        dt_seconds           {0.1f};
};

}  // namespace vector::game::ai
