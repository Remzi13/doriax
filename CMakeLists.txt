cmake_minimum_required(VERSION 3.20)
project(doriax-editor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tag when the commit has one, short hash otherwise. CI passes it in, local
# builds ask git so Help > About shows something real.
if(NOT DORIAXEDITOR_VERSION)
    find_package(Git QUIET)
    if(GIT_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE DORIAXEDITOR_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
    endif()
endif()

# Git allows characters that would break the generated C++ and .rc string literals
string(REGEX REPLACE "[^A-Za-z0-9._+-]" "" DORIAXEDITOR_VERSION "${DORIAXEDITOR_VERSION}")

if(NOT DORIAXEDITOR_VERSION)
    set(DORIAXEDITOR_VERSION "unknown")
endif()

# Windows VERSIONINFO wants four integers, which only a vX.Y[.Z] tag can give.
set(DORIAX_VERSION_NUMERIC "0,0,0,0")
if(DORIAXEDITOR_VERSION MATCHES "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
    set(DORIAX_VERSION_NUMERIC "${CMAKE_MATCH_1},${CMAKE_MATCH_2},${CMAKE_MATCH_3},0")
elseif(DORIAXEDITOR_VERSION MATCHES "^v?([0-9]+)\\.([0-9]+)$")
    set(DORIAX_VERSION_NUMERIC "${CMAKE_MATCH_1},${CMAKE_MATCH_2},0,0")
endif()

message(STATUS "Doriax version is ${DORIAXEDITOR_VERSION}")

if(APPLE)
    enable_language(OBJCXX)
endif()

set(DORIAX_SHARED ON)
set(GRAPHIC_BACKEND "glcore")

option(DORIAX_BUILD_EDITOR "Build the editor executable" ON)
option(DORIAX_BUILD_ENGINE "Build the engine library" ON)
option(DORIAX_BUILD_SHADERCOMPILER "Build the shader compiler" ON)

set(ENABLE_OPT OFF CACHE BOOL "Enables spirv-opt capability if present" FORCE)

# Tracy Profiler Integration Option
option(TRACY_ENABLE "Enable Tracy Profiler" ON)

if(TRACY_ENABLE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(TRACY_STATIC OFF CACHE BOOL "Build Tracy as a shared library for PIC compatibility" FORCE)
    set(TRACY_ON_DEMAND ON CACHE BOOL "" FORCE)
    add_subdirectory(libs/tracy EXCLUDE_FROM_ALL)

    add_compile_definitions(TRACY_ENABLE)
    add_compile_definitions(TRACY_IMPORTS)
endif()

if(NOT API_BACKEND)
    set(API_BACKEND "glfw")
endif()
message(STATUS "API backend is set to ${API_BACKEND}")

if(POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW) # NEW for GLVND, OLD for legacy OpenGL
endif()

if(WIN32)
    enable_language(RC)

    set(PLATFORM_EXEC_FLAGS WIN32)
 
    if(MSVC)
        set(PLATFORM_PROPERTIES_FLAGS LINK_FLAGS "/ENTRY:mainCRTStartup")
        if(DORIAX_SHARED)
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
        else()
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        endif()
    endif()

    if(MINGW)
        set(MINGW_STATIC_FLAGS -static -static-libgcc -static-libstdc++)
    endif()

    set(CMAKE_PREFIX_PATH "$ENV{ProgramFiles}/GLFW" ${CMAKE_PREFIX_PATH})
    set(CMAKE_PREFIX_PATH "$ENV{ProgramFiles\(X86\)}/GLFW" ${CMAKE_PREFIX_PATH})
endif()

set(IMGUI_DIR libs/imgui)
set(EDITOR_DIR editor)

# Icon and version resource for one executable. Each target needs its own so
# OriginalFilename matches the file on disk. Added after Defender quarantined
# doriax-editor-cmd.exe, the only binary that shipped without any resource.
function(doriax_add_windows_resource out_var target_name icon_name original_filename description)
    set(DORIAX_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${EDITOR_DIR}/resources/icons/${icon_name}")
    set(DORIAX_ORIGINAL_FILENAME ${original_filename})
    set(DORIAX_FILE_DESCRIPTION ${description})
    set(DORIAX_INTERNAL_NAME ${target_name})

    set(generated_rc "${CMAKE_CURRENT_BINARY_DIR}/resources/${target_name}.rc")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/${EDITOR_DIR}/resources/icons/doriax-editor.rc.in"
        ${generated_rc}
        @ONLY
    )
    set_property(SOURCE ${generated_rc} APPEND PROPERTY OBJECT_DEPENDS ${DORIAX_ICON_FILE})

    add_library(${target_name}-resource OBJECT ${generated_rc})
    # rc.exe chokes on the editor's include list, and the .rc needs no search path
    set_property(TARGET ${target_name}-resource PROPERTY INCLUDE_DIRECTORIES "")

    set(${out_var} $<TARGET_OBJECTS:${target_name}-resource> PARENT_SCOPE)
endfunction()

set(DORIAX_EDITOR_ICON_OBJECTS)
if(WIN32)
    doriax_add_windows_resource(
        DORIAX_EDITOR_ICON_OBJECTS
        doriax-editor
        "doriax-editor.ico"
        "doriax-editor.exe"
        "Doriax Engine Editor"
    )
endif()

# Find OpenGL
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
# The AI HTTP client uses the native WinHTTP backend on Windows (no external
# dependency) and libcurl everywhere else (see editor/ai/HttpClient.cpp).
if(NOT WIN32)
    find_package(CURL REQUIRED)
endif()

if(UNIX AND NOT APPLE)
    find_package(X11 QUIET)
endif()

set(BACKEND_SOURCES)
set(BACKEND_LIBS)

if(API_BACKEND STREQUAL "glfw")
    find_package(glfw3 REQUIRED)

    if(NOT DEFINED GLFW_LIBRARIES)
        set(GLFW_LIBRARIES glfw)
    endif()

    list(APPEND BACKEND_SOURCES
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${EDITOR_DIR}/backend/BackendGLFW.cpp
    )

    list(APPEND BACKEND_LIBS
        ${GLFW_LIBRARIES}
    )
endif()

if(API_BACKEND STREQUAL "sdl")
    find_package(SDL2 REQUIRED)

    list(APPEND BACKEND_SOURCES
        ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
        ${EDITOR_DIR}/backend/BackendSDL.cpp
    )

    list(APPEND BACKEND_LIBS
        ${SDL2_LIBRARIES}
    )
endif()

set(DORIAX_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/engine)
set(SHADERCOMPILER_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/shadercompiler)

add_compile_definitions(
    DORIAX_EDITOR
    DORIAX_CRASH_GUARD
    NO_LUA_INIT
)

set(NFD_PORTAL ON CACHE BOOL "Use xdg-desktop-portal instead of GTK") # For NFD

set(SHADERS_DIR "${CMAKE_SOURCE_DIR}/engine/tools/shaderlib")
set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
set(SHADERS_HEADER "${GENERATED_DIR}/shaders.h")
set(GENERATE_SHADERS_SCRIPT "${CMAKE_SOURCE_DIR}/generate_shaders.cmake")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${EDITOR_DIR}/EditorVersion.h.in"
    "${GENERATED_DIR}/EditorVersion.h"
    @ONLY
)

file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS "${SHADERS_DIR}/*.glsl" "${SHADERS_DIR}/*.frag" "${SHADERS_DIR}/*.vert")

add_custom_command(
    OUTPUT ${SHADERS_HEADER}
    COMMAND ${CMAKE_COMMAND}
            -DSHADERS_DIR=${SHADERS_DIR}
            -DSHADER_FILES="${SHADER_FILES}"
            -DSHADERS_HEADER=${SHADERS_HEADER}
            -P ${GENERATE_SHADERS_SCRIPT}
    DEPENDS ${GENERATE_SHADERS_SCRIPT} ${SHADER_FILES}
    COMMENT "Generating shaders.h"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Generate engine API suggestions header from Lua bindings and C++ headers
set(BINDING_DIR "${CMAKE_SOURCE_DIR}/engine/core/script/binding")
set(ENGINE_CORE_DIR "${CMAKE_SOURCE_DIR}/engine/core")
set(API_SUGGESTIONS_HEADER "${GENERATED_DIR}/engine_api_suggestions.h")
set(GENERATE_API_SCRIPT "${CMAKE_SOURCE_DIR}/generate_api_suggestions.py")

file(GLOB BINDING_FILES CONFIGURE_DEPENDS "${BINDING_DIR}/*.cpp")
file(GLOB_RECURSE ENGINE_HEADERS CONFIGURE_DEPENDS "${ENGINE_CORE_DIR}/*.h")

add_custom_command(
    OUTPUT ${API_SUGGESTIONS_HEADER}
    COMMAND ${Python3_EXECUTABLE} ${GENERATE_API_SCRIPT}
            ${BINDING_DIR} ${API_SUGGESTIONS_HEADER} ${ENGINE_CORE_DIR}
    DEPENDS ${GENERATE_API_SCRIPT} ${BINDING_FILES} ${ENGINE_HEADERS}
    COMMENT "Generating engine_api_suggestions.h from Lua bindings and C++ headers"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

if(DORIAX_BUILD_ENGINE)
    add_subdirectory(${DORIAX_ROOT})
endif()

if(DORIAX_BUILD_SHADERCOMPILER)
    add_subdirectory(${SHADERCOMPILER_ROOT} EXCLUDE_FROM_ALL)
endif()

add_subdirectory(libs/nfd)
add_subdirectory(libs/yaml-cpp)

# Add ImGui source files
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_custom_target(generate_shaders DEPENDS ${SHADERS_HEADER})
add_custom_target(generate_api_suggestions DEPENDS ${API_SUGGESTIONS_HEADER})

set(EDITOR_CORE_SOURCES
    ${EDITOR_DIR}/main.cpp
    ${EDITOR_DIR}/cli/CommandLine.cpp
    ${EDITOR_DIR}/App.cpp
    ${EDITOR_DIR}/AppSettings.cpp
    ${EDITOR_DIR}/Theme.cpp
    ${EDITOR_DIR}/Factory.cpp
    ${EDITOR_DIR}/Out.cpp
    ${EDITOR_DIR}/Platform.cpp
    ${EDITOR_DIR}/Project.cpp
    ${EDITOR_DIR}/Catalog.cpp
    ${EDITOR_DIR}/Conector.cpp
    ${EDITOR_DIR}/EditorHost.cpp
    ${EDITOR_DIR}/Generator.cpp
    ${EDITOR_DIR}/Stream.cpp
    ${EDITOR_DIR}/Exporter.cpp
)

set(EDITOR_AI_SOURCES
    ${EDITOR_DIR}/ai/AiPathUtils.cpp
    ${EDITOR_DIR}/ai/AiProvider.cpp
    ${EDITOR_DIR}/ai/AiService.cpp
    ${EDITOR_DIR}/ai/AiEngineApiContext.cpp
    ${EDITOR_DIR}/ai/AiTerrainCapability.cpp
    ${EDITOR_DIR}/ai/AiTypes.cpp
    ${EDITOR_DIR}/ai/ConversationStore.cpp
    ${EDITOR_DIR}/ai/EditorActionExecutor.cpp
    ${EDITOR_DIR}/ai/EditorActionRegistry.cpp
    ${EDITOR_DIR}/ai/HttpClient.cpp
    ${EDITOR_DIR}/ai/ModelCatalog.cpp
    ${EDITOR_DIR}/ai/RateLimitRetry.cpp
    ${EDITOR_DIR}/ai/SecretStore.cpp
)

set(EDITOR_UTIL_SOURCES
    ${EDITOR_DIR}/util/ShaderHeaderBuilder.cpp
    ${EDITOR_DIR}/util/GraphicUtils.cpp
    ${EDITOR_DIR}/util/CameraTextureLink.cpp
    ${EDITOR_DIR}/util/Clipboard.cpp
    ${EDITOR_DIR}/util/EntityBundle.cpp
    ${EDITOR_DIR}/util/ProjectUtils.cpp
    ${EDITOR_DIR}/util/ScriptParser.cpp
    ${EDITOR_DIR}/util/PngWriter.cpp
    ${EDITOR_DIR}/util/TerrainMapFileWriter.cpp
    ${EDITOR_DIR}/util/UIUtils.cpp
    ${EDITOR_DIR}/util/CommandRunner.cpp
    ${EDITOR_DIR}/shader/ShaderBuilder.cpp
)

set(EDITOR_WINDOW_SOURCES
    ${EDITOR_DIR}/window/Properties.cpp
    ${EDITOR_DIR}/window/OutputWindow.cpp
    ${EDITOR_DIR}/window/SceneWindow.cpp
    ${EDITOR_DIR}/window/Structure.cpp
    ${EDITOR_DIR}/window/ResourcesWindow.cpp
    ${EDITOR_DIR}/window/ImageViewerWindow.cpp
    ${EDITOR_DIR}/window/CodeEditor.cpp
    ${EDITOR_DIR}/window/AnimationWindow.cpp
    ${EDITOR_DIR}/window/TerrainEditWindow.cpp
    ${EDITOR_DIR}/window/AiChatWindow.cpp
    ${EDITOR_DIR}/window/LoadingWindow.cpp
    ${EDITOR_DIR}/window/dialog/SceneSaveDialog.cpp
    ${EDITOR_DIR}/window/dialog/ComponentAddDialog.cpp
    ${EDITOR_DIR}/window/dialog/ProjectSaveDialog.cpp
    ${EDITOR_DIR}/window/dialog/ScriptCreateDialog.cpp
    ${EDITOR_DIR}/window/dialog/TextureSlicerToolDialog.cpp
    ${EDITOR_DIR}/window/dialog/ExportWindow.cpp
    ${EDITOR_DIR}/window/dialog/ProjectSettingsWindow.cpp
    ${EDITOR_DIR}/window/dialog/AiSettingsWindow.cpp
    ${EDITOR_DIR}/window/widget/CustomTextEditor.cpp
    ${EDITOR_DIR}/window/widget/InputTextContextMenu.cpp
    ${EDITOR_DIR}/window/widget/SemanticSuggestions.cpp
    ${EDITOR_DIR}/window/widget/SelectableTextView.cpp
)

set(EDITOR_RENDER_SOURCES
    ${EDITOR_DIR}/render/ToolsLayer.cpp
    ${EDITOR_DIR}/render/UILayer.cpp
    ${EDITOR_DIR}/render/SceneRender.cpp
    ${EDITOR_DIR}/render/SceneRender2D.cpp
    ${EDITOR_DIR}/render/SceneRender3D.cpp
    ${EDITOR_DIR}/render/gizmo/TranslateGizmo.cpp
    ${EDITOR_DIR}/render/gizmo/RotateGizmo.cpp
    ${EDITOR_DIR}/render/gizmo/ScaleGizmo.cpp
    ${EDITOR_DIR}/render/gizmo/Object2DGizmo.cpp
    ${EDITOR_DIR}/render/gizmo/AnchorGizmo.cpp
    ${EDITOR_DIR}/render/gizmo/ViewportGizmo.cpp
    ${EDITOR_DIR}/render/preview/MaterialRender.cpp
    ${EDITOR_DIR}/render/preview/MeshPreviewRender.cpp
    ${EDITOR_DIR}/render/preview/DirectionRender.cpp
    ${EDITOR_DIR}/render/preview/ModelRender.cpp
)

set(EDITOR_COMMAND_SOURCES
    ${EDITOR_DIR}/command/CommandHandle.cpp
    ${EDITOR_DIR}/command/CommandHistory.cpp
    ${EDITOR_DIR}/command/type/AddChildSceneCmd.cpp
    ${EDITOR_DIR}/command/type/AddComponentCmd.cpp
    ${EDITOR_DIR}/command/type/RemoveComponentCmd.cpp
    ${EDITOR_DIR}/command/type/CreateEntityCmd.cpp
    ${EDITOR_DIR}/command/type/CreateTrackKeyCmd.cpp
    ${EDITOR_DIR}/command/type/DeleteEntityCmd.cpp
    ${EDITOR_DIR}/command/type/DuplicateEntityCmd.cpp
    ${EDITOR_DIR}/command/type/MoveEntityOrderCmd.cpp
    ${EDITOR_DIR}/command/type/ObjectTransformCmd.cpp
    ${EDITOR_DIR}/command/type/RemoveChildSceneCmd.cpp
    ${EDITOR_DIR}/command/type/SetChildSceneStartActiveCmd.cpp
    ${EDITOR_DIR}/command/type/EntityNameCmd.cpp
    ${EDITOR_DIR}/command/type/ImportEntityBundleCmd.cpp
    ${EDITOR_DIR}/command/type/RemoveEntityFromBundleCmd.cpp
    ${EDITOR_DIR}/command/type/AddEntityToBundleCmd.cpp
    ${EDITOR_DIR}/command/type/MeshChangeCmd.cpp
    ${EDITOR_DIR}/command/type/ModelLoadCmd.cpp
    ${EDITOR_DIR}/command/type/SceneNameCmd.cpp
    ${EDITOR_DIR}/command/type/CopyFileCmd.cpp
    ${EDITOR_DIR}/command/type/RenameFileCmd.cpp
    ${EDITOR_DIR}/command/type/CreateDirCmd.cpp
    ${EDITOR_DIR}/command/type/DeleteFileCmd.cpp
    ${EDITOR_DIR}/command/type/SetMainCameraCmd.cpp
    ${EDITOR_DIR}/command/type/CreateMaterialFileCmd.cpp
    ${EDITOR_DIR}/command/type/UnlinkMaterialCmd.cpp
    ${EDITOR_DIR}/command/type/LinkMaterialCmd.cpp
    ${EDITOR_DIR}/command/type/CreateEntityBundleCmd.cpp
    ${EDITOR_DIR}/command/type/ComponentToBundleLocalCmd.cpp
    ${EDITOR_DIR}/command/type/ComponentToBundleSharedCmd.cpp
    ${EDITOR_DIR}/command/type/ForkShaderCmd.cpp
)

set(DORIAX_EDITOR_SOURCES
    ${EDITOR_CORE_SOURCES}
    ${EDITOR_AI_SOURCES}
    ${EDITOR_UTIL_SOURCES}
    ${EDITOR_WINDOW_SOURCES}
    ${EDITOR_RENDER_SOURCES}
    ${EDITOR_COMMAND_SOURCES}
    ${IMGUI_SOURCES}
    ${BACKEND_SOURCES}
)

if(APPLE)
    # NSPasteboard image/file-URL reading for the AI chat attachment paste.
    list(APPEND DORIAX_EDITOR_SOURCES ${EDITOR_DIR}/util/ClipboardMac.mm)
endif()

function(configure_doriax_editor_target target_name)
    add_dependencies(${target_name} generate_shaders generate_api_suggestions)

    target_compile_definitions(${target_name} PRIVATE
        DORIAX_EDITOR
        DORIAX_CRASH_GUARD
        NO_LUA_INIT
        SOKOL_GLCORE
        WITH_MINIAUDIO
    )

    target_include_directories(${target_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/${EDITOR_DIR}
        ${DORIAX_ROOT}/platform/glfw
        ${SHADERCOMPILER_ROOT}/src
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${OPENGL_INCLUDE_DIR}
        ${GENERATED_DIR}
    )

    if(TRACY_ENABLE)
        target_include_directories(${target_name} PRIVATE libs/tracy/public)
    endif()

    target_link_libraries(
        ${target_name}

        PRIVATE

        ${BACKEND_LIBS}
        ${OPENGL_LIBRARIES}
        ${MINGW_STATIC_FLAGS}
        Threads::Threads
        doriax
        shadercompiler
        nfd
        yaml-cpp
    )

    if(WIN32)
        target_link_libraries(${target_name} PRIVATE winhttp shell32)
    else()
        target_link_libraries(${target_name} PRIVATE CURL::libcurl)
    endif()

    if(X11_FOUND)
        target_compile_definitions(${target_name} PRIVATE DORIAX_HAS_X11_CLIPBOARD)
        target_link_libraries(${target_name} PRIVATE X11::X11)
    endif()

    if(APPLE)
        target_link_libraries(${target_name} PRIVATE "-framework AppKit")
    endif()

    if(APPLE)
        set_target_properties(${target_name} PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            BUILD_RPATH "@loader_path"
            INSTALL_RPATH "@loader_path"
        )
    elseif(UNIX)
        set_target_properties(${target_name} PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            BUILD_RPATH "$ORIGIN;$ORIGIN/libs/tracy;${CMAKE_BINARY_DIR}/libs/tracy"
            INSTALL_RPATH "$ORIGIN"
        )
    endif()
endfunction()

if(DORIAX_BUILD_EDITOR AND NOT DORIAX_BUILD_ENGINE)
    message(FATAL_ERROR "DORIAX_BUILD_EDITOR requires DORIAX_BUILD_ENGINE=ON")
endif()

if(DORIAX_BUILD_EDITOR)
    add_executable(
        doriax-editor

        ${PLATFORM_EXEC_FLAGS}

        ${DORIAX_EDITOR_SOURCES}
        ${DORIAX_EDITOR_ICON_OBJECTS}
    )

    configure_doriax_editor_target(doriax-editor)

    if(PLATFORM_PROPERTIES_FLAGS)
        set_target_properties(doriax-editor PROPERTIES ${PLATFORM_PROPERTIES_FLAGS})
    endif()

    if(TRACY_ENABLE)
        if (WIN32)
            add_custom_command(TARGET doriax-editor POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${CMAKE_BINARY_DIR}/libs/tracy/$<CONFIG>/TracyClient.dll
                    $<TARGET_FILE_DIR:doriax-editor>/TracyClient.dll
                COMMENT "Copying TracyClient.dll next to the editor executable"
            )
        elseif (UNIX)
            add_custom_command(TARGET doriax-editor POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${CMAKE_BINARY_DIR}/libs/tracy/libTracyClient.so
                    $<TARGET_FILE_DIR:doriax-editor>/libTracyClient.so
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${CMAKE_BINARY_DIR}/libs/tracy/libTracyClient.so.0.13.6
                    $<TARGET_FILE_DIR:doriax-editor>/libTracyClient.so.0.13.6
                COMMENT "Copying Tracy runtime library next to the editor executable"
            )
        endif()
    endif()

    add_custom_target(copy_doriax_editor_engine
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/engine
            $<TARGET_FILE_DIR:doriax-editor>/engine
        COMMENT "Copying engine to executable directory"
    )

    add_dependencies(doriax-editor copy_doriax_editor_engine)

    set(DORIAX_EDITOR_INSTALL_TARGETS doriax-editor)

    # Windows GUI-subsystem executables do not behave like normal CLI programs
    # when launched from cmd.exe/PowerShell. Provide a console-subsystem twin for
    # automation while keeping Unix-like platforms on a single editor binary.
    #
    # EXCLUDE_FROM_ALL keeps it out of the default ("all") build, so local and IDE
    # builds only produce doriax-editor. Release builds compile it explicitly by
    # name (`cmake --build --target doriax-editor-cmd`, see
    # .github/workflows/cmake.yml); the install rule below still installs it because
    # CI builds it before installing.
    if(WIN32)
        # Its own icon because both executables install to the same folder
        doriax_add_windows_resource(
            DORIAX_EDITOR_CMD_ICON_OBJECTS
            doriax-editor-cmd
            "doriax-editor-cmd.ico"
            "doriax-editor-cmd.exe"
            "Doriax Engine Editor (command line)"
        )

        add_executable(
            doriax-editor-cmd

            EXCLUDE_FROM_ALL

            ${DORIAX_EDITOR_SOURCES}
            ${DORIAX_EDITOR_CMD_ICON_OBJECTS}
        )

        configure_doriax_editor_target(doriax-editor-cmd)
        add_dependencies(doriax-editor-cmd copy_doriax_editor_engine)
        list(APPEND DORIAX_EDITOR_INSTALL_TARGETS doriax-editor-cmd)
    endif()
else()
    set(DORIAX_EDITOR_INSTALL_TARGETS)
endif()

set(DORIAX_INSTALL_TARGETS)
if(DORIAX_BUILD_ENGINE)
    list(APPEND DORIAX_INSTALL_TARGETS doriax)
endif()
if(DORIAX_BUILD_EDITOR)
    list(APPEND DORIAX_INSTALL_TARGETS ${DORIAX_EDITOR_INSTALL_TARGETS})
endif()

install(TARGETS ${DORIAX_INSTALL_TARGETS}
        CONFIGURATIONS Release MinSizeRel 
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION bin
)

if(DORIAX_BUILD_ENGINE)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/engine
            DESTINATION bin
            CONFIGURATIONS Release MinSizeRel
    )
endif()
