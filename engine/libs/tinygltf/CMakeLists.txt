add_library(
    tinygltf
    STATIC
    tiny_gltf.cc
)

target_include_directories(tinygltf PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../stb
    ${CMAKE_CURRENT_SOURCE_DIR}/../json
)

find_package(Threads REQUIRED)
target_link_libraries(tinygltf PRIVATE Threads::Threads)

if(EMSCRIPTEN AND EMSCRIPTEN_THREAD_SUPPORT)
	target_compile_options(tinygltf PRIVATE -pthread)
endif()

target_link_libraries(
    tinygltf

    PUBLIC

    stb
)

set_target_properties(
    tinygltf

    PROPERTIES

    CXX_STANDARD 11
)