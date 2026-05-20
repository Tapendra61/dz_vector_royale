#include "game/systems/pickup_system.h"

#include "game/components/emitter_state.h"
#include "game/components/health.h"
#include "game/components/intent.h"
#include "game/components/inventory.h"
#include "game/components/physics.h"
#include "game/components/pickup.h"
#include "game/components/render_tag.h"
#include "game/components/transform.h"
#include "game/data/tuning.h"

#include <raymath.h>

#include <algorithm>
#include <random>

namespace vector::game {

namespace {

std::mt19937& prng() {
    thread_local std::mt19937 g{std::random_device{}()};
    return g;
}

float jitter(float base, float spread) {
    if (spread <= 0.0f) return base;
    std::uniform_real_distribution<float> d(-spread, spread);
    return std::max(0.0f, base + d(prng()));
}

}  // namespace

void PickupSystem::tick(ecs::Registry& reg, const PickupTuning& tune, float dt) {
    events_.clear();

    // --- 1) Spawn director. ---
    reg.view<PickupSpawnPoint, TransformComponent>().each(
        [&](PickupSpawnPoint& sp, TransformComponent& sp_tr) {
            if (sp.next_spawn_in > 0.0f) {
                sp.next_spawn_in -= dt;
                return;
            }
            auto pickup = reg.create();
            reg.emplace<TransformComponent>(pickup, TransformComponent{sp_tr.position, 0.0f});
            reg.emplace<PrevTransformComponent>(pickup);
            reg.emplace<ColliderComponent>(pickup, ColliderComponent{tune.radius});
            reg.emplace<PickupComponent>(pickup, PickupComponent{
                sp.preferred_type, tune.despawn_timeout, 0.0f
            });
            reg.emplace<RenderComponent>(pickup, RenderComponent{
                Shape::Pickup, Color{120, 220, 140, 255}, tune.radius
            });
            reg.emplace<EmitterStateComponent>(pickup);
            sp.next_spawn_in = jitter(sp.cooldown, sp.jitter);
            events_.push_back({Event::Spawned, ecs::null_entity});
        });

    // --- 2) Despawn timeouts + bob animation. ---
    {
        std::vector<ecs::Entity> doomed;
        reg.view<PickupComponent>().each([&](ecs::Entity e, PickupComponent& p) {
            p.bob_phase  += dt * 4.0f;
            p.despawn_in -= dt;
            if (p.despawn_in <= 0.0f) doomed.push_back(e);
        });
        for (auto e : doomed) {
            reg.destroy(e);
            events_.push_back({Event::Despawned, ecs::null_entity});
        }
    }

    // --- 3) Acquisition: every ship with an inventory checks pickup overlap. ---
    auto ship_view = reg.view<TransformComponent, ColliderComponent, InventoryComponent>();
    std::vector<ecs::Entity> acquired;
    for (auto ship : ship_view) {
        auto& inv     = ship_view.get<InventoryComponent>(ship);
        auto& ship_tr = ship_view.get<TransformComponent>(ship);
        auto& ship_co = ship_view.get<ColliderComponent>(ship);

        reg.view<PickupComponent, TransformComponent, ColliderComponent>().each(
            [&](ecs::Entity e, const PickupComponent& p, const TransformComponent& tr, const ColliderComponent& col) {
                if (std::find(acquired.begin(), acquired.end(), e) != acquired.end()) return;
                const float dx = tr.position.x - ship_tr.position.x;
                const float dy = tr.position.y - ship_tr.position.y;
                const float r  = col.radius + ship_co.radius;
                if (dx * dx + dy * dy > r * r) return;

                const auto slot = slot_for(p.type);
                if (slot == PowerUpSlot::Primary && inv.primary == PowerUpType::None) {
                    inv.primary        = p.type;
                    inv.primary_charge = 1.0f;
                    acquired.push_back(e);
                    events_.push_back({Event::Acquired, ship});
                } else if (slot == PowerUpSlot::Special && inv.special == PowerUpType::None) {
                    inv.special        = p.type;
                    inv.special_charge = 1.0f;
                    acquired.push_back(e);
                    events_.push_back({Event::Acquired, ship});
                }
            });
    }
    for (auto e : acquired) if (reg.valid(e)) reg.destroy(e);

    // --- 4) Activation. Per-ship; Phase 1/2 supports RepairPack only. ---
    auto act_view = reg.view<InventoryComponent, IntentComponent, HealthComponent>();
    for (auto ship : act_view) {
        auto& inv    = act_view.get<InventoryComponent>(ship);
        auto& intent = act_view.get<IntentComponent>(ship).value;
        auto& hp     = act_view.get<HealthComponent>(ship);

        if (intent.use_special && inv.special != PowerUpType::None) {
            if (inv.special == PowerUpType::RepairPack) {
                hp.current = std::min(hp.max, hp.current + tune.repair_amount);
                inv.special        = PowerUpType::None;
                inv.special_charge = 0.0f;
                events_.push_back({Event::Activated, ship});
            } else {
                events_.push_back({Event::ActivationDenied, ship});
            }
        }
        if (intent.use_primary && inv.primary != PowerUpType::None) {
            events_.push_back({Event::ActivationDenied, ship});
        }
    }
}

}  // namespace vector::game
