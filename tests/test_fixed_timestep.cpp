#include "core/time.h"

#include <doctest/doctest.h>

#include <thread>

using namespace vector::core;

TEST_CASE("FixedTimestep produces at least one tick after enough wall time") {
    FixedTimestep ts;
    // First step has near-zero elapsed; let some real time pass.
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    const auto step = ts.step();
    CHECK(step.sim_ticks >= 1);
    CHECK(step.alpha >= 0.0);
    CHECK(step.alpha <  1.0);
    CHECK(ts.tick() == static_cast<std::uint64_t>(step.sim_ticks));
}

TEST_CASE("FixedTimestep clamps catastrophic frames to kMaxFrameTime") {
    FixedTimestep ts;
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    const auto step = ts.step();
    // 0.8s wall clamped to 0.25s ⇒ at most 15 sim ticks at 60 Hz.
    CHECK(step.sim_ticks <= 16);
}
