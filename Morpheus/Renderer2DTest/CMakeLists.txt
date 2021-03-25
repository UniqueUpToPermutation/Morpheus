cmake_minimum_required (VERSION 3.6)

project(Renderer2D-Test CXX)

set(SOURCE
    main.cpp
)

set(SHADERS
)

set(ASSETS
)

add_engine_app("Renderer2D-Test" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
