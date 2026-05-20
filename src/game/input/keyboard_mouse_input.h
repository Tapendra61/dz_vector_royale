#pragma once

#include "game/input/input_backend.h"

namespace vector::game {

class KeyboardMouseInput final : public IInputBackend {
public:
    InputIntent poll(Vector2 player_world_pos, const Camera2D& camera) override;
    const char* name() const override { return "KeyboardMouse"; }
};

}  // namespace vector::game
