#pragma once

#include <cstdint>

namespace vector::game {

struct BulletComponent {
    float    damage     {8.0f};
    unsigned owner_id   {0};  // entity id of the firing ship; underflow-safe
};

// Generic "kill me at T" marker; LifetimeSystem decrements every tick.
struct LifetimeComponent {
    float seconds_remaining{1.0f};
};

}  // namespace vector::game
