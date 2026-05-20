#include "ecs/registry.h"
#include "game/systems/spatial_hash.h"

#include <doctest/doctest.h>

#include <algorithm>

using namespace vector;

TEST_CASE("SpatialHash returns nearby entity within query radius") {
    ecs::Registry reg;
    auto a = reg.create();
    auto b = reg.create();
    auto c = reg.create();

    game::SpatialHash hash(64.0f);
    hash.insert(a, {  10.0f,  10.0f}, 4.0f);
    hash.insert(b, {  20.0f,  10.0f}, 4.0f);
    hash.insert(c, {2000.0f, 2000.0f}, 4.0f);

    std::vector<ecs::Entity> hits;
    hash.query({15.0f, 10.0f}, 16.0f, hits);
    // De-dup: an entity can sit in two cells if its radius spans a boundary.
    std::sort(hits.begin(), hits.end());
    hits.erase(std::unique(hits.begin(), hits.end()), hits.end());

    CHECK(std::find(hits.begin(), hits.end(), a) != hits.end());
    CHECK(std::find(hits.begin(), hits.end(), b) != hits.end());
    CHECK(std::find(hits.begin(), hits.end(), c) == hits.end());
}

TEST_CASE("SpatialHash::clear empties cells") {
    game::SpatialHash hash(32.0f);
    ecs::Registry reg;
    auto e = reg.create();
    hash.insert(e, {0.0f, 0.0f}, 4.0f);
    hash.clear();
    std::vector<ecs::Entity> out;
    hash.query({0.0f, 0.0f}, 100.0f, out);
    CHECK(out.empty());
}
