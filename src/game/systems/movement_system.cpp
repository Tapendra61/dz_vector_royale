#include "game/systems/movement_system.h"

#include "game/components/physics.h"
#include "game/components/ship.h"
#include "game/components/transform.h"
#include "game/data/tuning.h"

#include <raymath.h>

#include <algorithm>
#include <cmath>

namespace vector::game {

void MovementSystem::apply_player_intent(ecs::Registry& reg,
                                        ecs::Entity ship,
                                        const InputIntent& intent,
                                        const ShipTuning& tuning,
                                        float dt) {
    if (!reg.valid(ship)) return;

    auto& tr  = reg.get<TransformComponent>(ship);
    auto& vel = reg.get<VelocityComponent>(ship);
    const auto& shipc = reg.get<ShipComponent>(ship);

    // Yaw: mouse aim is authoritative when present; otherwise A/D angular thrust.
    if (intent.aim_is_absolute) {
        const float target = std::atan2(intent.aim_direction.y, intent.aim_direction.x);
        float delta = target - tr.rotation;
        while (delta >  PI) delta -= 2.0f * PI;
        while (delta < -PI) delta += 2.0f * PI;
        // Map angle error to angular velocity with a soft cap.
        vel.angular = std::clamp(delta / dt, -shipc.angular_accel * 2.0f, shipc.angular_accel * 2.0f);
    } else {
        vel.angular += intent.steer.x * shipc.angular_accel * dt;
    }

    // Linear thrust along facing direction.
    if (intent.thrust > 0.0f) {
        const Vector2 facing{std::cos(tr.rotation), std::sin(tr.rotation)};
        const float   accel = shipc.thrust_accel * intent.thrust * dt;
        vel.linear.x += facing.x * accel;
        vel.linear.y += facing.y * accel;
    }

    (void)tuning;  // future: boost / lateral strafe land here
}

}  // namespace vector::game
