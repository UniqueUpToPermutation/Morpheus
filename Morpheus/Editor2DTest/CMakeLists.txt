cmake_minimum_required (VERSION 3.6)

project(Editor2DTest CXX)

set(SOURCE
    main.cpp
)

set(SHADERS
)

set(ASSETS
)

add_engine_app("Editor2DTest" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
