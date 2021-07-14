cmake_minimum_required (VERSION 3.6)

project(AsyncTest CXX)

set(SOURCE
    main.cpp
)

set(SHADERS
)

set(ASSETS
)

add_engine_app("AsyncTest" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
add_test(NAME AsyncTest COMMAND AsyncTest)
add_dependencies(MorpheusTests AsyncTest)