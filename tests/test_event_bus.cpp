#include "core/event_bus.h"

#include <doctest/doctest.h>

using namespace vector::core;

namespace {
struct Pinged { int value; };
}

TEST_CASE("EventBus delivers published events to subscribers") {
    EventBus bus;
    int  hits = 0;
    auto sub = bus.subscribe<Pinged>([&](const Pinged& p) { hits += p.value; });
    CHECK(sub.valid());

    bus.publish(Pinged{1});
    bus.publish(Pinged{2});
    CHECK(hits == 3);
}

TEST_CASE("EventBus skips dropped subscriptions") {
    EventBus bus;
    int hits = 0;
    {
        auto sub = bus.subscribe<Pinged>([&](const Pinged&) { hits = 1; });
        bus.publish(Pinged{0});
        CHECK(hits == 1);
    }
    bus.publish(Pinged{0});
    CHECK(hits == 1);  // subscription went out of scope; second publish must not fire
}
