#pragma once

#include <cstddef>

namespace vector::render {

enum class GraphicsPreset { Low, Medium, High };

struct GraphicsSettings {
    GraphicsPreset preset       {GraphicsPreset::High};
    std::size_t    particle_cap {3000};
    bool           bloom        {true};

    static GraphicsSettings for_preset(GraphicsPreset p) {
        GraphicsSettings s;
        s.preset = p;
        switch (p) {
            case GraphicsPreset::Low:
                s.particle_cap = 600;
                s.bloom        = false;
                break;
            case GraphicsPreset::Medium:
                s.particle_cap = 1800;
                s.bloom        = true;
                break;
            case GraphicsPreset::High:
                s.particle_cap = 4000;
                s.bloom        = true;
                break;
        }
        return s;
    }
};

}  // namespace vector::render
