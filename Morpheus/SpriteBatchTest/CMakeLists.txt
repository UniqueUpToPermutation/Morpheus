cmake_minimum_required (VERSION 3.6)

project(SpriteBatchTest CXX)

set(SOURCE
    main.cpp
)

set(SHADERS
)

set(ASSETS
)

add_engine_app("SpriteBatchTest" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
add_test(NAME SpriteBatchTest COMMAND SpriteBatchTest)
add_dependencies(MorpheusTests SpriteBatchTest)