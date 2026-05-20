#pragma once

#include <array>
#include <cstdint>

namespace vector::game {

// Roadmap §1.4. Full list declared so the enum is stable across phases;
// only `Invulnerable` is actually emitted in Phase 1 (spawn protection).
enum class StatusEffectType : std::uint8_t {
    None = 0,
    Frozen,
    Slowed,
    Burning,
    Stunned,
    Disabled,
    Cloaked,
    Invulnerable,
};

struct StatusEffect {
    StatusEffectType type           {StatusEffectType::None};
    float            time_remaining {0.0f};
};

// Fixed-size to keep this trivially copyable and net-friendly later.
constexpr std::size_t kMaxStatusEffectsPerEntity = 6;

struct StatusEffectComponent {
    std::array<StatusEffect, kMaxStatusEffectsPerEntity> effects{};

    bool has(StatusEffectType t) const {
        for (const auto& e : effects) if (e.type == t && e.time_remaining > 0.0f) return true;
        return false;
    }
};

}  // namespace vector::game
