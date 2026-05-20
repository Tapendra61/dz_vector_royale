#pragma once

#include "game/ai/behavior_tree.h"
#include "game/components/ai.h"

#include <array>

namespace vector::game::ai {

// Configuration for an archetype: BT tuning *and* a tree builder. The
// AISystem holds one shared tree per archetype (trees are stateless).
struct ArchetypePreset {
    AIControllerComponent ctrl_defaults;
    NodePtr (*build_tree)();
};

ArchetypePreset preset_for(BotArchetype a);

// Tree builders.
NodePtr build_sniper_tree();
NodePtr build_brawler_tree();
NodePtr build_runner_tree();

}  // namespace vector::game::ai
