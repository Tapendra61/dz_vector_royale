#include "game/data/tuning.h"

#include "core/logging.h"

namespace vector::game {

namespace {

template <typename T>
T pick(const core::Config& cfg, std::string_view key, T fallback) {
    return cfg.get<T>(key, fallback);
}

}  // namespace

Tuning Tuning::from_config(const core::Config& cfg) {
    Tuning t;

    t.ship.thrust_accel     = pick<float>(cfg, "ship.thrust_accel",     t.ship.thrust_accel);
    t.ship.angular_accel    = pick<float>(cfg, "ship.angular_accel",    t.ship.angular_accel);
    t.ship.linear_damping   = pick<float>(cfg, "ship.linear_damping",   t.ship.linear_damping);
    t.ship.angular_damping  = pick<float>(cfg, "ship.angular_damping",  t.ship.angular_damping);
    t.ship.max_speed        = pick<float>(cfg, "ship.max_speed",        t.ship.max_speed);
    t.ship.radius           = pick<float>(cfg, "ship.radius",           t.ship.radius);
    t.ship.hp_max           = pick<float>(cfg, "ship.hp_max",           t.ship.hp_max);
    t.ship.hp_regen_rate    = pick<float>(cfg, "ship.hp_regen_rate",    t.ship.hp_regen_rate);
    t.ship.hp_regen_delay   = pick<float>(cfg, "ship.hp_regen_delay",   t.ship.hp_regen_delay);
    t.ship.boost_multiplier = pick<float>(cfg, "ship.boost_multiplier", t.ship.boost_multiplier);
    t.ship.boost_duration   = pick<float>(cfg, "ship.boost_duration",   t.ship.boost_duration);
    t.ship.boost_cooldown   = pick<float>(cfg, "ship.boost_cooldown",   t.ship.boost_cooldown);
    t.ship.spawn_protect_s  = pick<float>(cfg, "ship.spawn_protect_s",  t.ship.spawn_protect_s);

    t.weapon.fire_rate        = pick<float>(cfg, "weapon.fire_rate",        t.weapon.fire_rate);
    t.weapon.bullet_speed     = pick<float>(cfg, "weapon.bullet_speed",     t.weapon.bullet_speed);
    t.weapon.bullet_lifetime  = pick<float>(cfg, "weapon.bullet_lifetime",  t.weapon.bullet_lifetime);
    t.weapon.bullet_damage    = pick<float>(cfg, "weapon.bullet_damage",    t.weapon.bullet_damage);
    t.weapon.bullet_radius    = pick<float>(cfg, "weapon.bullet_radius",    t.weapon.bullet_radius);

    t.arena.width       = pick<float>(cfg, "arena.width",       t.arena.width);
    t.arena.height      = pick<float>(cfg, "arena.height",      t.arena.height);
    t.arena.wall_bounce = pick<float>(cfg, "arena.wall_bounce", t.arena.wall_bounce);
    t.arena.wall_damage = pick<float>(cfg, "arena.wall_damage", t.arena.wall_damage);

    t.pickup.repair_amount    = pick<float>(cfg, "pickup.repair_amount",    t.pickup.repair_amount);
    t.pickup.spawn_cooldown   = pick<float>(cfg, "pickup.spawn_cooldown",   t.pickup.spawn_cooldown);
    t.pickup.spawn_jitter     = pick<float>(cfg, "pickup.spawn_jitter",     t.pickup.spawn_jitter);
    t.pickup.despawn_timeout  = pick<float>(cfg, "pickup.despawn_timeout",  t.pickup.despawn_timeout);
    t.pickup.radius           = pick<float>(cfg, "pickup.radius",           t.pickup.radius);

    return t;
}

Tuning Tuning::load(const std::filesystem::path& path) {
    if (auto cfg = core::Config::load_from_file(path)) {
        VECTOR_INFO("tuning: loaded {}", path.string());
        return from_config(*cfg);
    }
    VECTOR_WARN("tuning: using defaults (no file at {})", path.string());
    return defaults();
}

}  // namespace vector::game
