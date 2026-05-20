#pragma once

#include <raylib.h>

#include <filesystem>
#include <string>
#include <unordered_map>

namespace vector::render {

// JSON-loadable emitter blueprint. One spec is shared by many emit calls.
struct EmitterSpec {
    std::string name;

    // Emission shape — point or short line back-projected from the emit dir.
    int     burst_min      {6};
    int     burst_max      {12};
    float   rate_per_second{0.0f};   // 0 = burst-only (no continuous)

    // Per-particle randomization ranges.
    float   speed_min      {40.0f};
    float   speed_max      {120.0f};
    float   spread_rad     {0.35f};  // cone half-angle around emit direction
    float   lifetime_min   {0.30f};
    float   lifetime_max   {0.60f};
    float   size_start_min {2.0f};
    float   size_start_max {4.0f};
    float   size_end_min   {0.0f};
    float   size_end_max   {0.0f};
    float   drag           {2.0f};

    Color   color_start    {RAYWHITE};
    Color   color_end      {Color{255, 255, 255, 0}};
    bool    additive       {true};

    static EmitterSpec defaults() { return EmitterSpec{}; }
};

class EmitterLibrary {
public:
    bool load(const std::filesystem::path& json_path);

    const EmitterSpec* find(const std::string& name) const;
    std::size_t size() const { return specs_.size(); }

private:
    std::unordered_map<std::string, EmitterSpec> specs_;
};

}  // namespace vector::render
