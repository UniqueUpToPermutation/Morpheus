cmake_minimum_required(VERSION 3.6)
project(Morpheus-Redux VERSION 0.1.0)

option(USE_PBRT "Compile and integrate the PBRT Backend" OFF)
option(USE_BOX2D "Compile and integrate Box2D" OFF)
option(USE_BULLET "Compile and integrate Bullet" OFF)
option(COMPILE_MESH2CPP "Should CMAKE compile the mesh2cpp utility?" ON)
option(BUILD_TESTS "Build tests for Morpheus" ON)
option(USE_GLFW "Include GLFW Platform" ON)

include(CTest)

if(USE_BOX2D)
	message("Integrating Box2D Backend...")
	add_definitions(-DUSE_BOX2D)
	add_subdirectory(box2d)
endif()

if(USE_BULLET)
	message("Integrating Bullet Backend...")
	add_definitions(-DUSE_BULLET)
	add_subdirectory(bullet3)
endif()

if(USE_PBRT)
	add_definitions(-DUSE_PBRT)
endif()

if (USE_GLFW)
	add_definitions(-DUSE_GLFW)
endif()

add_subdirectory(DiligentEngine)
add_subdirectory(Morpheus)

if(USE_PBRT)
	message("Integrating PBRT Backend...")
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/morpheus-pbrt/cmake)
	add_definitions(-DUSE_PBRT)
	add_subdirectory(morpheus-pbrt)
endif()

if(BUILD_TESTS)
	add_subdirectory(Tests)
endif()

if(COMPILE_MESH2CPP)
	add_subdirectory(mesh2cpp)
endif()
