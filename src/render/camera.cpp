#include "render/camera.h"

#include <raymath.h>

#include <cmath>

namespace vector::render {

FollowCamera::FollowCamera() {
    cam_.target   = {0.0f, 0.0f};
    cam_.offset   = {0.0f, 0.0f};
    cam_.rotation = 0.0f;
    cam_.zoom     = 1.0f;
}

void FollowCamera::onWindowResize(int width, int height) {
    cam_.offset = {width * 0.5f, height * 0.5f};
}

void FollowCamera::update(Vector2 target_pos, Vector2 target_velocity, float dt) {
    const Vector2 ahead = Vector2Scale(target_velocity, lookahead_factor_);
    const Vector2 want  = Vector2Add(target_pos, ahead);
    const float   t     = 1.0f - std::exp(-follow_lerp_ * dt);
    smoothed_target_    = Vector2Lerp(smoothed_target_, want, t);
    cam_.target         = smoothed_target_;
}

}  // namespace vector::render
