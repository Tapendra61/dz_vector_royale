#pragma once

#include <raylib.h>

namespace vector::render {

// Smooth-follow Camera2D with velocity-based look-ahead, trauma-based
// screen shake (per Squirrel Eiserloh's "Juicing Your Cameras" talk), and
// speed-modulated zoom-out for sense-of-speed.
class FollowCamera {
public:
    FollowCamera();

    void update(Vector2 target_pos, Vector2 target_velocity, float dt);
    void onWindowResize(int width, int height);

    // Add to the "trauma" accumulator; 1.0 = full screen shake, 0 = idle.
    // Caller adds smaller values for near-misses, larger for kills.
    void add_trauma(float amount);

    Camera2D& raw() { return cam_; }
    const Camera2D& raw() const { return cam_; }

private:
    Camera2D cam_;
    Vector2  smoothed_target_{0.0f, 0.0f};
    float    follow_lerp_     {6.0f};
    float    lookahead_factor_{0.35f};

    // Shake state.
    float    trauma_          {0.0f};
    float    shake_phase_     {0.0f};

    // Speed-zoom state.
    float    base_zoom_       {1.0f};
    float    current_zoom_    {1.0f};
};

}  // namespace vector::render
