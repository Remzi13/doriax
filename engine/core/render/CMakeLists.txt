cmake_minimum_required(VERSION 3.15)

add_library(doriax_render STATIC
    BufferRender.h
    BufferRender.cpp
    CameraRender.h
    CameraRender.cpp
    FramebufferRender.h
    FramebufferRender.cpp
    ObjectRender.h
    ObjectRender.cpp
    ShaderRender.h
    ShaderRender.cpp
    SystemRender.h
    SystemRender.cpp
    TextureRender.h
    TextureRender.cpp
)

target_include_directories(doriax_render
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)

target_link_libraries(doriax_render
    PUBLIC
        doriax_math
        sokol
)

target_compile_features(doriax_render PUBLIC cxx_std_17)

set_target_properties(doriax_render PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    FOLDER "doriax"
)
