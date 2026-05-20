#pragma once

#include <raylib.h>

#include <string>
#include <vector>

namespace vector::render {

// Lightweight one-frame fx queue. Game systems push events; the renderer
// drains and turns each into a particle burst (+ screen shake). Keeps
// game code free of any rendering dependency.
struct FxEvent {
    std::string emitter;   // matches an EmitterSpec name in particles.json
    Vector2     position{0, 0};
    Vector2     direction{1, 0};
    float       shake_trauma{0.0f};  // 0..1
};

class FxBridge {
public:
    void push(FxEvent e) { events_.push_back(std::move(e)); }
    const std::vector<FxEvent>& drain() const { return events_; }
    void clear() { events_.clear(); }

private:
    std::vector<FxEvent> events_;
};

}  // namespace vector::render
