cmake_minimum_required (VERSION 3.6)

project(Pbrt-Test CXX)

set(SOURCE
    main.cpp
)

set(SHADERS
)

set(ASSETS
)

add_engine_app("Pbrt-Test" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
morpheus_pbrt_app("Pbrt-Test")

add_test(NAME Pbrt-Test COMMAND Pbrt-Test)
add_dependencies(MorpheusTests Pbrt-Test)