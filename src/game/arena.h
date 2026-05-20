#pragma once

#include "game/data/tuning.h"

#include <raylib.h>

namespace vector::game {

struct Arena {
    float width  {2400.0f};
    float height {1600.0f};

    Rectangle bounds() const { return Rectangle{0.0f, 0.0f, width, height}; }
    Vector2   center() const { return Vector2{width * 0.5f, height * 0.5f}; }

    static Arena from_tuning(const ArenaTuning& t) {
        return Arena{t.width, t.height};
    }
};

}  // namespace vector::game
