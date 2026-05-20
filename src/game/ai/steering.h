#pragma once

#include "ecs/registry.h"

#include <raylib.h>

#include <cstddef>

namespace vector::game::ai {

// Pure functions following Craig Reynolds — given an agent's current
// state and a target, return a *desired velocity* vector. The AISystem
// composes these weighted into a single steering force that's converted
// to an InputIntent.

struct AgentState {
    Vector2 position;
    Vector2 velocity;
    float   max_speed;
};

Vector2 seek (const AgentState& self, Vector2 target);
Vector2 flee (const AgentState& self, Vector2 threat);
Vector2 pursue(const AgentState& self, Vector2 target_pos, Vector2 target_vel);
Vector2 evade (const AgentState& self, Vector2 threat_pos, Vector2 threat_vel);

// Heading-noise wander. `phase` is the persistent state the AgentState
// owns; pass it in and out so behavior is smooth across frames.
Vector2 wander(const AgentState& self, float& phase, float jitter, float radius, float dt);

// Average flee vector from all neighbors within `radius`. Use to keep bots
// from clumping with friendlies (roadmap §6.1).
Vector2 separation(const AgentState& self,
                   const Vector2* neighbor_positions,
                   std::size_t    neighbor_count,
                   float          radius);

// Cheap obstacle avoidance via a forward "feeler" against arena bounds.
// Returns a perpendicular nudge if the feeler exits the rect, else zero.
Vector2 avoid_bounds(const AgentState& self, Rectangle bounds, float feeler_length);

}  // namespace vector::game::ai
