cmake_minimum_required (VERSION 3.6)

project(TextureTest CXX)

set(SOURCE
    main.cpp
)

set(SHADERS
)

set(ASSETS
)

add_engine_app("TextureTest" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
add_test(NAME TextureTest COMMAND TextureTest)
add_dependencies(MorpheusTests TextureTest)