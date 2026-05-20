add_executable(vector main.cpp)

# render→game→ecs→core, audio→core, platform→core, and all of those bring
# raylib transitively. Linking only the top-level libs avoids the
# "ignoring duplicate libraries" ld warning.
target_link_libraries(vector
    PRIVATE
        vector::render
        vector::audio
        vector::platform
)

if(VECTOR_ENABLE_IMGUI)
    target_link_libraries(vector PRIVATE vector::imgui)
    target_compile_definitions(vector PRIVATE VECTOR_ENABLE_IMGUI=1)
endif()

set_target_properties(vector PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
