add_library(vector_ecs INTERFACE)
target_include_directories(vector_ecs INTERFACE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(vector_ecs INTERFACE EnTT::EnTT)
add_library(vector::ecs ALIAS vector_ecs)
