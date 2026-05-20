#include "game/systems/health_system.h"

#include "game/components/health.h"
#include "game/components/status_effects.h"

#include <algorithm>

namespace vector::game {

void HealthSystem::tick(ecs::Registry& reg, float dt) {
    deaths_.clear();

    // 1) Apply queued damage.
    auto dmg_view = reg.view<HealthComponent, DamageEvent>();
    for (auto e : dmg_view) {
        auto& hp  = reg.get<HealthComponent>(e);
        auto& dmg = reg.get<DamageEvent>(e);
        hp.current        = std::max(0.0f, hp.current - dmg.amount);
        hp.time_since_dmg = 0.0f;
        if (hp.is_dead()) {
            deaths_.push_back({e, dmg.source});
        }
        reg.remove<DamageEvent>(e);
    }

    // 2) Tick status-effect timers (decay).
    reg.view<StatusEffectComponent>().each([&](StatusEffectComponent& sx) {
        for (auto& slot : sx.effects) {
            if (slot.time_remaining > 0.0f) {
                slot.time_remaining -= dt;
                if (slot.time_remaining <= 0.0f) {
                    slot.type = StatusEffectType::None;
                    slot.time_remaining = 0.0f;
                }
            }
        }
    });

    // 3) Out-of-combat regen.
    reg.view<HealthComponent>().each([&](HealthComponent& hp) {
        hp.time_since_dmg += dt;
        if (hp.is_dead()) return;
        if (hp.time_since_dmg >= hp.regen_delay && hp.current < hp.max) {
            hp.current = std::min(hp.max, hp.current + hp.regen_rate * dt);
        }
    });
}

}  // namespace vector::game
