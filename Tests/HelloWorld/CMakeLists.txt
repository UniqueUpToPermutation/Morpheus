cmake_minimum_required (VERSION 3.6)

project(EngineHelloWorld CXX)

set(SOURCE
    main.cpp
)

set(SHADERS
)

set(ASSETS
)

add_engine_app("EngineHelloWorld" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
add_test(NAME HelloWorldTest COMMAND EngineHelloWorld)
add_dependencies(MorpheusTests EngineHelloWorld)