#pragma once

#include <raylib.h>

namespace vector::render {

// Smooth-follow Camera2D with velocity-based look-ahead (roadmap §5.3 /
// §7.4 — full tuning happens in Phase 3, this is the minimum viable rig).
class FollowCamera {
public:
    FollowCamera();

    void update(Vector2 target_pos, Vector2 target_velocity, float dt);
    void onWindowResize(int width, int height);

    Camera2D& raw() { return cam_; }
    const Camera2D& raw() const { return cam_; }

private:
    Camera2D cam_;
    Vector2  smoothed_target_{0.0f, 0.0f};
    float    follow_lerp_     {6.0f};   // higher = snappier
    float    lookahead_factor_{0.35f};
};

}  // namespace vector::render
