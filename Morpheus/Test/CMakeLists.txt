cmake_minimum_required (VERSION 3.6)

project(Engine-Test CXX)

set(SOURCE
    main.cpp
)

set(SHADERS
)

set(ASSETS
)

add_engine_app("Engine-Test" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
