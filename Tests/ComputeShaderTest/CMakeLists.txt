cmake_minimum_required (VERSION 3.6)

project(ComputeTest CXX)

add_custom_command(
	OUTPUT shader_rc.cpp
	COMMAND embedfile ${CMAKE_CURRENT_SOURCE_DIR}/shaders shader_rc.cpp AddEmbeddedShaders
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*)

message("embedfile ${CMAKE_CURRENT_SOURCE_DIR}/shaders shader_rc.cpp AddEmbeddedShaders")

set(SOURCE
    main.cpp
	shader_rc.cpp
)

set(SHADERS
)

set(ASSETS
)

add_engine_app("ComputeTest" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")

add_test(NAME ComputeTest COMMAND ComputeTest)
add_dependencies(MorpheusTests ComputeTest)