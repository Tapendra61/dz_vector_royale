#pragma once

#include <cstdint>

namespace vector::game {

// Roadmap §1.5 — the full enum is declared now even though Phase 1 only
// ships RepairPack. Later phases extend behavior; this header doesn't grow.
enum class PowerUpType : std::uint8_t {
    None = 0,
    // Offensive (Primary slot)
    Laser,
    IceBlast,
    Mine,
    Missile,
    ScatterShot,
    Railgun,
    // Defensive / Utility (Special slot)
    Shield,
    RepairPack,
    Cloak,
    SpeedBoost,
    EMP,
    Teleport,
};

enum class PowerUpSlot : std::uint8_t { Primary, Special };

PowerUpSlot slot_for(PowerUpType type);
const char* name_of(PowerUpType type);

// In-world pickup that has spawned and is waiting to be grabbed.
struct PickupComponent {
    PowerUpType type           {PowerUpType::RepairPack};
    float       despawn_in     {30.0f};  // seconds until it vanishes if untouched
    float       bob_phase      { 0.0f};  // visual-only animation seed
};

// Spawn point fixed in the arena. The pickup spawner ticks each one and
// emits a pickup when the cooldown expires (roadmap §1.5).
struct PickupSpawnPoint {
    float       cooldown        {12.0f};
    float       jitter          { 2.0f};
    float       next_spawn_in   { 0.0f};
    PowerUpType preferred_type  {PowerUpType::RepairPack};
};

}  // namespace vector::game
