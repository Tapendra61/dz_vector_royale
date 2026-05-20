add_library(vector_game STATIC
    components/components.cpp
    data/tuning.cpp
    input/keyboard_mouse_input.cpp
    input/gamepad_input.cpp
    ai/steering.cpp
    ai/behavior_tree.cpp
    ai/bot_brain.cpp
    systems/movement_system.cpp
    systems/physics_system.cpp
    systems/lifetime_system.cpp
    systems/spatial_hash.cpp
    systems/weapon_system.cpp
    systems/collision_system.cpp
    systems/health_system.cpp
    systems/pickup_system.cpp
    systems/ai_system.cpp
    world.cpp
)

target_include_directories(vector_game PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(vector_game
    PUBLIC
        vector::core
        vector::ecs
        raylib
)

add_library(vector::game ALIAS vector_game)
