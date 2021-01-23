cmake_minimum_required (VERSION 3.6)

project(Engine-PBR CXX)

set(SOURCE
	main.cpp
	${CMAKE_SOURCE_DIR}/ImGuizmo/ImGuizmo.cpp
)

set(SHADERS
)

set(ASSETS
)

include_directories(Engine-PBR PRIVATE 
	${CMAKE_SOURCE_DIR}/ImGuizmo/)

add_engine_app("Engine-PBR" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
