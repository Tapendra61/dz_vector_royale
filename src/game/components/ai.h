#pragma once

#include "ecs/registry.h"

#include <raylib.h>

#include <cstdint>

namespace vector::game {

enum class BotArchetype : std::uint8_t { Sniper, Brawler, Runner };

// Per-bot AI state. The BT writes its high-level intent into these fields;
// the steering pass each tick reads them and produces a desired velocity.
struct AIControllerComponent {
    BotArchetype archetype     {BotArchetype::Brawler};

    // BT tuning (data-driven per archetype + difficulty).
    float reaction_delay_s     {0.20f};   // seconds before BT reacts to a stimulus
    float aim_noise_rad        {0.10f};   // gaussian wobble on aim
    float aggression           {0.7f};    // 0..1 weight of pursue vs flee
    float low_hp_threshold     {0.35f};   // flee/heal when HP fraction <= this
    float preferred_range      {280.0f};  // ideal distance to target
    float fire_range           {520.0f};  // open-fire when target inside this

    // Per-tick scratch (filled by BT, consumed by steering).
    ecs::Entity target          {ecs::null_entity};
    Vector2     target_pos      {0.0f, 0.0f};
    Vector2     target_vel      {0.0f, 0.0f};
    Vector2     desired_velocity{0.0f, 0.0f};
    bool        want_fire       {false};
    bool        want_use_special{false};

    // Cadenced state.
    float wander_phase        {0.0f};
    float bt_tick_in_s        {0.0f};     // counts down to next BT tick
    float aim_lag_s           {0.0f};     // reaction delay buffer for this bot
};

}  // namespace vector::game
