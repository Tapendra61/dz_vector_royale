#include "game/systems/physics_system.h"

#include "game/arena.h"
#include "game/components/physics.h"
#include "game/components/transform.h"

#include <raymath.h>

#include <algorithm>
#include <cmath>

namespace vector::game {

namespace {

// Exponential damping that's frame-rate independent: v *= exp(-k*dt)
inline float damp(float v, float k, float dt) {
    return v * std::exp(-k * dt);
}

}  // namespace

void PhysicsSystem::tick(ecs::Registry& reg, const Arena& arena, float dt, float wall_bounce) {
    // Snapshot last position for the renderer to interpolate against.
    reg.view<TransformComponent, PrevTransformComponent>().each(
        [](TransformComponent& tr, PrevTransformComponent& prev) {
            prev.position = tr.position;
            prev.rotation = tr.rotation;
        });

    reg.view<TransformComponent, VelocityComponent>().each(
        [&](ecs::Entity e, TransformComponent& tr, VelocityComponent& vel) {
            // Optional rigid-body damping & clamp.
            if (auto* rb = reg.try_get<RigidBodyComponent>(e)) {
                vel.linear.x = damp(vel.linear.x, rb->linear_damping,  dt);
                vel.linear.y = damp(vel.linear.y, rb->linear_damping,  dt);
                vel.angular  = damp(vel.angular,  rb->angular_damping, dt);
                const float sp2 = vel.linear.x * vel.linear.x + vel.linear.y * vel.linear.y;
                if (sp2 > rb->max_speed * rb->max_speed) {
                    const float s = rb->max_speed / std::sqrt(sp2);
                    vel.linear.x *= s;
                    vel.linear.y *= s;
                }
            }

            tr.position.x += vel.linear.x * dt;
            tr.position.y += vel.linear.y * dt;
            tr.rotation   += vel.angular  * dt;

            // Keep yaw bounded to [-PI, PI] so atan2 deltas behave.
            while (tr.rotation >  PI) tr.rotation -= 2.0f * PI;
            while (tr.rotation < -PI) tr.rotation += 2.0f * PI;

            // Arena bounds — bounce or clamp depending on tuning.
            const float r = reg.try_get<ColliderComponent>(e) ? reg.get<ColliderComponent>(e).radius : 0.0f;
            const auto b = arena.bounds();
            if (tr.position.x < b.x + r)         { tr.position.x = b.x + r;          vel.linear.x = -vel.linear.x * wall_bounce; }
            if (tr.position.y < b.y + r)         { tr.position.y = b.y + r;          vel.linear.y = -vel.linear.y * wall_bounce; }
            if (tr.position.x > b.x + b.width - r)  { tr.position.x = b.x + b.width - r;  vel.linear.x = -vel.linear.x * wall_bounce; }
            if (tr.position.y > b.y + b.height - r) { tr.position.y = b.y + b.height - r; vel.linear.y = -vel.linear.y * wall_bounce; }
        });
}

}  // namespace vector::game
