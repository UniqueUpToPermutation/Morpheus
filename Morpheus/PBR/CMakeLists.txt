cmake_minimum_required (VERSION 3.6)

project(Engine-PBR CXX)

set(SOURCE
    main.cpp
)

set(SHADERS
)

set(ASSETS
)

add_engine_app("Engine-PBR" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
