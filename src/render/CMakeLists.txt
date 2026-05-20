add_library(vector_render STATIC
    camera.cpp
    sprite_renderer.cpp
    hud.cpp
)

target_include_directories(vector_render PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(vector_render
    PUBLIC
        vector::game
        raylib
)

add_library(vector::render ALIAS vector_render)
