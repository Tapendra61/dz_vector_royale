add_library(vector_audio STATIC
    audio_system.cpp
)

target_include_directories(vector_audio PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(vector_audio
    PUBLIC
        vector::core
        raylib
)

add_library(vector::audio ALIAS vector_audio)
