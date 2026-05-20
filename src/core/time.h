#pragma once

#include <chrono>
#include <cstdint>

namespace vector::core {

inline constexpr int    kSimTickRateHz = 60;
inline constexpr double kSimDt        = 1.0 / static_cast<double>(kSimTickRateHz);
inline constexpr double kMaxFrameTime = 0.25;  // clamp to avoid spiral of death

using Clock     = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Seconds   = std::chrono::duration<double>;

// Glenn Fiedler "Fix Your Timestep" accumulator. Drives a fixed-rate sim
// regardless of render rate. Caller invokes `step()` once per frame: the
// returned struct says how many sim ticks to run and the interpolation
// alpha (0..1) for rendering between the latest two sim states.
class FixedTimestep {
public:
    struct Step {
        int    sim_ticks;
        double alpha;
        double frame_dt;
    };

    FixedTimestep();

    Step step();

    std::uint64_t tick() const { return tick_; }
    double        accumulator() const { return accumulator_; }
    void          advance_tick() { ++tick_; }

private:
    TimePoint     last_;
    double        accumulator_ = 0.0;
    std::uint64_t tick_        = 0;
};

}  // namespace vector::core
