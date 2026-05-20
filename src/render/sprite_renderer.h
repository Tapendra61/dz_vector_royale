#pragma once

#include "ecs/registry.h"

#include <raylib.h>

namespace vector::game { struct Arena; }

namespace vector::render {

// Phase 1 rendering: primitive shapes only. Phase 3 replaces this with
// sprites / shaders / particle emitters.
class SpriteRenderer {
public:
    void drawArena(const game::Arena& arena);
    void drawEntities(ecs::Registry& reg, float alpha);
};

}  // namespace vector::render
