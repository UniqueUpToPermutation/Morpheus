cmake_minimum_required (VERSION 3.6)

project(Raw-Texture-Test CXX)

set(SOURCE
    main.cpp
)

set(SHADERS
)

set(ASSETS
)

add_engine_app("Raw-Texture-Test" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
