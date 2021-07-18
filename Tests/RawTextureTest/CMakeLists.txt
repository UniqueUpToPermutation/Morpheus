cmake_minimum_required (VERSION 3.6)

project(RawTextureTest CXX)

set(SOURCE
    main.cpp
)

set(SHADERS
)

set(ASSETS
)

add_engine_app("RawTextureTest" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
add_test(NAME RawTextureTest COMMAND RawTextureTest)
add_dependencies(MorpheusTests RawTextureTest)