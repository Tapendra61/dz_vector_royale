#pragma once

#include <cstdint>

namespace vector::game {

// Phase 2 has no factions — just "who can target whom". Phase 4+ will
// expand this to clans / FFA modes.
enum class Team : std::uint8_t {
    Player  = 0,
    Bot     = 1,
    Neutral = 2,   // target dummies, training targets — bots ignore
};

struct TeamComponent {
    Team team{Team::Neutral};
};

}  // namespace vector::game
