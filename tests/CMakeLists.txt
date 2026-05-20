add_executable(vector_tests
    main.cpp
    test_fixed_timestep.cpp
    test_spatial_hash.cpp
    test_event_bus.cpp
    test_config.cpp
    test_behavior_tree.cpp
)

target_link_libraries(vector_tests
    PRIVATE
        vector::core
        vector::game
        doctest::doctest
)

target_include_directories(vector_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

set_target_properties(vector_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME vector_tests COMMAND vector_tests)
