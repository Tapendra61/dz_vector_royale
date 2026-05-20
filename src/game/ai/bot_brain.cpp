#include "game/ai/bot_brain.h"

#include "game/ai/blackboard.h"
#include "game/components/health.h"
#include "game/components/inventory.h"
#include "game/components/pickup.h"
#include "game/components/transform.h"

#include <raymath.h>

#include <cmath>

namespace vector::game::ai {

namespace {

// ---- Predicates ------------------------------------------------------

bool has_target(const Blackboard& bb) { return bb.best_target != ecs::null_entity; }

bool low_hp(const Blackboard& bb) {
    if (!bb.self_hp || !bb.ctrl) return false;
    return bb.self_hp->fraction() <= bb.ctrl->low_hp_threshold;
}

bool target_in_fire_range(const Blackboard& bb) {
    return bb.ctrl && bb.best_target_distance <= bb.ctrl->fire_range;
}

bool has_repair_special(const Blackboard& bb) {
    return bb.self_inv && bb.self_inv->special == PowerUpType::RepairPack;
}

bool pickup_visible(const Blackboard& bb) {
    return bb.nearest_pickup != ecs::null_entity;
}

// ---- Actions: write into bb.ctrl, never directly touch other state. ----

template <typename ApplyFn>
NodePtr act(const char* tag, ApplyFn fn) {
    return std::make_unique<Action>(tag, [fn = std::move(fn)](Blackboard& bb) {
        if (!bb.ctrl) return Status::Failure;
        fn(bb);
        return Status::Success;
    });
}

NodePtr cond(const char* tag, Condition::Pred p) {
    return std::make_unique<Condition>(tag, std::move(p));
}

NodePtr selector(std::vector<NodePtr> children) {
    return std::make_unique<Selector>(std::move(children));
}

NodePtr sequence(std::vector<NodePtr> children) {
    return std::make_unique<Sequence>(std::move(children));
}

// Action: zero the desired velocity (used as a fallback "stand").
NodePtr act_idle() {
    return act("Idle", [](Blackboard& bb) {
        bb.ctrl->desired_velocity = {0.0f, 0.0f};
        bb.ctrl->want_fire = false;
    });
}

// Aim at target + open fire if inside range. The BT writes `want_fire`;
// the steering pass converts it to an InputIntent the WeaponSystem reads.
NodePtr act_engage() {
    return act("Engage", [](Blackboard& bb) {
        bb.ctrl->target     = bb.best_target;
        bb.ctrl->target_pos = bb.best_target_pos;
        bb.ctrl->target_vel = bb.best_target_vel;

        // Maintain preferred range: pursue if too far, back off if too close.
        const float dist = bb.best_target_distance;
        if (dist > bb.ctrl->preferred_range) {
            bb.ctrl->desired_velocity = Vector2Subtract(bb.best_target_pos, bb.self_tr->position);
        } else {
            bb.ctrl->desired_velocity = Vector2Subtract(bb.self_tr->position, bb.best_target_pos);
        }
        bb.ctrl->want_fire = bb.best_target_distance <= bb.ctrl->fire_range;
    });
}

// Sniper variant — fire from afar, retreat if anything closes the gap.
NodePtr act_snipe() {
    return act("Snipe", [](Blackboard& bb) {
        bb.ctrl->target     = bb.best_target;
        bb.ctrl->target_pos = bb.best_target_pos;
        bb.ctrl->target_vel = bb.best_target_vel;
        const float dist = bb.best_target_distance;
        if (dist < bb.ctrl->preferred_range * 0.85f) {
            // Back-pedal.
            bb.ctrl->desired_velocity = Vector2Subtract(bb.self_tr->position, bb.best_target_pos);
        } else {
            // Hold position laterally (zero throttle).
            bb.ctrl->desired_velocity = {0.0f, 0.0f};
        }
        bb.ctrl->want_fire = bb.best_target_distance <= bb.ctrl->fire_range;
    });
}

// Brawler — get in close fast.
NodePtr act_brawl() {
    return act("Brawl", [](Blackboard& bb) {
        bb.ctrl->target     = bb.best_target;
        bb.ctrl->target_pos = bb.best_target_pos;
        bb.ctrl->target_vel = bb.best_target_vel;
        bb.ctrl->desired_velocity = Vector2Subtract(bb.best_target_pos, bb.self_tr->position);
        bb.ctrl->want_fire = bb.best_target_distance <= bb.ctrl->fire_range;
    });
}

NodePtr act_flee() {
    return act("FleeFromTarget", [](Blackboard& bb) {
        if (bb.best_target == ecs::null_entity) {
            bb.ctrl->desired_velocity = {0.0f, 0.0f};
            return;
        }
        bb.ctrl->desired_velocity = Vector2Subtract(bb.self_tr->position, bb.best_target_pos);
        bb.ctrl->want_fire = false;
    });
}

NodePtr act_seek_pickup() {
    return act("SeekPickup", [](Blackboard& bb) {
        bb.ctrl->desired_velocity = Vector2Subtract(bb.nearest_pickup_pos, bb.self_tr->position);
        bb.ctrl->want_fire = false;
    });
}

NodePtr act_activate_repair() {
    return act("ActivateRepair", [](Blackboard& bb) {
        bb.ctrl->want_use_special = true;
    });
}

NodePtr act_wander() {
    return act("Wander", [](Blackboard& bb) {
        // Steering pass will fill `desired_velocity` from the wander phase
        // because this leaf leaves it untouched. Signal "no specific goal"
        // by clearing target.
        bb.ctrl->target = ecs::null_entity;
        bb.ctrl->want_fire = false;
        bb.ctrl->desired_velocity = {0.0f, 0.0f};  // steering layer overrides
    });
}

}  // namespace

NodePtr build_sniper_tree() {
    // Priority: heal if low → flee to range if pressured → snipe → wander.
    std::vector<NodePtr> root;
    {
        std::vector<NodePtr> heal;
        heal.push_back(cond("HasRepair",  &has_repair_special));
        heal.push_back(cond("LowHP",      &low_hp));
        heal.push_back(act_activate_repair());
        root.push_back(sequence(std::move(heal)));
    }
    {
        std::vector<NodePtr> snipe;
        snipe.push_back(cond("HasTarget", &has_target));
        snipe.push_back(act_snipe());
        root.push_back(sequence(std::move(snipe)));
    }
    {
        std::vector<NodePtr> pickup;
        pickup.push_back(cond("PickupVisible", &pickup_visible));
        pickup.push_back(act_seek_pickup());
        root.push_back(sequence(std::move(pickup)));
    }
    root.push_back(act_wander());
    return selector(std::move(root));
}

NodePtr build_brawler_tree() {
    // Priority: heal if low → engage close → grab pickup → wander.
    std::vector<NodePtr> root;
    {
        std::vector<NodePtr> heal;
        heal.push_back(cond("HasRepair", &has_repair_special));
        heal.push_back(cond("LowHP",     &low_hp));
        heal.push_back(act_activate_repair());
        root.push_back(sequence(std::move(heal)));
    }
    {
        std::vector<NodePtr> brawl;
        brawl.push_back(cond("HasTarget", &has_target));
        brawl.push_back(act_brawl());
        root.push_back(sequence(std::move(brawl)));
    }
    {
        std::vector<NodePtr> pickup;
        pickup.push_back(cond("PickupVisible", &pickup_visible));
        pickup.push_back(act_seek_pickup());
        root.push_back(sequence(std::move(pickup)));
    }
    root.push_back(act_wander());
    return selector(std::move(root));
}

NodePtr build_runner_tree() {
    // Priority: flee when seen → grab pickup → engage opportunistically → wander.
    std::vector<NodePtr> root;
    {
        std::vector<NodePtr> heal;
        heal.push_back(cond("HasRepair", &has_repair_special));
        heal.push_back(cond("LowHP",     &low_hp));
        heal.push_back(act_activate_repair());
        root.push_back(sequence(std::move(heal)));
    }
    {
        std::vector<NodePtr> flee;
        flee.push_back(cond("LowHP",      &low_hp));
        flee.push_back(cond("HasTarget",  &has_target));
        flee.push_back(act_flee());
        root.push_back(sequence(std::move(flee)));
    }
    {
        std::vector<NodePtr> pickup;
        pickup.push_back(cond("PickupVisible", &pickup_visible));
        pickup.push_back(act_seek_pickup());
        root.push_back(sequence(std::move(pickup)));
    }
    {
        std::vector<NodePtr> engage;
        engage.push_back(cond("HasTarget", &has_target));
        engage.push_back(cond("TargetInFireRange", &target_in_fire_range));
        engage.push_back(act_engage());
        root.push_back(sequence(std::move(engage)));
    }
    root.push_back(act_wander());
    return selector(std::move(root));
}

ArchetypePreset preset_for(BotArchetype a) {
    AIControllerComponent c{};
    switch (a) {
        case BotArchetype::Sniper:
            c.archetype          = BotArchetype::Sniper;
            c.reaction_delay_s   = 0.35f;
            c.aim_noise_rad      = 0.04f;
            c.aggression         = 0.4f;
            c.preferred_range    = 480.0f;
            c.fire_range         = 700.0f;
            return {c, &build_sniper_tree};
        case BotArchetype::Brawler:
            c.archetype          = BotArchetype::Brawler;
            c.reaction_delay_s   = 0.18f;
            c.aim_noise_rad      = 0.12f;
            c.aggression         = 0.9f;
            c.preferred_range    = 160.0f;
            c.fire_range         = 320.0f;
            return {c, &build_brawler_tree};
        case BotArchetype::Runner:
            c.archetype          = BotArchetype::Runner;
            c.reaction_delay_s   = 0.10f;
            c.aim_noise_rad      = 0.18f;
            c.aggression         = 0.3f;
            c.preferred_range    = 240.0f;
            c.fire_range         = 420.0f;
            c.low_hp_threshold   = 0.55f;  // bails earlier
            return {c, &build_runner_tree};
    }
    return {c, &build_brawler_tree};
}

}  // namespace vector::game::ai
