#include "render/camera.h"

#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <random>

namespace vector::render {

namespace {

std::mt19937& prng() {
    thread_local std::mt19937 g{std::random_device{}() ^ 0xFEEDF00Du};
    return g;
}

float frand(float lo, float hi) {
    std::uniform_real_distribution<float> d(lo, hi);
    return d(prng());
}

}  // namespace

FollowCamera::FollowCamera() {
    cam_.target   = {0.0f, 0.0f};
    cam_.offset   = {0.0f, 0.0f};
    cam_.rotation = 0.0f;
    cam_.zoom     = 1.0f;
}

void FollowCamera::onWindowResize(int width, int height) {
    cam_.offset = {width * 0.5f, height * 0.5f};
}

void FollowCamera::add_trauma(float amount) {
    trauma_ = std::clamp(trauma_ + amount, 0.0f, 1.0f);
}

void FollowCamera::update(Vector2 target_pos, Vector2 target_velocity, float dt) {
    const Vector2 ahead = Vector2Scale(target_velocity, lookahead_factor_);
    const Vector2 want  = Vector2Add(target_pos, ahead);
    const float   t     = 1.0f - std::exp(-follow_lerp_ * dt);
    smoothed_target_    = Vector2Lerp(smoothed_target_, want, t);

    // Speed-modulated zoom-out — at top speed shave ~12% off the zoom.
    const float speed       = Vector2Length(target_velocity);
    const float speed_norm  = std::clamp(speed / 360.0f, 0.0f, 1.0f);
    const float target_zoom = base_zoom_ * (1.0f - 0.12f * speed_norm);
    current_zoom_           = current_zoom_ + (target_zoom - current_zoom_) * (1.0f - std::exp(-4.0f * dt));

    // Trauma decays linearly toward 0 each frame.
    trauma_       = std::max(0.0f, trauma_ - 1.6f * dt);
    shake_phase_ += dt * 60.0f;
    const float shake_amount = trauma_ * trauma_;
    Vector2 shake{0, 0};
    if (shake_amount > 0.0f) {
        const float radius = 18.0f * shake_amount;
        shake.x = std::sin(shake_phase_ * 1.13f) * radius * frand(-1.0f, 1.0f);
        shake.y = std::cos(shake_phase_ * 0.97f) * radius * frand(-1.0f, 1.0f);
    }

    cam_.target = Vector2Add(smoothed_target_, shake);
    cam_.zoom   = current_zoom_;
}

}  // namespace vector::render
