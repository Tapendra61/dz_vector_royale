# --------------------------------------------------------------------------
# Third-party dependencies via FetchContent.
# Pinned to specific tags / commits per roadmap §4.2.
# --------------------------------------------------------------------------
include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

# --- raylib --------------------------------------------------------------
set(BUILD_EXAMPLES   OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES      OFF CACHE BOOL "" FORCE)
set(SUPPORT_MODULE_RMODELS  ON CACHE BOOL "" FORCE)
set(SUPPORT_MODULE_RAUDIO   ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)

# --- entt ----------------------------------------------------------------
FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        v3.14.0
    GIT_SHALLOW    TRUE
)

# --- spdlog --------------------------------------------------------------
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL       OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.15.0
    GIT_SHALLOW    TRUE
)

# --- nlohmann_json (header-only JSON for config + asset packer) ----------
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install    OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(raylib entt spdlog nlohmann_json)

# --- doctest (only when tests are requested) ---
if(VECTOR_BUILD_TESTS)
    set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
    set(DOCTEST_NO_INSTALL ON  CACHE BOOL "" FORCE)
    # doctest v2.4.11's CMake still declares cmake_minimum_required <3.5,
    # which CMake 4.x rejects without an explicit policy floor.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
    FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG        v2.4.11
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(doctest)
endif()

# --- Dear ImGui + rlImGui (upstream ships no CMakeLists, vendor target) --
if(VECTOR_ENABLE_IMGUI)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.92.0
        GIT_SHALLOW    TRUE
    )
    FetchContent_Declare(
        rlimgui
        GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
        GIT_TAG        main
    )
    FetchContent_MakeAvailable(imgui rlimgui)

    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    )
    target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    set_target_properties(imgui PROPERTIES CXX_STANDARD 17)

    add_library(rlimgui STATIC
        ${rlimgui_SOURCE_DIR}/rlImGui.cpp
    )
    target_include_directories(rlimgui PUBLIC ${rlimgui_SOURCE_DIR})
    target_link_libraries(rlimgui PUBLIC imgui raylib)
    set_target_properties(rlimgui PROPERTIES CXX_STANDARD 17)

    add_library(vector::imgui ALIAS rlimgui)
endif()
