cmake_minimum_required (VERSION 3.6)

project(Morpheus-Engine)

# Include the bullet physics engine
INCLUDE_DIRECTORIES(
${BULLET_PHYSICS_SOURCE_DIR}/src
)

LINK_LIBRARIES(
	BulletDynamics BulletCollision LinearMath
)

# Include the Box2D physics engine
LINK_LIBRARIES(
	box2d
)

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/box2d/include/
)

# Include package for embedding shaders in source code
add_subdirectory(shaders/cpp)
# Embed shaders (LINUX)
add_custom_command(
	OUTPUT shader_rc.cpp
	COMMAND embedfile ${CMAKE_CURRENT_SOURCE_DIR}/shaders shader_rc.cpp
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*)
message("embedfile ${CMAKE_CURRENT_SOURCE_DIR}/shaders shader_rc.cpp")

find_package(assimp REQUIRED)

if(PLATFORM_WIN32)
    set(SOURCE 
        src/Win32/PlatformWin32.cpp
        src/Win32/InputControllerWin32.cpp
    )
    set(INCLUDE 
        include/Engine/Win32/PlatformWin32.hpp
        include/Engine/Win32/InputControllerWin32.hpp
    )
    set(WIN32_RESOURCES
        src/Win32/resources/Win32AppResource.h
        src/Win32/resources/directx11-logo.bmp
        src/Win32/resources/directx12-logo.bmp
        src/Win32/resources/vulkan-logo.bmp
        src/Win32/resources/opengl-logo.bmp
        src/Win32/resources/Win32AppResource.rc
    )

    function(append_sample_base_win32_source TARGET_NAME)
        get_target_property(SAMPLE_BASE_SOURCE_DIR Morpheus-Engine SOURCE_DIR)
        # We need to add .res file to executable target to make resources available
        set(RES_FILE ${SAMPLE_BASE_SOURCE_DIR}/src/Win32/resources/Win32AppResource.rc)
        target_sources(${TARGET_NAME} PRIVATE ${RES_FILE})
        source_group("resources" FILES ${RES_FILE})
    endfunction()

elseif(PLATFORM_UNIVERSAL_WINDOWS)

    # Windows Runtime types cannot be included into static libraries
    # https://social.msdn.microsoft.com/Forums/en-US/269db513-64ef-4817-a025-43954f614eb3/lnk4264-why-are-static-libraries-not-recommended-when-authoring-windows-runtime-types?forum=winappswithnativecode
    # So as a workaround, we will include all source files into the target app project
    function(append_sample_base_uwp_source TARGET_NAME)
        get_target_property(SAMPLE_BASE_SOURCE_DIR Morpheus-Engine SOURCE_DIR)

        set(SAMPLE_BASE_UWP_SOURCE
            ${SAMPLE_BASE_SOURCE_DIR}/src/UWP/ImguiUWPEventHelper.cpp
            ${SAMPLE_BASE_SOURCE_DIR}/src/UWP/SampleAppUWP.cpp
            ${SAMPLE_BASE_SOURCE_DIR}/src/UWP/InputControllerEventHandlerUWP.cpp
            ${SAMPLE_BASE_SOURCE_DIR}/src/EngineApp.cpp
        )

        set(SAMPLE_BASE_UWP_INCLUDE
            ${SAMPLE_BASE_SOURCE_DIR}/src/UWP/ImguiUWPEventHelper.h
            ${SAMPLE_BASE_SOURCE_DIR}/src/UWP/InputControllerEventHandlerUWP.h
            ${SAMPLE_BASE_SOURCE_DIR}/include/EngineApp.hpp
            ${SAMPLE_BASE_SOURCE_DIR}/include/UWP/InputControllerUWP.hpp
        )

        set(SAMPLE_BASE_UWP_INCLUDE_DIR
            ${SAMPLE_BASE_SOURCE_DIR}/src/UWP
        )

        target_sources(${TARGET_NAME} PRIVATE ${SAMPLE_BASE_UWP_SOURCE} ${SAMPLE_BASE_UWP_INCLUDE})
        source_group("src\\SampleBase" FILES ${SAMPLE_BASE_UWP_SOURCE})
        source_group("include\\SampleBase" FILES ${SAMPLE_BASE_UWP_INCLUDE})
        target_include_directories(${TARGET_NAME} PRIVATE ${SAMPLE_BASE_UWP_INCLUDE_DIR})
    endfunction()

elseif(PLATFORM_ANDROID)
    set(SOURCE
        src/Android/InputControllerAndroid.cpp
        src/Android/SampleAppAndroid.cpp
    )
    set(INCLUDE 
        include/Engine/Android/InputControllerAndroid.hpp
    )
elseif(PLATFORM_LINUX)
    set(SOURCE 
        src/Linux/InputControllerLinux.cpp
		src/Linux/PlatformLinux.cpp
    )
    set(INCLUDE 
        include/Engine/Linux/InputControllerLinux.hpp
		include/Engine/Linux/PlatformLinux.hpp
    )
elseif(PLATFORM_MACOS)

    set(SOURCE
        src/MacOS/InputControllerMacOS.cpp
        src/MacOS/SampleAppMacOS.mm
    )
    set(INCLUDE
        Include/Engine/MacOS/InputControllerMacOS.hpp
    )

elseif(PLATFORM_IOS)
    set(SOURCE
        src/IOS/InputControllerIOS.cpp
        src/IOS/SampleAppIOS.cpp
    )
    set(INCLUDE
        include/Engine/IOS/InputControllerIOS.hpp
    )

endif()

list(APPEND SOURCE
	src/LightProbeProcessor.cpp
	src/HdriToCubemap.cpp
	src/Camera.cpp
	src/ThreadPool.cpp
	src/Graphics.cpp
	src/RendererTransformCache.cpp
	src/SpriteBatch.cpp
	src/Loading.cpp
	src/GeometryStructures.cpp
	src/Im3d.cpp
	src/Renderer.cpp

	src/Systems/GeometryCache.cpp
	src/Systems/TextureCache.cpp
	src/Systems/DefaultRenderer.cpp
	src/Systems/EmptyRenderer.cpp
	src/Systems/System.cpp
	src/Systems/ImGuiSystem.cpp
	src/Systems/SimpleFPSCameraSystem.cpp

	src/Resources/Shader.cpp
	src/Resources/Texture.cpp
	src/Resources/Geometry.cpp
	src/Resources/ShaderPreprocessor.cpp
	src/Resources/EmbeddedFileLoader.cpp
	src/Resources/ResourceData.cpp
	src/Resources/ResourceSerialization.cpp
	src/Resources/RawSampler.cpp
	src/Resources/TextureIterator.cpp
	src/Resources/Resource.cpp
	src/Resources/EmbeddedGeometry.cpp

	src/Components/Transform.cpp

	src/Raytrace/DefaultRaytracer.cpp

	${CMAKE_SOURCE_DIR}/lodepng/lodepng.cpp
	${CMAKE_SOURCE_DIR}/im3d/im3d.cpp
	shader_rc.cpp
)

list(APPEND INCLUDE
    include/Engine/LightProbeProcessor.hpp
    include/Engine/Camera.hpp
    include/Engine/Entity.hpp
    include/Engine/GeometryStructures.hpp
    include/Engine/HdriToCubemap.hpp
    include/Engine/InputController.hpp
    include/Engine/Platform.hpp
    include/Engine/ThreadPool.hpp
	include/Engine/SpriteBatch.hpp
	include/Engine/Loading.hpp
	include/Engine/Im3d.hpp
	include/Engine/Renderer.hpp
	include/Engine/Graphics.hpp
	include/Engine/Frame.hpp
	include/Engine/RendererTransformCache.hpp

	include/Engine/Systems/GeometryCache.hpp
	include/Engine/Systems/TextureCache.hpp
	include/Engine/Systems/EmptyRenderer.hpp
	include/Engine/Systems/DefaultRenderer.hpp
	include/Engine/Systems/System.hpp
	include/Engine/Systems/ImGuiSystem.hpp
	include/Engine/Systems/SimpleFPSCameraSystem.hpp

	include/Engine/Components/Transform.hpp

    include/Engine/Resources/EmbeddedFileLoader.hpp
    include/Engine/Resources/Geometry.hpp
    include/Engine/Resources/Resource.hpp
    include/Engine/Resources/ShaderPreprocessor.hpp
	include/Engine/Resources/Shader.hpp
    include/Engine/Resources/Texture.hpp
	include/Engine/Resources/ImageCopy.hpp
	include/Engine/Resources/ResourceData.hpp
	include/Engine/Resources/ResourceSerialization.hpp
	include/Engine/Resources/RawSampler.hpp
	include/Engine/Resources/TextureIterator.hpp

	include/Engine/Raytrace/DefaultRaytracer.hpp
	include/Engine/Raytrace/Raytracer.hpp
)

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