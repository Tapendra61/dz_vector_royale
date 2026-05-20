#include "game/systems/movement_system.h"

#include "game/components/intent.h"
#include "game/components/physics.h"
#include "game/components/ship.h"
#include "game/components/status_effects.h"
#include "game/components/transform.h"

#include <raymath.h>

#include <algorithm>
#include <cmath>

namespace vector::game {

void MovementSystem::tick(ecs::Registry& reg, float dt) {
    reg.view<TransformComponent, VelocityComponent, ShipComponent, IntentComponent>().each(
        [&](ecs::Entity e, TransformComponent& tr, VelocityComponent& vel,
            const ShipComponent& shipc, const IntentComponent& intent_c) {
            const auto& intent = intent_c.value;

            // Frozen / stunned ships ignore their own intent.
            if (auto* sx = reg.try_get<StatusEffectComponent>(e)) {
                if (sx->has(StatusEffectType::Frozen) ||
                    sx->has(StatusEffectType::Stunned)) {
                    return;
                }
            }

            // Yaw: absolute aim is authoritative (mouse / AI target vector);
            // otherwise A/D angular thrust.
            if (intent.aim_is_absolute) {
                const float target = std::atan2(intent.aim_direction.y, intent.aim_direction.x);
                float delta = target - tr.rotation;
                while (delta >  PI) delta -= 2.0f * PI;
                while (delta < -PI) delta += 2.0f * PI;
                vel.angular = std::clamp(delta / dt,
                                         -shipc.angular_accel * 2.0f,
                                          shipc.angular_accel * 2.0f);
            } else {
                vel.angular += intent.steer.x * shipc.angular_accel * dt;
            }

            // Linear thrust along facing direction.
            if (intent.thrust > 0.0f && shipc.thrust_accel > 0.0f) {
                const Vector2 facing{std::cos(tr.rotation), std::sin(tr.rotation)};
                const float   accel = shipc.thrust_accel * intent.thrust * dt;
                vel.linear.x += facing.x * accel;
                vel.linear.y += facing.y * accel;
            }
        });
}

}  // namespace vector::game
