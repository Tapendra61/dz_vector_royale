add_library(vector_platform STATIC
    desktop_platform.cpp
)

target_include_directories(vector_platform
    PUBLIC ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(vector_platform
    PUBLIC
        vector::core
        raylib
)

add_library(vector::platform ALIAS vector_platform)
