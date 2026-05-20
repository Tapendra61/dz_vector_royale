#pragma once

#include "game/input/input_intent.h"

#include <raylib.h>

namespace vector::game {

// Backends are polled once per frame and produce an InputIntent. The
// `camera` is needed for backends that translate screen-space input
// (mouse) into world-space aim vectors.
class IInputBackend {
public:
    virtual ~IInputBackend() = default;

    virtual InputIntent poll(Vector2 player_world_pos, const Camera2D& camera) = 0;
    virtual const char* name() const = 0;
};

}  // namespace vector::game
