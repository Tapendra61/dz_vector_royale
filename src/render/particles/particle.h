#pragma once

#include <raylib.h>

namespace vector::render {

// One in-flight particle. Pooled, never `new`-d in the hot loop.
struct Particle {
    Vector2 position {0.0f, 0.0f};
    Vector2 velocity {0.0f, 0.0f};
    float   age      {0.0f};
    float   lifetime {1.0f};
    float   size_start {3.0f};
    float   size_end   {0.0f};
    Color   color_start {RAYWHITE};
    Color   color_end   {Color{0, 0, 0, 0}};
    float   drag     {1.0f};   // per-second multiplier on velocity
    bool    alive    {false};
};

}  // namespace vector::render
