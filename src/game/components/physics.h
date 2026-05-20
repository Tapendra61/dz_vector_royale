#pragma once

namespace vector::game {

struct RigidBodyComponent {
    float linear_damping  {0.30f};   // per-second
    float angular_damping {4.00f};
    float max_speed       {350.0f};  // soft clamp
};

struct ColliderComponent {
    float radius{12.0f};
};

}  // namespace vector::game
