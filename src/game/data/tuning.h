#pragma once

#include "core/config.h"

#include <filesystem>

namespace vector::game {

// Roadmap §3.3 — every constant a designer cares about lives in JSON.
// Tuning is loaded once at startup; the fields below are the single source
// of truth that systems read.
struct ShipTuning {
    float thrust_accel    {320.0f};
    float angular_accel   {  6.0f};
    float linear_damping  {  0.30f};
    float angular_damping {  4.0f};
    float max_speed       {350.0f};
    float radius          { 18.0f};
    float hp_max          {100.0f};
    float hp_regen_rate   {  8.0f};
    float hp_regen_delay  {  3.0f};
    float boost_multiplier{  1.8f};
    float boost_duration  {  0.5f};
    float boost_cooldown  {  3.0f};
    float spawn_protect_s {  1.5f};
};

struct WeaponTuning {
    float fire_rate       {  4.5f};
    float bullet_speed    {800.0f};
    float bullet_lifetime {  1.2f};
    float bullet_damage   {  8.0f};
    float bullet_radius   {  3.0f};
};

struct ArenaTuning {
    float width        {6000.0f};
    float height       {4000.0f};
    float wall_bounce  {   0.4f};   // 0 = absorb, 1 = perfect bounce
    float wall_damage  {   0.0f};   // hp/sec while crossing the bound (unused if bounce > 0)
};

struct PickupTuning {
    float repair_amount   {40.0f};
    float spawn_cooldown  {12.0f};
    float spawn_jitter    { 2.0f};
    float despawn_timeout {30.0f};
    float radius          {16.0f};
};

struct Tuning {
    ShipTuning   ship;
    WeaponTuning weapon;
    ArenaTuning  arena;
    PickupTuning pickup;

    static Tuning defaults() { return Tuning{}; }
    static Tuning load(const std::filesystem::path& path);
    static Tuning from_config(const core::Config& cfg);
};

}  // namespace vector::game
