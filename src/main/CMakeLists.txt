add_executable(vector main.cpp)

target_link_libraries(vector
    PRIVATE
        vector::core
        vector::platform
        raylib
)

if(VECTOR_ENABLE_IMGUI)
    target_link_libraries(vector PRIVATE vector::imgui)
    target_compile_definitions(vector PRIVATE VECTOR_ENABLE_IMGUI=1)
endif()

if(APPLE)
    target_link_libraries(vector PRIVATE
        "-framework IOKit"
        "-framework Cocoa"
        "-framework OpenGL"
    )
endif()

set_target_properties(vector PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
