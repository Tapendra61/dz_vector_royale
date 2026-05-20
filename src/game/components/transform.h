#pragma once

#include <raylib.h>

namespace vector::game {

struct TransformComponent {
    Vector2 position{0.0f, 0.0f};
    float   rotation{0.0f};  // radians, 0 = facing +X
};

struct VelocityComponent {
    Vector2 linear{0.0f, 0.0f};
    float   angular{0.0f};  // rad/s
};

// Snapshot of the previous tick's transform — used by the renderer to
// interpolate between sim ticks at the render rate (roadmap §2.2).
struct PrevTransformComponent {
    Vector2 position{0.0f, 0.0f};
    float   rotation{0.0f};
};

}  // namespace vector::game
