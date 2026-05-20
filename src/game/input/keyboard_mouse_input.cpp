#include "game/input/keyboard_mouse_input.h"

#include <raymath.h>

namespace vector::game {

InputIntent KeyboardMouseInput::poll(Vector2 player_world_pos, const Camera2D& camera) {
    InputIntent intent{};

    // Steering — A/D or arrow keys; held = full deflection.
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  intent.steer.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) intent.steer.x += 1.0f;

    // Thrust — W / Up / Space, all binary in Phase 1.
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) intent.thrust = 1.0f;

    intent.boost       = IsKeyDown(KEY_LEFT_SHIFT);
    intent.fire        = IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyDown(KEY_J);
    intent.use_primary = IsKeyPressed(KEY_Q);
    intent.use_special = IsKeyPressed(KEY_E) || IsKeyPressed(KEY_R);

    // Mouse provides absolute aim. The MovementSystem will use this to
    // override yaw if `aim_is_absolute` is set, while still letting A/D
    // strafe turn for keyboard-only players.
    const Vector2 mouse_screen = GetMousePosition();
    const Vector2 mouse_world  = GetScreenToWorld2D(mouse_screen, camera);
    Vector2 dir = Vector2Subtract(mouse_world, player_world_pos);
    if (Vector2LengthSqr(dir) > 1.0f) {
        intent.aim_direction  = Vector2Normalize(dir);
        intent.aim_is_absolute = true;
    }

    return intent;
}

}  // namespace vector::game
