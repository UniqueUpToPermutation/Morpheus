cmake_minimum_required (VERSION 3.6)

project(AsyncTest CXX)

set(SOURCE
    mesh2cpp.cpp
)

set(SHADERS
)

set(ASSETS
)

add_engine_app("mesh2cpp" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
