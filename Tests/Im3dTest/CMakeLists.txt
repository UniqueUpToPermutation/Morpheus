cmake_minimum_required (VERSION 3.6)

project(Im3dTest CXX)

set(SOURCE
    main.cpp
)

set(SHADERS
)

set(ASSETS
)

add_engine_app("Im3dTest" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
add_test(NAME Im3dTest COMMAND Im3dTest)
add_dependencies(MorpheusTests Im3dTest)