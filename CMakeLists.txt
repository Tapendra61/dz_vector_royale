cmake_minimum_required(VERSION 3.24)
project(vector
    VERSION 0.1.0
    DESCRIPTION "VECTOR — 2D multiplayer arena shooter"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build universal binary" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
endif()

if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

option(VECTOR_BUILD_TOOLS  "Build CLI tools (asset packer, etc.)" ON)
option(VECTOR_BUILD_TESTS  "Build unit tests"                     OFF)
option(VECTOR_ENABLE_IMGUI "Enable Dear ImGui debug overlay"      ON)

set(VECTOR_THIRD_PARTY_DIR "${CMAKE_BINARY_DIR}/_deps")

include(cmake/Dependencies.cmake)

add_subdirectory(src)

if(VECTOR_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(VECTOR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "")
message(STATUS "VECTOR configured:")
message(STATUS "  build type    : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard  : ${CMAKE_CXX_STANDARD}")
message(STATUS "  imgui overlay : ${VECTOR_ENABLE_IMGUI}")
message(STATUS "  build tools   : ${VECTOR_BUILD_TOOLS}")
message(STATUS "  build tests   : ${VECTOR_BUILD_TESTS}")
message(STATUS "")
