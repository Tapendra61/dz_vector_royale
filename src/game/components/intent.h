#pragma once

#include "game/input/input_intent.h"

namespace vector::game {

// One-per-ship slot the input source (player backend or AI) writes into,
// and the Movement / Weapon / Pickup systems read from. This is the
// concrete realization of roadmap §6.1 — "AI ships drive themselves
// through the exact same code path as humans."
struct IntentComponent {
    InputIntent value{};
};

}  // namespace vector::game
