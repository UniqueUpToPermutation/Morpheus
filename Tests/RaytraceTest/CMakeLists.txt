cmake_minimum_required (VERSION 3.6)

project(RaytraceTest CXX)

set(SOURCE
    main.cpp
)

set(SHADERS
)

set(ASSETS
)

add_engine_app("RaytraceTest" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
