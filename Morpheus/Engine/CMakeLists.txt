cmake_minimum_required (VERSION 3.6)

project(Morpheus-Engine)

# Include the bullet physics engine
INCLUDE_DIRECTORIES(
${BULLET_PHYSICS_SOURCE_DIR}/src
)
LINK_LIBRARIES(
	BulletDynamics BulletCollision LinearMath
)

# Include package for embedding shaders in source code
add_subdirectory(shaders/cpp)
# Embed shaders
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
	src/Engine.cpp
	src/Brdf.cpp
	src/HdriToCubemap.cpp
	src/EditorCameraController.cpp
	src/PostProcessor.cpp
	src/Scene.cpp
	src/DefaultRenderer.cpp
	src/Camera.cpp
	src/ThreadPool.cpp
	src/ThreadTasks.cpp
	src/SpriteBatch.cpp
	src/Im3d.cpp
	src/Renderer.cpp
	src/RendererGlobalsData.cpp

	src/Engine2D/Renderer2D.cpp

	src/Resources/ShaderResource.cpp
	src/Resources/Resource.cpp
	src/Resources/ResourceManager.cpp
	src/Resources/PipelineResource.cpp
	src/Resources/TextureResource.cpp
	src/Resources/MaterialResource.cpp
	src/Resources/GeometryResource.cpp
	src/Resources/ShaderLoader.cpp
	src/Resources/EmbeddedFileLoader.cpp
	src/Resources/ResourceData.cpp
	src/Resources/ResourceSerialization.cpp
	src/Resources/RawTexture.cpp
	src/Resources/RawSampler.cpp
	src/Resources/RawGeometry.cpp
	src/Resources/TextureIterator.cpp

	src/Components/Transform.cpp
	src/Components/SkyboxComponent.cpp
	
	src/Pipelines/PipelineFactory.cpp
	src/Pipelines/BasicTextured.cpp
	src/Pipelines/Skybox.cpp
	src/Pipelines/StaticMeshPBR.cpp
	src/Pipelines/White.cpp
	src/Pipelines/ImageBasedLightingView.cpp

	src/Materials/MaterialPrototypes.cpp
	src/Materials/BasicTexturedMaterial.cpp
	src/Materials/WhiteMaterial.cpp
	src/Materials/StaticMeshPBRMaterial.cpp

	src/Systems/RendererBridge.cpp
	src/Systems/Physics.cpp
	src/Systems/TransformSystem.cpp
	src/Systems/ScriptSystem.cpp

	${CMAKE_SOURCE_DIR}/lodepng/lodepng.cpp
	${CMAKE_SOURCE_DIR}/im3d/im3d.cpp
	shader_rc.cpp
)

list(APPEND INCLUDE
    include/Engine/Brdf.hpp
    include/Engine/Camera.hpp
    include/Engine/Core.hpp
    include/Engine/DefaultRenderer.hpp
    include/Engine/Defines.hpp
    include/Engine/EditorCameraController.hpp
    include/Engine/Engine.hpp
    include/Engine/Entity.hpp
    include/Engine/Geometry.hpp
    include/Engine/Graph.hpp
    include/Engine/HdriToCubemap.hpp
    include/Engine/ImGuiObject.hpp
    include/Engine/InputController.hpp
    include/Engine/LightProbe.hpp
    include/Engine/Platform.hpp
    include/Engine/PostProcessor.hpp
    include/Engine/Renderer.hpp
    include/Engine/Scene.hpp
    include/Engine/SpriteBatch.hpp
    include/Engine/ThreadPool.hpp
    include/Engine/ThreadTasks.hpp
	include/Engine/Im3d.hpp
	include/Engine/DynamicGlobalsBuffer.hpp
	include/Engine/RendererGlobalsData.hpp

	include/Engine/Engine2D/Renderer2D.hpp
	include/Engine/Engine2D/Sprite.hpp
	include/Engine/Engine2D/Tilemap.hpp

    include/Engine/Materials/BasicTexturedMaterial.hpp
    include/Engine/Materials/MaterialPrototypes.hpp
    include/Engine/Materials/StaticMeshPBRMaterial.hpp
    include/Engine/Materials/WhiteMaterial.hpp

    include/Engine/Pipelines/PipelineFactory.hpp
	include/Engine/Pipelines/ImageBasedLightingView.hpp

    include/Engine/Resources/CollisionShapeResource.hpp
    include/Engine/Resources/EmbeddedFileLoader.hpp
    include/Engine/Resources/GeometryResource.hpp
    include/Engine/Resources/MaterialResource.hpp
    include/Engine/Resources/PipelineResource.hpp
    include/Engine/Resources/Resource.hpp
    include/Engine/Resources/ResourceManager.hpp
    include/Engine/Resources/ShaderLoader.hpp
    include/Engine/Resources/TextureResource.hpp
	include/Engine/Resources/ImageCopy.hpp
	include/Engine/Resources/ResourceData.hpp
	include/Engine/Resources/ResourceSerialization.hpp
	include/Engine/Resources/RawTexture.hpp
	include/Engine/Resources/RawSampler.hpp
	include/Engine/Resources/RawGeometry.hpp
	include/Engine/Resources/TextureIterator.hpp
    
    include/Engine/Systems/Physics.hpp
    include/Engine/Systems/RendererBridge.hpp
	include/Engine/Systems/ScriptSystem.hpp
	include/Engine/Systems/TransformSystem.hpp
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