#pragma once

namespace vector::game {

// Holds the fractional-particle accumulators for the continuous emitters
// attached to an entity. Bullets use `trail`; ships use `thrust`. Each
// emitter needs its own state — sharing one accumulator across entities
// produces uneven bursts.
struct EmitterStateComponent {
    float trail_acc   {0.0f};   // bullets
    float thrust_acc  {0.0f};   // ships
    float sparkle_acc {0.0f};   // pickups
};

}  // namespace vector::game
