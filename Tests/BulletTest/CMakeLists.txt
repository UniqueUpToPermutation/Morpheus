cmake_minimum_required (VERSION 3.6)

project(Bullet-Test CXX)

set(SOURCE
    main.cpp
)

set(SHADERS
)

set(ASSETS
)

add_engine_app("Bullet-Test" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
add_test(NAME Bullet-Test COMMAND Bullet-Test)
add_dependencies(MorpheusTests Bullet-Test)