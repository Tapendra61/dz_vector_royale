#include "core/config.h"

#include <doctest/doctest.h>

using namespace vector::core;

TEST_CASE("Config dotted-key lookup with fallback") {
    auto cfg = Config::from_string(R"({
        "window": { "title": "X", "width": 800 },
        "audio":  { "master_volume": 0.6 }
    })");
    CHECK(cfg.get<std::string>("window.title", "default") == "X");
    CHECK(cfg.get<int>("window.width", 0)          == 800);
    CHECK(cfg.get<double>("audio.master_volume", 0.0) == doctest::Approx(0.6));
    CHECK(cfg.get<std::string>("missing.key", "fallback") == "fallback");
    CHECK(cfg.get<int>("window.title", 99) == 99);  // type mismatch falls back
}
