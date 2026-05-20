add_library(vector_core STATIC
    logging.cpp
    config.cpp
    event_bus.cpp
    time.cpp
)

target_include_directories(vector_core
    PUBLIC ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(vector_core
    PUBLIC
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        EnTT::EnTT
)

add_library(vector::core ALIAS vector_core)
