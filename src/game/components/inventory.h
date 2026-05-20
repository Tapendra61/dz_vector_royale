#pragma once

#include "game/components/pickup.h"

namespace vector::game {

// Roadmap §1.5: two slots — Primary (offensive) and Special (defensive /
// utility). `*_charge` semantics depend on the held power-up: shots
// remaining for projectiles, seconds for sustained effects, 1.0 for instant.
struct InventoryComponent {
    PowerUpType primary         {PowerUpType::None};
    PowerUpType special         {PowerUpType::None};
    float       primary_charge  {0.0f};
    float       special_charge  {0.0f};
};

}  // namespace vector::game
