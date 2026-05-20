#include "game/systems/lifetime_system.h"

#include "game/components/bullet.h"

#include <vector>

namespace vector::game {

void LifetimeSystem::tick(ecs::Registry& reg, float dt) {
    std::vector<ecs::Entity> doomed;
    reg.view<LifetimeComponent>().each(
        [&](ecs::Entity e, LifetimeComponent& lt) {
            lt.seconds_remaining -= dt;
            if (lt.seconds_remaining <= 0.0f) doomed.push_back(e);
        });
    for (auto e : doomed) reg.destroy(e);
}

}  // namespace vector::game
