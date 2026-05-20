#include "game/systems/pickup_system.h"

#include "game/components/health.h"
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

void PickupSystem::tick(ecs::Registry& reg,
                       ecs::Entity player_ship,
                       const InputIntent& intent,
                       const PickupTuning& tune,
                       float dt) {
    events_.clear();

    // --- 1) Spawn director: each spawn point counts down independently. ---
    // We *create* pickup entities only when the spawn-point's timer hits 0
    // and there isn't already a live pickup at that point.
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

            sp.next_spawn_in = jitter(sp.cooldown, sp.jitter);
            events_.push_back({Event::Spawned, ecs::null_entity});
        });

    // --- 2) Despawn expired pickups + animate the bob phase. ---
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

    // --- 3) Acquisition: ship overlaps a pickup → into inventory. ---
    if (reg.valid(player_ship)) {
        auto& ship_tr = reg.get<TransformComponent>(player_ship);
        auto& ship_co = reg.get<ColliderComponent>(player_ship);
        auto& inv     = reg.get<InventoryComponent>(player_ship);

        std::vector<ecs::Entity> acquired;
        reg.view<PickupComponent, TransformComponent, ColliderComponent>().each(
            [&](ecs::Entity e, const PickupComponent& p, const TransformComponent& tr, const ColliderComponent& col) {
                const float dx = tr.position.x - ship_tr.position.x;
                const float dy = tr.position.y - ship_tr.position.y;
                const float r  = col.radius + ship_co.radius;
                if (dx * dx + dy * dy > r * r) return;

                const auto slot = slot_for(p.type);
                if (slot == PowerUpSlot::Primary && inv.primary == PowerUpType::None) {
                    inv.primary        = p.type;
                    inv.primary_charge = 1.0f;
                    acquired.push_back(e);
                } else if (slot == PowerUpSlot::Special && inv.special == PowerUpType::None) {
                    inv.special        = p.type;
                    inv.special_charge = 1.0f;
                    acquired.push_back(e);
                }
            });
        for (auto e : acquired) {
            reg.destroy(e);
            events_.push_back({Event::Acquired, player_ship});
        }

        // --- 4) Activation. Phase 1 supports RepairPack only. ---
        auto activate_special = [&]() {
            switch (inv.special) {
                case PowerUpType::RepairPack: {
                    auto& hp = reg.get<HealthComponent>(player_ship);
                    hp.current = std::min(hp.max, hp.current + tune.repair_amount);
                    inv.special        = PowerUpType::None;
                    inv.special_charge = 0.0f;
                    events_.push_back({Event::Activated, player_ship});
                    return true;
                }
                default:
                    events_.push_back({Event::ActivationDenied, player_ship});
                    return false;
            }
        };
        auto activate_primary = [&]() {
            // No Phase 1 primaries shipped yet; reject so the SFX cue can play.
            events_.push_back({Event::ActivationDenied, player_ship});
            return false;
        };

        if (intent.use_special && inv.special != PowerUpType::None) activate_special();
        if (intent.use_primary && inv.primary != PowerUpType::None) activate_primary();
    }
}

}  // namespace vector::game
