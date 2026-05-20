#include "render/post_fx.h"

#include "core/logging.h"

#include <rlgl.h>

namespace vector::render {

namespace {

// Draws a screen-filling textured quad of the named RT into whatever the
// current draw target is. Used to chain shader passes.
void blit_rt(RenderTexture2D src, int target_w, int target_h, Shader sh) {
    BeginShaderMode(sh);
    // Flip Y because raylib's RenderTexture origin is bottom-left.
    Rectangle srcRect{0, 0, static_cast<float>(src.texture.width),
                            -static_cast<float>(src.texture.height)};
    Rectangle dstRect{0, 0, static_cast<float>(target_w),
                            static_cast<float>(target_h)};
    DrawTexturePro(src.texture, srcRect, dstRect, {0, 0}, 0.0f, WHITE);
    EndShaderMode();
}

}  // namespace

PostFX::PostFX() = default;
PostFX::~PostFX() { cleanup(); }

void PostFX::cleanup() {
    if (!ready_) return;
    UnloadRenderTexture(scene_);
    UnloadRenderTexture(bright_);
    UnloadRenderTexture(blur_a_);
    UnloadRenderTexture(blur_b_);
    UnloadShader(bright_shader_);
    UnloadShader(blur_shader_);
    UnloadShader(composite_);
    ready_ = false;
}

void PostFX::load_shaders(const std::filesystem::path& dir) {
    bright_shader_ = LoadShader(nullptr, (dir / "bloom_bright.fs").string().c_str());
    blur_shader_   = LoadShader(nullptr, (dir / "blur.fs").string().c_str());
    composite_     = LoadShader(nullptr, (dir / "composite.fs").string().c_str());

    bright_threshold_loc_ = GetShaderLocation(bright_shader_, "threshold");
    bright_knee_loc_      = GetShaderLocation(bright_shader_, "soft_knee");
    blur_direction_loc_   = GetShaderLocation(blur_shader_,   "direction");
    comp_bloom_tex_loc_   = GetShaderLocation(composite_,     "bloomTex");
    comp_bloom_int_loc_   = GetShaderLocation(composite_,     "bloom_intensity");
    comp_vignette_loc_    = GetShaderLocation(composite_,     "vignette_strength");
    comp_low_hp_loc_      = GetShaderLocation(composite_,     "low_hp");

    const float threshold = 0.72f, knee = 0.35f;
    SetShaderValue(bright_shader_, bright_threshold_loc_, &threshold, SHADER_UNIFORM_FLOAT);
    SetShaderValue(bright_shader_, bright_knee_loc_,      &knee,      SHADER_UNIFORM_FLOAT);
}

bool PostFX::init(int width, int height, const std::filesystem::path& shader_dir) {
    cleanup();
    w_       = width;
    h_       = height;
    scene_   = LoadRenderTexture(width,     height);
    bright_  = LoadRenderTexture(width / 2, height / 2);
    blur_a_  = LoadRenderTexture(width / 2, height / 2);
    blur_b_  = LoadRenderTexture(width / 2, height / 2);
    load_shaders(shader_dir);

    if (bright_shader_.id == 0 || blur_shader_.id == 0 || composite_.id == 0) {
        VECTOR_WARN("post_fx: one or more shaders failed to compile; bloom disabled");
        cleanup();
        return false;
    }
    ready_ = true;
    VECTOR_INFO("post_fx: ready ({}x{})", width, height);
    return true;
}

void PostFX::resize(int width, int height) {
    if (!ready_ || (width == w_ && height == h_)) return;
    // Reuse current shader dir by repointing — re-init keeps shaders alive.
    UnloadRenderTexture(scene_);
    UnloadRenderTexture(bright_);
    UnloadRenderTexture(blur_a_);
    UnloadRenderTexture(blur_b_);
    w_      = width;
    h_      = height;
    scene_  = LoadRenderTexture(width,     height);
    bright_ = LoadRenderTexture(width / 2, height / 2);
    blur_a_ = LoadRenderTexture(width / 2, height / 2);
    blur_b_ = LoadRenderTexture(width / 2, height / 2);
}

void PostFX::begin_scene() {
    if (!ready_) return;
    BeginTextureMode(scene_);
    ClearBackground(BLACK);
}

void PostFX::end_scene() {
    if (!ready_) return;
    EndTextureMode();
}

void PostFX::present(float low_hp, bool bloom_enabled) {
    if (!ready_) return;

    if (bloom_enabled) {
        // 1) bright pass at half-res.
        BeginTextureMode(bright_);
        ClearBackground(BLACK);
        blit_rt(scene_, bright_.texture.width, bright_.texture.height, bright_shader_);
        EndTextureMode();

        // 2) horizontal blur into blur_a_.
        const float texelW = 1.0f / static_cast<float>(bright_.texture.width);
        const float texelH = 1.0f / static_cast<float>(bright_.texture.height);
        BeginTextureMode(blur_a_);
        ClearBackground(BLACK);
        {
            float dir[2] = {texelW, 0.0f};
            SetShaderValue(blur_shader_, blur_direction_loc_, dir, SHADER_UNIFORM_VEC2);
            blit_rt(bright_, blur_a_.texture.width, blur_a_.texture.height, blur_shader_);
        }
        EndTextureMode();

        // 3) vertical blur into blur_b_.
        BeginTextureMode(blur_b_);
        ClearBackground(BLACK);
        {
            float dir[2] = {0.0f, texelH};
            SetShaderValue(blur_shader_, blur_direction_loc_, dir, SHADER_UNIFORM_VEC2);
            blit_rt(blur_a_, blur_b_.texture.width, blur_b_.texture.height, blur_shader_);
        }
        EndTextureMode();
    }

    // 4) composite to back buffer.
    const float bloom_intensity   = bloom_enabled ? 1.05f : 0.0f;
    const float vignette_strength = 0.5f;
    SetShaderValue(composite_, comp_bloom_int_loc_,  &bloom_intensity,   SHADER_UNIFORM_FLOAT);
    SetShaderValue(composite_, comp_vignette_loc_,   &vignette_strength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(composite_, comp_low_hp_loc_,     &low_hp,            SHADER_UNIFORM_FLOAT);
    SetShaderValueTexture(composite_, comp_bloom_tex_loc_, blur_b_.texture);

    BeginShaderMode(composite_);
    Rectangle src{0, 0, static_cast<float>(scene_.texture.width),
                       -static_cast<float>(scene_.texture.height)};
    Rectangle dst{0, 0, static_cast<float>(GetScreenWidth()),
                        static_cast<float>(GetScreenHeight())};
    DrawTexturePro(scene_.texture, src, dst, {0, 0}, 0.0f, WHITE);
    EndShaderMode();
}

}  // namespace vector::render
