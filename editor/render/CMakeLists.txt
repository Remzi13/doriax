project(editor-render)

set(EDITOR_RENDER_SOURCES
    ToolsLayer.h
    ToolsLayer.cpp
    UILayer.h
    UILayer.cpp
    SceneRender.h
    SceneRender.cpp
    SceneRender2D.h
    SceneRender2D.cpp
    SceneRender3D.h
    SceneRender3D.cpp
    gizmo/TranslateGizmo.h
    gizmo/TranslateGizmo.cpp
    gizmo/RotateGizmo.h
    gizmo/RotateGizmo.cpp
    gizmo/ScaleGizmo.h
    gizmo/ScaleGizmo.cpp    
    gizmo/Object2DGizmo.h
    gizmo/Object2DGizmo.cpp
    gizmo/AnchorGizmo.h
    gizmo/AnchorGizmo.cpp
    gizmo/ViewportGizmo.h
    gizmo/ViewportGizmo.cpp
    preview/MaterialRender.h
    preview/MaterialRender.cpp
    preview/MeshPreviewRender.h
    preview/MeshPreviewRender.cpp
    preview/DirectionRender.h
    preview/DirectionRender.cpp
    preview/ModelRender.h
    preview/ModelRender.cpp
)

# Define paths relative to CMAKE_SOURCE_DIR since this is a subdirectory
set(DORIAX_ROOT ${CMAKE_SOURCE_DIR}/engine)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/libs/imgui)
set(SHADERCOMPILER_ROOT ${CMAKE_SOURCE_DIR}/shadercompiler)

add_library(editor-render STATIC ${EDITOR_RENDER_SOURCES})

# Set up include directories
target_include_directories(editor-render
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_BINARY_DIR}/generated
        ${DORIAX_ROOT}/libs/sokol
        ${DORIAX_ROOT}/libs/json
        ${DORIAX_ROOT}/libs/lua
        ${DORIAX_ROOT}/libs/luabridge3
        ${DORIAX_ROOT}/libs/box2d/include
        ${DORIAX_ROOT}/libs/joltphysics
        ${DORIAX_ROOT}/libs/stb
        ${DORIAX_ROOT}/libs/harfbuzz
        ${DORIAX_ROOT}/libs/sheenbidi/Headers
        ${DORIAX_ROOT}/libs/soloud/include        
        ${DORIAX_ROOT}/platform/common
        ${DORIAX_ROOT}/renders
        ${SHADERCOMPILER_ROOT}/src
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Platform-specific includes
if(API_BACKEND STREQUAL "linux")
    target_include_directories(editor-render PRIVATE ${DORIAX_ROOT}/platform/linux)
elseif(API_BACKEND STREQUAL "win")
    target_include_directories(editor-render PRIVATE ${DORIAX_ROOT}/platform/win)
elseif(API_BACKEND STREQUAL "mac")
    target_include_directories(editor-render PRIVATE ${DORIAX_ROOT}/platform/mac ${DORIAX_ROOT}/platform/apple)
endif()

# Link against required libraries
target_link_libraries(editor-render
    PRIVATE
        doriax
        yaml-cpp
)

# Add compile definitions
target_compile_definitions(editor-render
    PRIVATE
        DORIAX_EDITOR
        DORIAX_CRASH_GUARD
        NO_LUA_INIT
        WITH_MINIAUDIO
)

# Add graphic backend definitions
if(GRAPHIC_BACKEND STREQUAL "vulkan")
    target_compile_definitions(editor-render PRIVATE SOKOL_VULKAN)
elseif(GRAPHIC_BACKEND STREQUAL "metal")
    target_compile_definitions(editor-render PRIVATE SOKOL_METAL)
else()
    target_compile_definitions(editor-render PRIVATE SOKOL_GLCORE)
endif()

# Add native menu definition if applicable
if(NATIVE_MENU)
    target_compile_definitions(editor-render PRIVATE DORIAX_NATIVE_MENU)
endif()

set_target_properties(editor-render
    PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
