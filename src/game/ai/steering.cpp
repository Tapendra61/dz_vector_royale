#include "game/ai/steering.h"

#include <raymath.h>

#include <algorithm>
#include <cmath>

namespace vector::game::ai {

namespace {

inline Vector2 to_speed(Vector2 v, float speed) {
    const float l2 = v.x * v.x + v.y * v.y;
    if (l2 < 1e-6f) return v;
    const float inv = speed / std::sqrt(l2);
    return Vector2{v.x * inv, v.y * inv};
}

inline Vector2 sub(Vector2 a, Vector2 b) { return Vector2{a.x - b.x, a.y - b.y}; }
inline Vector2 add(Vector2 a, Vector2 b) { return Vector2{a.x + b.x, a.y + b.y}; }
inline Vector2 mul(Vector2 a, float s)   { return Vector2{a.x * s,    a.y * s};    }

}  // namespace

Vector2 seek(const AgentState& self, Vector2 target) {
    return sub(to_speed(sub(target, self.position), self.max_speed), self.velocity);
}

Vector2 flee(const AgentState& self, Vector2 threat) {
    return sub(to_speed(sub(self.position, threat), self.max_speed), self.velocity);
}

Vector2 pursue(const AgentState& self, Vector2 target_pos, Vector2 target_vel) {
    const float dist  = Vector2Distance(self.position, target_pos);
    const float t     = std::min(0.6f, dist / std::max(self.max_speed, 1.0f));
    const Vector2 predicted = add(target_pos, mul(target_vel, t));
    return seek(self, predicted);
}

Vector2 evade(const AgentState& self, Vector2 threat_pos, Vector2 threat_vel) {
    const float dist  = Vector2Distance(self.position, threat_pos);
    const float t     = std::min(0.6f, dist / std::max(self.max_speed, 1.0f));
    const Vector2 predicted = add(threat_pos, mul(threat_vel, t));
    return flee(self, predicted);
}

Vector2 wander(const AgentState& self, float& phase, float jitter, float radius, float dt) {
    phase += jitter * dt * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) - 0.5f) * 2.0f;
    const Vector2 forward = (Vector2LengthSqr(self.velocity) > 1.0f)
        ? Vector2Normalize(self.velocity)
        : Vector2{1.0f, 0.0f};
    const Vector2 circle_center = add(self.position, mul(forward, radius * 1.5f));
    const Vector2 displacement{std::cos(phase) * radius, std::sin(phase) * radius};
    const Vector2 target = add(circle_center, displacement);
    return seek(self, target);
}

Vector2 separation(const AgentState& self,
                   const Vector2* neighbor_positions,
                   std::size_t    neighbor_count,
                   float          radius) {
    Vector2 force{0, 0};
    int n = 0;
    for (std::size_t i = 0; i < neighbor_count; ++i) {
        const Vector2 diff = sub(self.position, neighbor_positions[i]);
        const float   d2   = diff.x * diff.x + diff.y * diff.y;
        if (d2 > 1e-6f && d2 < radius * radius) {
            const float w = 1.0f / std::sqrt(d2);
            force = add(force, mul(diff, w));
            ++n;
        }
    }
    if (n == 0) return Vector2{0, 0};
    return mul(force, self.max_speed / static_cast<float>(n));
}

Vector2 avoid_bounds(const AgentState& self, Rectangle bounds, float feeler_length) {
    const Vector2 forward = (Vector2LengthSqr(self.velocity) > 1.0f)
        ? Vector2Normalize(self.velocity)
        : Vector2{1.0f, 0.0f};
    const Vector2 ahead = add(self.position, mul(forward, feeler_length));

    Vector2 push{0, 0};
    constexpr float pad = 24.0f;
    if (ahead.x < bounds.x + pad)                    push.x +=  1.0f;
    if (ahead.x > bounds.x + bounds.width - pad)     push.x += -1.0f;
    if (ahead.y < bounds.y + pad)                    push.y +=  1.0f;
    if (ahead.y > bounds.y + bounds.height - pad)    push.y += -1.0f;

    if (push.x == 0.0f && push.y == 0.0f) return push;
    return to_speed(push, self.max_speed);
}

}  // namespace vector::game::ai
