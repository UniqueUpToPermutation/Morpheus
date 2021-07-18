cmake_minimum_required (VERSION 3.6)

project(Im3dGizmo CXX)

set(SOURCE
    main.cpp
)

set(SHADERS
)

set(ASSETS
)

add_engine_app("Im3dGizmo" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
add_test(NAME Im3dGizmoTest COMMAND Im3dGizmo)
add_dependencies(MorpheusTests Im3dGizmo)