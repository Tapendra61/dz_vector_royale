#pragma once

namespace vector::game {

struct HealthComponent {
    float current        {100.0f};
    float max            {100.0f};
    float regen_rate     {  8.0f};  // hp/sec when out of combat
    float regen_delay    {  3.0f};  // seconds since last damage before regen kicks in
    float time_since_dmg {999.0f};  // start "out of combat" so initial spawn doesn't wait

    bool  is_dead()         const { return current <= 0.0f; }
    bool  is_in_combat()    const { return time_since_dmg < regen_delay; }
    float fraction()        const { return max > 0.0f ? current / max : 0.0f; }
};

// Marker emitted by combat resolution; consumed by HealthSystem next tick.
struct DamageEvent {
    float    amount   {0.0f};
    unsigned source   {0};
};

}  // namespace vector::game
