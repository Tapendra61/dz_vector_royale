#pragma once

#include <raylib.h>

namespace vector::game {

// Roadmap §5.2 — the *only* input contract game systems see. Multiple
// backends (KeyboardMouse, Gamepad, Touch later, AI later) produce this
// struct. Gameplay code never reads the OS input devices directly.
struct InputIntent {
    Vector2 steer        {0.0f, 0.0f};  // each axis -1..1; .x = turn, .y reserved
    Vector2 aim_direction{1.0f, 0.0f};  // unit vector (mouse-relative or stick)
    bool    aim_is_absolute{false};     // true => use aim_direction directly; false => use steer for facing
    float   thrust       {0.0f};        // 0..1
    bool    fire         {false};
    bool    boost        {false};
    bool    use_primary  {false};       // activate held Primary slot power-up
    bool    use_special  {false};       // activate held Special slot power-up
};

}  // namespace vector::game
