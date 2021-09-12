cmake_minimum_required (VERSION 3.6)

project(Morpheus-Engine)

# Include the bullet physics engine
if (USE_BULLET)
	INCLUDE_DIRECTORIES(
	${BULLET_PHYSICS_SOURCE_DIR}/src
	)

	LINK_LIBRARIES(
		BulletDynamics BulletCollision LinearMath
	)
endif()

if (USE_GLFW)
	INCLUDE_DIRECTORIES(
		${glfw_INCLUDE_DIRS}
	)

	LINK_LIBRARIES(
		${GLFW_LIB_NAME}
		${glfw_LIBRARIES}
	)
endif()

if (USE_BOX2D) 
	# Include the Box2D physics engine
	LINK_LIBRARIES(
		box2d
	)

	INCLUDE_DIRECTORIES(
		${CMAKE_SOURCE_DIR}/box2d/include/
	)
endif()

# Include package for embedding shaders in source code
add_subdirectory(shaders/cpp)
# Embed shaders (LINUX)
add_custom_command(
	OUTPUT shader_rc.cpp
	COMMAND embedfile ${CMAKE_CURRENT_SOURCE_DIR}/shaders shader_rc.cpp
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*)
message("embedfile ${CMAKE_CURRENT_SOURCE_DIR}/shaders shader_rc.cpp")

find_package(assimp REQUIRED)

list(APPEND SOURCE
	src/LightProbeProcessor.cpp
	src/HdriToCubemap.cpp
	src/ThreadPool.cpp
	src/Graphics.cpp
	src/RendererTransformCache.cpp
	src/SpriteBatch.cpp
	src/GeometryStructures.cpp
	src/Im3d.cpp
	src/Frame.cpp
	src/Reflection.cpp

	src/Systems/EmptyRenderer.cpp
	src/Systems/System.cpp
	
	src/Resources/Cache.cpp
	src/Resources/Shader.cpp
	src/Resources/Texture.cpp
	src/Resources/Geometry.cpp
	src/Resources/Material.cpp
	src/Resources/ShaderPreprocessor.cpp
	src/Resources/EmbeddedFileLoader.cpp
	src/Resources/ResourceData.cpp
	src/Resources/RawSampler.cpp
	src/Resources/TextureIterator.cpp
	src/Resources/Resource.cpp
	src/Resources/EmbeddedGeometry.cpp
	src/Resources/Buffer.cpp
	src/Resources/Cache.cpp
	src/Resources/FrameIO.cpp

	src/Components/Transform.cpp
	src/Components/SkyboxComponent.cpp
	src/Components/StaticMeshComponent.cpp
	src/Components/Camera.cpp

	${CMAKE_SOURCE_DIR}/lodepng/lodepng.cpp
	${CMAKE_SOURCE_DIR}/im3d/im3d.cpp
	shader_rc.cpp
)

if (USE_GLFW) 
	list(APPEND SOURCE
		src/PlatformGLFW.cpp)
endif()

list(APPEND INCLUDE
    include/Engine/LightProbeProcessor.hpp
    include/Engine/Entity.hpp
    include/Engine/GeometryStructures.hpp
    include/Engine/HdriToCubemap.hpp
    include/Engine/InputController.hpp
    include/Engine/Platform.hpp
    include/Engine/ThreadPool.hpp
	include/Engine/SpriteBatch.hpp
	include/Engine/Im3d.hpp
	include/Engine/Graphics.hpp
	include/Engine/Frame.hpp
	include/Engine/RendererTransformCache.hpp
	include/Engine/Reflection.hpp

	include/Engine/Systems/EmptyRenderer.hpp
	include/Engine/Systems/System.hpp

	include/Engine/Components/Transform.hpp
	include/Engine/Components/StaticMeshComponent.hpp
	include/Engine/Components/SkyboxComponent.hpp
	include/Engine/Components/LightProbe.hpp
	include/Engine/Components/ResourceComponent.hpp
	include/Engine/Components/Camera.hpp

	include/Engine/Resources/Cache.hpp
    include/Engine/Resources/EmbeddedFileLoader.hpp
    include/Engine/Resources/Geometry.hpp
    include/Engine/Resources/Resource.hpp
    include/Engine/Resources/ShaderPreprocessor.hpp
	include/Engine/Resources/Shader.hpp
    include/Engine/Resources/Texture.hpp
	include/Engine/Resources/ImageCopy.hpp
	include/Engine/Resources/ResourceData.hpp
	include/Engine/Resources/RawSampler.hpp
	include/Engine/Resources/TextureIterator.hpp
	include/Engine/Resources/Buffer.hpp
	include/Engine/Resources/FrameIO.hpp
	include/Engine/Resources/Material.hpp
)

if (USE_GLFW) 
	list(APPEND INCLUDE
		include/Engine/PlatformGLFW.hpp)
endif()

add_library(Morpheus-Engine STATIC ${SOURCE} ${INCLUDE})
set_common_target_properties(Morpheus-Engine)

target_include_directories(Morpheus-Engine
PUBLIC
	include
	${CMAKE_SOURCE_DIR}/gli
	${CMAKE_SOURCE_DIR}/gli/external
	${CMAKE_SOURCE_DIR}/lodepng
	${CMAKE_SOURCE_DIR}/im3d
	${CMAKE_SOURCE_DIR}/cereal/include/
	.
)

if(MSVC)
    target_compile_options(Morpheus-Engine PRIVATE -DUNICODE)
    add_msvc_ignores(Morpheus-Engine)

    if(PLATFORM_UNIVERSAL_WINDOWS)
        # Disable w4189: local variable is initialized but not referenced
        # Disable w4063: case is not a valid value for switch of enum
        # Consume the windows runtime extensions (/ZW)
        target_compile_options(Morpheus-Engine INTERFACE /wd4189 /wd4063 /ZW)
    endif()
endif()

get_supported_backends(ENGINE_LIBRARIES)

target_link_libraries(Morpheus-Engine 
PRIVATE 
    Diligent-BuildSettings
PUBLIC
    Diligent-Common
    Diligent-GraphicsTools
    Diligent-TextureLoader
    Diligent-TargetPlatform
    Diligent-Imgui
    ${ENGINE_LIBRARIES}
	Diligent-NativeAppBase
	${ASSIMP_LIBRARIES}
)

if(PLATFORM_UNIVERSAL_WINDOWS)
    target_link_libraries(Morpheus-Engine PRIVATE dxguid.lib)
elseif(PLATFORM_ANDROID)
    target_link_libraries(Morpheus-Engine PRIVATE GLESv3 PUBLIC native_app_glue)
elseif(PLATFORM_LINUX)
    target_link_libraries(Morpheus-Engine PRIVATE XCBKeySyms GL X11)
elseif(PLATFORM_MACOS OR PLATFORM_IOS)

endif()

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})

if(PLATFORM_WIN32)
    target_sources(Morpheus-Engine PRIVATE ${WIN32_RESOURCES})
    source_group("resources" FILES ${WIN32_RESOURCES})
endif()

set_target_properties(Morpheus-Engine PROPERTIES
    FOLDER Morpheus
)

# Add all third party libraries to the engine
add_third_party_libraries("Morpheus-Engine")
target_compile_features(Morpheus-Engine PRIVATE cxx_std_17)