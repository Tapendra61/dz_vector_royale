#include "game/input/gamepad_input.h"

#include <raymath.h>

#include <cmath>

namespace vector::game {

namespace {
inline float apply_deadzone(float v, float dz) {
    if (std::fabs(v) < dz) return 0.0f;
    const float s = (v > 0.0f) ? 1.0f : -1.0f;
    return s * (std::fabs(v) - dz) / (1.0f - dz);
}
}  // namespace

bool GamepadInput::isAvailable() const {
    return IsGamepadAvailable(slot_);
}

InputIntent GamepadInput::poll(Vector2 /*player_world_pos*/, const Camera2D& /*camera*/) {
    InputIntent intent{};
    if (!IsGamepadAvailable(slot_)) return intent;

    const float lx = apply_deadzone(GetGamepadAxisMovement(slot_, GAMEPAD_AXIS_LEFT_X), kDeadzone);
    const float ly = apply_deadzone(GetGamepadAxisMovement(slot_, GAMEPAD_AXIS_LEFT_Y), kDeadzone);
    const float rx = apply_deadzone(GetGamepadAxisMovement(slot_, GAMEPAD_AXIS_RIGHT_X), kDeadzone);
    const float ry = apply_deadzone(GetGamepadAxisMovement(slot_, GAMEPAD_AXIS_RIGHT_Y), kDeadzone);
    const float rt = GetGamepadAxisMovement(slot_, GAMEPAD_AXIS_RIGHT_TRIGGER);

    intent.steer.x = lx;
    intent.thrust  = std::fmax(rt > -0.5f ? (rt + 1.0f) * 0.5f : 0.0f, -ly > 0.0f ? -ly : 0.0f);

    const Vector2 aim{rx, ry};
    if (Vector2LengthSqr(aim) > 0.0f) {
        intent.aim_direction   = Vector2Normalize(aim);
        intent.aim_is_absolute = true;
    }

    intent.fire        = IsGamepadButtonDown(slot_, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)
                      || IsGamepadButtonDown(slot_, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    intent.boost       = IsGamepadButtonDown(slot_, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
    intent.use_primary = IsGamepadButtonPressed(slot_, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
    intent.use_special = IsGamepadButtonPressed(slot_, GAMEPAD_BUTTON_LEFT_FACE_UP);

    return intent;
}

}  // namespace vector::game
