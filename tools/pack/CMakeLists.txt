add_executable(vector_pack main.cpp)

target_link_libraries(vector_pack
    PRIVATE
        vector::core
)

set_target_properties(vector_pack PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
