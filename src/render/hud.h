#pragma once

#include "ecs/registry.h"

namespace vector::game { class World; }

namespace vector::render {

// Roadmap §5.3: HP bar above ship, screen-edge red vignette below 25% HP,
// plus tiny crosshair-ish marker for aim. Real HUD lives in Phase 4.
class HUD {
public:
    void drawScreen(const game::World& world, int screen_w, int screen_h);
};

}  // namespace vector::render
