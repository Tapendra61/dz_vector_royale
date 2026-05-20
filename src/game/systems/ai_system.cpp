#include "game/systems/ai_system.h"

#include "game/ai/behavior_tree.h"
#include "game/ai/blackboard.h"
#include "game/ai/bot_brain.h"
#include "game/ai/steering.h"
#include "game/arena.h"
#include "game/components/ai.h"
#include "game/components/health.h"
#include "game/components/intent.h"
#include "game/components/inventory.h"
#include "game/components/physics.h"
#include "game/components/pickup.h"
#include "game/components/team.h"
#include "game/components/transform.h"
#include "game/data/tuning.h"

#include <raymath.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <random>

namespace vector::game {

struct AISystem::TreeBundle {
    std::array<ai::NodePtr, 3> by_archetype;  // index = static_cast<int>(BotArchetype)
};

namespace {

inline int idx(BotArchetype a) { return static_cast<int>(a); }

std::mt19937& prng() {
    thread_local std::mt19937 g{std::random_device{}() ^ 0x9E3779B9u};
    return g;
}

float aim_noise(float spread) {
    std::normal_distribution<float> d(0.0f, spread);
    return d(prng());
}

// "Best target" = closest entity with HP that is *not* on our team and
// *not* on the Neutral team (so bots don't shoot dummies, just the player
// and each other if they're on different teams).
struct Perception {
    ecs::Entity nearest_enemy   {ecs::null_entity};
    Vector2     enemy_pos       {0, 0};
    Vector2     enemy_vel       {0, 0};
    float       enemy_distance  {1e9f};

    ecs::Entity nearest_pickup  {ecs::null_entity};
    Vector2     pickup_pos      {0, 0};
    float       pickup_distance {1e9f};
};

Perception perceive(ecs::Registry& reg, ecs::Entity self) {
    Perception p;
    const auto& self_tr = reg.get<TransformComponent>(self);
    const auto* self_team = reg.try_get<TeamComponent>(self);

    // Enemies: any ship with HP and a different team.
    reg.view<TransformComponent, VelocityComponent, HealthComponent, TeamComponent>().each(
        [&](ecs::Entity e, const TransformComponent& tr, const VelocityComponent& vel,
            const HealthComponent& hp, const TeamComponent& team) {
            if (e == self) return;
            if (hp.is_dead()) return;
            if (team.team == Team::Neutral) return;
            if (self_team && team.team == self_team->team) return;
            const float dx = tr.position.x - self_tr.position.x;
            const float dy = tr.position.y - self_tr.position.y;
            const float d2 = dx * dx + dy * dy;
            if (d2 < p.enemy_distance * p.enemy_distance) {
                p.nearest_enemy  = e;
                p.enemy_pos      = tr.position;
                p.enemy_vel      = vel.linear;
                p.enemy_distance = std::sqrt(d2);
            }
        });

    // Pickups within ~600u: bots are pickup-aware (roadmap §6.3).
    reg.view<PickupComponent, TransformComponent>().each(
        [&](ecs::Entity e, const PickupComponent&, const TransformComponent& tr) {
            const float dx = tr.position.x - self_tr.position.x;
            const float dy = tr.position.y - self_tr.position.y;
            const float d2 = dx * dx + dy * dy;
            constexpr float kSightSq = 600.0f * 600.0f;
            if (d2 < kSightSq && d2 < p.pickup_distance * p.pickup_distance) {
                p.nearest_pickup  = e;
                p.pickup_pos      = tr.position;
                p.pickup_distance = std::sqrt(d2);
            }
        });

    return p;
}

InputIntent desired_velocity_to_intent(const AIControllerComponent& ctrl,
                                       Vector2 self_pos,
                                       float   max_speed) {
    InputIntent intent{};

    // Convert desired velocity into thrust + aim direction.
    Vector2 dv = ctrl.desired_velocity;
    const float speed = std::sqrt(dv.x * dv.x + dv.y * dv.y);
    Vector2 facing_dir{1.0f, 0.0f};

    if (ctrl.target != ecs::null_entity) {
        // Aim at the target (with reaction-noise applied each BT tick later).
        Vector2 to_target = Vector2Subtract(ctrl.target_pos, self_pos);
        if (Vector2LengthSqr(to_target) > 1.0f) {
            facing_dir = Vector2Normalize(to_target);
        }
        intent.thrust = (speed > 1.0f) ? std::clamp(speed / max_speed, 0.0f, 1.0f) : 0.4f;
    } else if (speed > 1.0f) {
        facing_dir = Vector2Scale(dv, 1.0f / speed);
        intent.thrust = std::clamp(speed / max_speed, 0.2f, 1.0f);
    } else {
        intent.thrust = 0.0f;
    }

    intent.aim_direction   = facing_dir;
    intent.aim_is_absolute = true;
    intent.fire            = ctrl.want_fire;
    intent.use_special     = ctrl.want_use_special;

    return intent;
}

}  // namespace

AISystem::AISystem()
    : trees_(new TreeBundle{{
          ai::build_sniper_tree(),
          ai::build_brawler_tree(),
          ai::build_runner_tree(),
      }}) {}

AISystem::~AISystem() { delete trees_; }

void AISystem::tick(ecs::Registry& reg,
                    const Arena& arena,
                    const ShipTuning& ship_tune,
                    const WeaponTuning& /*weapon_tune*/,
                    float dt) {
    auto view = reg.view<AIControllerComponent, TransformComponent, VelocityComponent,
                          IntentComponent, HealthComponent>();

    for (auto self : view) {
        auto& ctrl   = view.get<AIControllerComponent>(self);
        auto& tr     = view.get<TransformComponent>(self);
        auto& vel    = view.get<VelocityComponent>(self);
        auto& intent = view.get<IntentComponent>(self);
        auto& hp     = view.get<HealthComponent>(self);

        // Reset per-tick "scratch" outputs. The BT will overwrite these
        // when it ticks; between BT ticks we keep the previous goal.
        ctrl.want_use_special = false;  // edge-triggered

        // Optionally tick the BT (10 Hz with stagger per bot).
        ctrl.bt_tick_in_s -= dt;
        if (ctrl.bt_tick_in_s <= 0.0f) {
            ctrl.bt_tick_in_s = bt_period_s_;

            const auto perception = perceive(reg, self);

            ai::Blackboard bb{};
            bb.reg                 = &reg;
            bb.self                = self;
            bb.self_tr             = &tr;
            bb.self_vel            = &vel;
            bb.self_hp             = &hp;
            bb.self_inv            = reg.try_get<InventoryComponent>(self);
            bb.ctrl                = &ctrl;
            bb.best_target         = perception.nearest_enemy;
            bb.best_target_pos     = perception.enemy_pos;
            bb.best_target_vel     = perception.enemy_vel;
            bb.best_target_distance = perception.enemy_distance;
            bb.nearest_pickup      = perception.nearest_pickup;
            bb.nearest_pickup_pos  = perception.pickup_pos;
            bb.nearest_pickup_dist = perception.pickup_distance;
            bb.arena_bounds        = arena.bounds();
            bb.dt_seconds          = bt_period_s_;

            if (auto& tree = trees_->by_archetype[idx(ctrl.archetype)]) {
                tree->evaluate(bb);
            }

            // Apply aim noise — done per BT tick (not per sim tick) so it
            // visibly shifts when the bot "recalculates".
            if (ctrl.target != ecs::null_entity && ctrl.aim_noise_rad > 0.0f) {
                const float angle = aim_noise(ctrl.aim_noise_rad);
                const float c     = std::cos(angle), s = std::sin(angle);
                const Vector2 v   = Vector2Subtract(ctrl.target_pos, tr.position);
                ctrl.target_pos.x = tr.position.x + v.x * c - v.y * s;
                ctrl.target_pos.y = tr.position.y + v.x * s + v.y * c;
            }
        }

        // Steering pass — runs every sim tick. If the BT chose a wander
        // (target=null AND desired_velocity≈0), inject a wander vector.
        ai::AgentState state{tr.position, vel.linear, ship_tune.max_speed};
        if (ctrl.target == ecs::null_entity
            && std::fabs(ctrl.desired_velocity.x) < 1.0f
            && std::fabs(ctrl.desired_velocity.y) < 1.0f) {
            ctrl.desired_velocity = ai::wander(state, ctrl.wander_phase, 0.8f, 90.0f, dt);
        }

        // Always avoid arena walls.
        const Vector2 avoid = ai::avoid_bounds(state, arena.bounds(), ship_tune.radius * 4.0f);
        if (avoid.x != 0.0f || avoid.y != 0.0f) {
            ctrl.desired_velocity = Vector2Add(ctrl.desired_velocity, avoid);
        }

        intent.value = desired_velocity_to_intent(ctrl, tr.position, ship_tune.max_speed);
    }
}

}  // namespace vector::game
