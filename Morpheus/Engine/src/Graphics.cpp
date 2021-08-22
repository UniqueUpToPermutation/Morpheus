#include <Engine/Graphics.hpp>

#if PLATFORM_WIN32
#    define GLFW_EXPOSE_NATIVE_WIN32 1
#endif

#if PLATFORM_LINUX
#    define GLFW_EXPOSE_NATIVE_X11 1
#endif

#if PLATFORM_MACOS
#    define GLFW_EXPOSE_NATIVE_COCOA 1
#endif

#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <Engine/PlatformGLFW.hpp>
#endif

#if D3D11_SUPPORTED
#    include "EngineFactoryD3D11.h"
#endif
#if D3D12_SUPPORTED
#    include "EngineFactoryD3D12.h"
#endif
#if GL_SUPPORTED
#    include "EngineFactoryOpenGL.h"
#endif
#if VULKAN_SUPPORTED
#    include "EngineFactoryVk.h"
#endif
#if METAL_SUPPORTED
#    include "EngineFactoryMtl.h"
#endif


using namespace DG;

namespace Morpheus {
	void RealtimeGraphics::Startup(const GraphicsParams& parameters, const GraphicsCapabilityConfig& capabilities) {

	#ifdef USE_GLFW
		auto glfwPlatform = mPlatform->ToGLFW();
		GLFWwindow* window = nullptr;
		DG::RENDER_DEVICE_TYPE DevType = DG::RENDER_DEVICE_TYPE_UNDEFINED;

		if (glfwPlatform) {
			window = glfwPlatform->GetWindow();
			DevType = glfwPlatform->GetParams().mDeviceType;
		}
	#endif

		if (DevType == DG::RENDER_DEVICE_TYPE_UNDEFINED) {
#ifdef GL_SUPPORTED
			DevType = DG::RENDER_DEVICE_TYPE_GL;
#endif
#if D3D11_SUPPORTED
			DevType = DG::RENDER_DEVICE_TYPE_D3D11;
#endif
#if VULKAN_SUPPORTED
			DevType = DG::RENDER_DEVICE_TYPE_VULKAN;
#endif
#if D3D12_SUPPORTED
			DevType = DG::RENDER_DEVICE_TYPE_D3D12;
#endif
#if METAL_SUPPORTED
			DevType = DG::RENDER_DEVICE_TYPE_METAL;
#endif
		}


#ifdef USE_GLFW
	#if PLATFORM_WIN32
		Win32NativeWindow Window{glfwGetWin32Window(window)};
	#endif
	#if PLATFORM_LINUX
		LinuxNativeWindow Window;
		Window.WindowId = glfwGetX11Window(window);
		Window.pDisplay = glfwGetX11Display();
		if (DevType == RENDER_DEVICE_TYPE_GL)
			glfwMakeContextCurrent(window);
	#endif
	#if PLATFORM_MACOS
		MacOSNativeWindow Window;
		if (DevType == RENDER_DEVICE_TYPE_GL)
			glfwMakeContextCurrent(window);
		else
			Window.pNSView = GetNSWindowView(window);
	#endif

#endif

		SwapChainDesc SCDesc;
		switch (DevType)
		{

	#if D3D11_SUPPORTED
			case RENDER_DEVICE_TYPE_D3D11:
			{
	#    if ENGINE_DLL
				// Load the dll and import GetEngineFactoryD3D11() function
				auto* GetEngineFactoryD3D11 = LoadGraphicsEngineD3D11();
	#    endif
				auto* pFactoryD3D11 = GetEngineFactoryD3D11();

				EngineD3D11CreateInfo EngineCI;
				GetEngineInitializationAttribs(DevType, EngineCI, SCDesc);
				pFactoryD3D11->CreateDeviceAndContextsD3D11(EngineCI, &m_pDevice, &m_pImmediateContext);
				pFactoryD3D11->CreateSwapChainD3D11(m_pDevice, m_pImmediateContext, SCDesc, FullScreenModeDesc{}, Window, &m_pSwapChain);
			}
			break;
	#endif // D3D11_SUPPORTED


	#if D3D12_SUPPORTED
			case RENDER_DEVICE_TYPE_D3D12:
			{
	#    if ENGINE_DLL
				// Load the dll and import GetEngineFactoryD3D12() function
				auto* GetEngineFactoryD3D12 = LoadGraphicsEngineD3D12();
	#    endif
				auto* pFactoryD3D12 = GetEngineFactoryD3D12();

				EngineD3D12CreateInfo EngineCI;
				GetEngineInitializationAttribs(DevType, EngineCI, SCDesc);
				pFactoryD3D12->CreateDeviceAndContextsD3D12(EngineCI, &mDevice, &mImmediateContext);
				pFactoryD3D12->CreateSwapChainD3D12(m_pDevice, mImmediateContext, SCDesc, FullScreenModeDesc{}, Window, &mSwapChain);
			}
			break;
	#endif // D3D12_SUPPORTED


	#if GL_SUPPORTED
			case RENDER_DEVICE_TYPE_GL:
			{
	#    if EXPLICITLY_LOAD_ENGINE_GL_DLL
				// Load the dll and import GetEngineFactoryOpenGL() function
				auto GetEngineFactoryOpenGL = LoadGraphicsEngineOpenGL();
	#    endif
				auto* pFactoryOpenGL = GetEngineFactoryOpenGL();

				EngineGLCreateInfo EngineCI;
				GetEngineInitializationAttribs(DevType, EngineCI, SCDesc);
				EngineCI.Window = Window;
				pFactoryOpenGL->CreateDeviceAndSwapChainGL(EngineCI, &mDevice, &mImmediateContext, SCDesc, &mSwapChain);
			}
			break;
	#endif // GL_SUPPORTED


	#if VULKAN_SUPPORTED
			case RENDER_DEVICE_TYPE_VULKAN:
			{
	#    if EXPLICITLY_LOAD_ENGINE_VK_DLL
				// Load the dll and import GetEngineFactoryVk() function
				auto* GetEngineFactoryVk = LoadGraphicsEngineVk();
	#    endif
				auto* pFactoryVk = GetEngineFactoryVk();

				EngineVkCreateInfo EngineCI;
				GetEngineInitializationAttribs(DevType, EngineCI, SCDesc);
				pFactoryVk->CreateDeviceAndContextsVk(EngineCI, &mDevice, &mImmediateContext);
				pFactoryVk->CreateSwapChainVk(mDevice, mImmediateContext, SCDesc, Window, &mSwapChain);
			}
			break;
	#endif // VULKAN_SUPPORTED

	#if METAL_SUPPORTED
			case RENDER_DEVICE_TYPE_METAL:
			{
				auto* pFactoryMtl = GetEngineFactoryMtl();

				EngineMtlCreateInfo EngineCI;
				GetEngineInitializationAttribs(DevType, EngineCI, SCDesc);
				pFactoryMtl->CreateDeviceAndContextsMtl(EngineCI, &mDevice, &mImmediateContext);
				pFactoryMtl->CreateSwapChainMtl(mDevice, mImmediateContext, SCDesc, Window, &mSwapChain);
			}
			break;
	#endif // METAL_SUPPORTED

			default:
				std::cerr << "Unknown/unsupported device type";
				throw std::runtime_error("Unknown/unsupported device type");
				break;
		}

		if (mDevice == nullptr || mImmediateContext == nullptr || mSwapChain == nullptr)
			throw std::runtime_error("Failed to create device!");

		mPlatform->AddUserResizeHandler(&mUserResizeDelegate);
	}

	void RealtimeGraphics::OnUserResize(uint width, uint height) {
		mSwapChain->Resize(width, height);
	}

	void RealtimeGraphics::GetEngineInitializationAttribs(DG::RENDER_DEVICE_TYPE DeviceType, 
		DG::EngineCreateInfo& EngineCI, DG::SwapChainDesc& SCDesc) {

		SCDesc.ColorBufferFormat = DG::TEX_FORMAT_RGBA8_UNORM;
			
		switch (DeviceType)
 	   {
#if VULKAN_SUPPORTED
			case RENDER_DEVICE_TYPE_VULKAN:
			{
				EngineCI.Features.GeometryShaders   = DEVICE_FEATURE_STATE_ENABLED;
				EngineCI.Features.Tessellation 		= DEVICE_FEATURE_STATE_ENABLED;
			}
			break;
#endif

#if GL_SUPPORTED
			case RENDER_DEVICE_TYPE_GL:
			{
				EngineCI.Features.GeometryShaders   = DEVICE_FEATURE_STATE_ENABLED;
				EngineCI.Features.Tessellation 		= DEVICE_FEATURE_STATE_ENABLED;
			}
			break;
#endif

		default:
			LOG_ERROR_AND_THROW("Unknown device type");
			break;
		}
	}

	void RealtimeGraphics::Shutdown() {

		mPlatform->RemoveUserResizeHandler(&mUserResizeDelegate);

		if (mSwapChain) {
			mSwapChain->Release();
			mSwapChain = nullptr;
		}

		for (auto context : mDeferredContexts) {
			context->Release();
		}
		
		mDeferredContexts.clear();

		if (mImmediateContext) {
			mImmediateContext->Release();
			mImmediateContext = nullptr; 
		}

		if (mDevice) {
			mDevice->Release();
			mDevice = nullptr;
		}

		bInitialized = false;
	}

	void Context::Flush() {
		switch (mType) {
			case ContextType::GPU:
			mUnderlying.mGpuContext->Flush();
			break;
		}
	}
}