#pragma once

#include "ecs/registry.h"

#include <raylib.h>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace vector::game {

// Uniform-grid broadphase. Cell size ≈ 2× max entity radius keeps a
// query's candidate set tiny. Rebuilt each tick.
class SpatialHash {
public:
    explicit SpatialHash(float cell_size = 64.0f) : cell_(cell_size) {}

    void clear();
    void insert(ecs::Entity e, Vector2 pos, float radius);
    void query(Vector2 pos, float radius, std::vector<ecs::Entity>& out) const;

    float cell_size() const { return cell_; }

private:
    struct Key {
        std::int32_t x, y;
        bool operator==(const Key& o) const { return x == o.x && y == o.y; }
    };
    struct KeyHash {
        std::size_t operator()(const Key& k) const noexcept {
            return (static_cast<std::size_t>(k.x) * 73856093u)
                 ^ (static_cast<std::size_t>(k.y) * 19349663u);
        }
    };

    float cell_;
    std::unordered_map<Key, std::vector<ecs::Entity>, KeyHash> cells_;
};

}  // namespace vector::game
