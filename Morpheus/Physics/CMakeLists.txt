cmake_minimum_required (VERSION 3.6)

project(Engine-Physics CXX)

set(SOURCE
    main.cpp
)

set(SHADERS
)

set(ASSETS
)

add_engine_app("Engine-Physics" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
