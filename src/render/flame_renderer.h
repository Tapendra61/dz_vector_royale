#pragma once

#include "ecs/registry.h"

#include <raylib.h>

#include <filesystem>

namespace vector::render {

// Procedural flame plume rendered behind every thrusting ship via a
// fragment shader (assets/shaders/flame.fs). Color sweeps from hot
// white-blue at the engine to red at the tail; the width tapers and the
// silhouette flickers via animated noise. Bloom picks the brightest
// pixels up so the engine pops.
class FlameRenderer {
public:
    FlameRenderer();
    ~FlameRenderer();

    FlameRenderer(const FlameRenderer&)            = delete;
    FlameRenderer& operator=(const FlameRenderer&) = delete;

    bool init(const std::filesystem::path& shader_dir);
    void draw(const ecs::Registry& reg, float ship_radius, float time_s);

    bool ready() const { return ready_; }

private:
    Shader    shader_           {};
    int       u_time_loc_       {-1};
    int       u_intensity_loc_  {-1};
    Texture2D blank_            {};
    bool      ready_            {false};
};

}  // namespace vector::render
