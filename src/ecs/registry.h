#pragma once

#include <entt/entt.hpp>

namespace vector::ecs {

// Project-wide alias so we can swap entt out later without a global rename.
// All gameplay code spells `vector::ecs::Registry` / `Entity`.
using Registry = entt::registry;
using Entity   = entt::entity;

constexpr Entity null_entity = entt::null;

}  // namespace vector::ecs
