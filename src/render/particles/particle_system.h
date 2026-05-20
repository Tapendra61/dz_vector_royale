#pragma once

#include "render/particles/emitter_spec.h"
#include "render/particles/particle.h"

#include <raylib.h>

#include <cstddef>
#include <vector>

namespace vector::render {

class ParticleSystem {
public:
    explicit ParticleSystem(std::size_t pool_capacity = 4096);

    // One-off burst at position emitting along `dir` (unit vector).
    void burst(const EmitterSpec& spec, Vector2 pos, Vector2 dir);

    // Continuous emission helper — pass `dt` and a state float that the
    // caller persists per-emitter to accumulate fractional spawns.
    void continuous(const EmitterSpec& spec, Vector2 pos, Vector2 dir,
                    float dt, float& accumulator);

    void update(float dt);
    void draw() const;

    std::size_t active_count() const { return active_count_; }
    std::size_t pool_capacity() const { return pool_.size(); }

    // Graphics-preset hook: how many particles to keep alive at once.
    void set_max_active(std::size_t n) { max_active_ = n; }

private:
    Particle& acquire_slot();

    std::vector<Particle> pool_;
    std::size_t           cursor_       {0};
    std::size_t           active_count_ {0};
    std::size_t           max_active_   {2048};
};

}  // namespace vector::render
