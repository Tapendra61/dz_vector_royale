#include "game/input/keyboard_mouse_input.h"

#include <raymath.h>

#include <algorithm>
#include <cmath>

namespace vector::game {

namespace {

// Distance to cursor (world units) at which thrust transitions from
// zero (cursor on top of ship → idle) to one (cursor far → full thrust).
constexpr float kIdleRadius     = 30.0f;
constexpr float kFullThrustDist = 150.0f;

}  // namespace

InputIntent KeyboardMouseInput::poll(Vector2 player_world_pos, const Camera2D& camera) {
    InputIntent intent{};

    // Steering keys (A/D) still feed steer.x for future settings-toggle of
    // keyboard turning. MovementSystem ignores them while aim_is_absolute.
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  intent.steer.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) intent.steer.x += 1.0f;

    intent.boost       = IsKeyDown(KEY_LEFT_SHIFT);
    intent.fire        = IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyDown(KEY_J);
    intent.use_primary = IsKeyPressed(KEY_Q);
    intent.use_special = IsKeyPressed(KEY_E) || IsKeyPressed(KEY_R);

    // Cursor-follow controls: ship continuously aims at the cursor and
    // auto-thrusts toward it, with a small idle radius so you can hover
    // the cursor near the ship to coast/stop.
    const Vector2 mouse_screen = GetMousePosition();
    const Vector2 mouse_world  = GetScreenToWorld2D(mouse_screen, camera);
    const Vector2 dir          = Vector2Subtract(mouse_world, player_world_pos);
    const float   dist2        = Vector2LengthSqr(dir);
    if (dist2 > 1.0f) {
        intent.aim_direction   = Vector2Normalize(dir);
        intent.aim_is_absolute = true;

        const float dist  = std::sqrt(dist2);
        intent.thrust     = std::clamp(
            (dist - kIdleRadius) / (kFullThrustDist - kIdleRadius), 0.0f, 1.0f);
    }

    return intent;
}

}  // namespace vector::game
