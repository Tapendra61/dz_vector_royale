#include "core/time.h"

#include <algorithm>

namespace vector::core {

FixedTimestep::FixedTimestep() : last_(Clock::now()) {}

FixedTimestep::Step FixedTimestep::step() {
    const auto   now      = Clock::now();
    const double frame_dt = std::min(Seconds(now - last_).count(), kMaxFrameTime);
    last_                 = now;

    accumulator_ += frame_dt;

    int sim_ticks = 0;
    while (accumulator_ >= kSimDt) {
        accumulator_ -= kSimDt;
        ++sim_ticks;
        ++tick_;
    }

    const double alpha = accumulator_ / kSimDt;
    return Step{sim_ticks, alpha, frame_dt};
}

}  // namespace vector::core
