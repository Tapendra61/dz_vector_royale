#pragma once

#include "game/input/input_backend.h"

namespace vector::game {

class GamepadInput final : public IInputBackend {
public:
    explicit GamepadInput(int slot = 0) : slot_(slot) {}

    InputIntent poll(Vector2 player_world_pos, const Camera2D& camera) override;
    const char* name() const override { return "Gamepad"; }

    bool isAvailable() const;

private:
    int slot_;
    static constexpr float kDeadzone = 0.18f;
};

}  // namespace vector::game
