#pragma once

#include <raylib.h>

#include <filesystem>

namespace vector::render {

// Manages screen-sized render targets and the bloom + vignette + low-HP
// composite pass. One instance lives in main; on window resize it
// recreates targets at the new size.
class PostFX {
public:
    PostFX();
    ~PostFX();

    PostFX(const PostFX&)            = delete;
    PostFX& operator=(const PostFX&) = delete;

    bool init(int width, int height, const std::filesystem::path& shader_dir);
    void resize(int width, int height);

    // Draw the world into the scene RT. Caller wraps gameplay rendering
    // between begin_scene() / end_scene().
    void begin_scene();
    void end_scene();

    // Present the composite to the back buffer. `low_hp` is 0..1.
    void present(float low_hp, bool bloom_enabled);

    bool ready() const { return ready_; }

private:
    void cleanup();
    void load_shaders(const std::filesystem::path& shader_dir);

    bool             ready_      {false};
    int              w_          {0};
    int              h_          {0};
    RenderTexture2D  scene_      {};
    RenderTexture2D  bright_     {};
    RenderTexture2D  blur_a_     {};
    RenderTexture2D  blur_b_     {};
    Shader           bright_shader_{};
    Shader           blur_shader_  {};
    Shader           composite_   {};
    int              bright_threshold_loc_ {-1};
    int              bright_knee_loc_      {-1};
    int              blur_direction_loc_   {-1};
    int              comp_bloom_tex_loc_   {-1};
    int              comp_bloom_int_loc_   {-1};
    int              comp_vignette_loc_    {-1};
    int              comp_low_hp_loc_      {-1};
};

}  // namespace vector::render
