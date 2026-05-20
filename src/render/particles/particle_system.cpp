#include "render/particles/particle_system.h"

#include <raymath.h>
#include <rlgl.h>

#include <algorithm>
#include <cmath>
#include <random>

namespace vector::render {

namespace {

std::mt19937& prng() {
    thread_local std::mt19937 g{std::random_device{}() ^ 0xDEADBEEFu};
    return g;
}

float frand(float lo, float hi) {
    std::uniform_real_distribution<float> d(lo, hi);
    return d(prng());
}

int irand(int lo, int hi) {
    if (hi <= lo) return lo;
    std::uniform_int_distribution<int> d(lo, hi);
    return d(prng());
}

Color lerp_color(Color a, Color b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return Color{
        static_cast<unsigned char>(a.r + (b.r - a.r) * t),
        static_cast<unsigned char>(a.g + (b.g - a.g) * t),
        static_cast<unsigned char>(a.b + (b.b - a.b) * t),
        static_cast<unsigned char>(a.a + (b.a - a.a) * t)
    };
}

Vector2 rotate(Vector2 v, float rad) {
    const float c = std::cos(rad), s = std::sin(rad);
    return Vector2{v.x * c - v.y * s, v.x * s + v.y * c};
}

}  // namespace

ParticleSystem::ParticleSystem(std::size_t pool_capacity) : pool_(pool_capacity) {}

Particle& ParticleSystem::acquire_slot() {
    // Linear-probe for a free slot starting at cursor; if all live, overwrite
    // the oldest. Cursor wrap keeps the search bounded to O(capacity).
    for (std::size_t step = 0; step < pool_.size(); ++step) {
        const std::size_t i = (cursor_ + step) % pool_.size();
        if (!pool_[i].alive) {
            cursor_ = (i + 1) % pool_.size();
            return pool_[i];
        }
    }
    auto& p = pool_[cursor_];
    cursor_ = (cursor_ + 1) % pool_.size();
    return p;
}

void ParticleSystem::burst(const EmitterSpec& spec, Vector2 pos, Vector2 dir) {
    if (active_count_ >= max_active_) return;
    if (Vector2LengthSqr(dir) < 1e-6f) dir = {1.0f, 0.0f};
    dir = Vector2Normalize(dir);

    const int count = irand(spec.burst_min, spec.burst_max);
    for (int i = 0; i < count && active_count_ < max_active_; ++i) {
        Particle& p = acquire_slot();
        const float spread  = frand(-spec.spread_rad, spec.spread_rad);
        const Vector2 d     = rotate(dir, spread);
        const float speed   = frand(spec.speed_min, spec.speed_max);
        if (!p.alive) ++active_count_;
        p.alive       = true;
        p.position    = pos;
        p.velocity    = Vector2{d.x * speed, d.y * speed};
        p.age         = 0.0f;
        p.lifetime    = frand(spec.lifetime_min,   spec.lifetime_max);
        p.size_start  = frand(spec.size_start_min, spec.size_start_max);
        p.size_end    = frand(spec.size_end_min,   spec.size_end_max);
        p.color_start = spec.color_start;
        p.color_end   = spec.color_end;
        p.drag        = spec.drag;
    }
}

void ParticleSystem::continuous(const EmitterSpec& spec, Vector2 pos, Vector2 dir,
                                float dt, float& accumulator) {
    if (spec.rate_per_second <= 0.0f) return;
    accumulator += dt * spec.rate_per_second;
    while (accumulator >= 1.0f) {
        accumulator -= 1.0f;
        EmitterSpec one = spec;
        one.burst_min = 1;
        one.burst_max = 1;
        burst(one, pos, dir);
    }
}

void ParticleSystem::update(float dt) {
    active_count_ = 0;
    for (auto& p : pool_) {
        if (!p.alive) continue;
        p.age += dt;
        if (p.age >= p.lifetime) { p.alive = false; continue; }

        // Frame-rate-independent drag.
        const float k = std::exp(-p.drag * dt);
        p.velocity.x *= k;
        p.velocity.y *= k;
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        ++active_count_;
    }
}

void ParticleSystem::draw() const {
    // Additive blend looks better for sparks/exhaust. The composite shader
    // (bloom) will pick up the bright pixels and bloom them out.
    rlSetBlendMode(BLEND_ADDITIVE);
    for (const auto& p : pool_) {
        if (!p.alive) continue;
        const float t = p.age / p.lifetime;
        const Color c = lerp_color(p.color_start, p.color_end, t);
        const float s = p.size_start + (p.size_end - p.size_start) * t;
        DrawCircleV(p.position, std::max(0.5f, s), c);
    }
    rlSetBlendMode(BLEND_ALPHA);
}

}  // namespace vector::render
