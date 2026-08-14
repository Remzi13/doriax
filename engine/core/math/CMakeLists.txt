cmake_minimum_required(VERSION 3.15)

set(SOURCES
    AABB.h
    AABB.cpp
    Matrix3.h
    Matrix3.cpp
    Matrix4.h
    Matrix4.cpp
    OBB.h
    OBB.cpp
    Plane.h
    Plane.cpp
    Quaternion.h
    Quaternion.cpp
    Ray.h
    Ray.cpp
    Rect.h
    Rect.cpp
    Sphere.h
    Sphere.cpp
    Vector2.h
    Vector2.cpp
    Vector3.h
    Vector3.cpp
    Vector4.h
    Vector4.cpp
)

add_library(doriax_math STATIC
    ${SOURCES}
)

source_group("engine/math" FILES ${SOURCES})

target_include_directories(doriax_math
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)

target_compile_features(doriax_math PUBLIC cxx_std_17)

set_target_properties(doriax_math PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
