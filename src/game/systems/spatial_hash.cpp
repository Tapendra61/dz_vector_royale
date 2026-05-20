#include "game/systems/spatial_hash.h"

#include <cmath>

namespace vector::game {

void SpatialHash::clear() {
    for (auto& [k, v] : cells_) v.clear();
}

void SpatialHash::insert(ecs::Entity e, Vector2 pos, float radius) {
    const int min_x = static_cast<int>(std::floor((pos.x - radius) / cell_));
    const int max_x = static_cast<int>(std::floor((pos.x + radius) / cell_));
    const int min_y = static_cast<int>(std::floor((pos.y - radius) / cell_));
    const int max_y = static_cast<int>(std::floor((pos.y + radius) / cell_));
    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            cells_[Key{x, y}].push_back(e);
        }
    }
}

void SpatialHash::query(Vector2 pos, float radius, std::vector<ecs::Entity>& out) const {
    const int min_x = static_cast<int>(std::floor((pos.x - radius) / cell_));
    const int max_x = static_cast<int>(std::floor((pos.x + radius) / cell_));
    const int min_y = static_cast<int>(std::floor((pos.y - radius) / cell_));
    const int max_y = static_cast<int>(std::floor((pos.y + radius) / cell_));
    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            auto it = cells_.find(Key{x, y});
            if (it == cells_.end()) continue;
            for (auto e : it->second) out.push_back(e);
        }
    }
}

}  // namespace vector::game
