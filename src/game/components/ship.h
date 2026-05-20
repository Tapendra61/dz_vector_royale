#pragma once

namespace vector::game {

// Marks an entity as a controllable ship. The intent slot is filled by
// whichever input backend owns this ship (player, AI, replay) — gameplay
// systems read this and never the raw input devices (roadmap §5.2).
struct ShipComponent {
    float thrust_accel  {320.0f};   // units/s^2 applied along facing
    float angular_accel {  6.0f};   // rad/s^2
};

// Local-player tag; only one ship has it at a time.
struct LocalPlayerTag {};

// Stationary target dummy used in Phase 1 to validate HP/damage tuning.
// Phase 2 replaces this with real AI.
struct TargetDummyTag {
    float respawn_after{2.0f};
    float respawn_timer{0.0f};
};

}  // namespace vector::game
