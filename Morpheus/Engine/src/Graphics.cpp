#include <Engine/Graphics.hpp>

#if PLATFORM_LINUX
#include <Engine/Linux/PlatformLinux.hpp>
#endif

#if PLATFORM_WIN32
#include <Engine/Win32/PlatformWin32.hpp>
#endif

#if D3D11_SUPPORTED
#    include "EngineFactoryD3D11.h"
#endif

#if D3D12_SUPPORTED
#    include "EngineFactoryD3D12.h"
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
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

		mParams = parameters;
		auto platformParams = mPlatform->GetParameters();

		if (mParams.mSwapChainInitDesc.Width == 0) {
			mParams.mSwapChainInitDesc.Width = platformParams.mWindowWidth;
		}
		if (mParams.mSwapChainInitDesc.Height == 0) {
			mParams.mSwapChainInitDesc.Height = platformParams.mWindowHeight;
		}

	#if PLATFORM_LINUX
		auto linuxPlatform = mPlatform->ToLinux();
		auto window = linuxPlatform->GetNativeWindow();
		auto pWindow = &window;
	#endif

	#if PLATFORM_WIN32
		auto windowsPlatform = mPlatform->ToWindows();
		auto window = windowsPlatform->GetNativeWindow();
		auto pWindow = &window;
	#endif

	#if PLATFORM_MACOS
		// We need at least 3 buffers on Metal to avoid massive
		// peformance degradation in full screen mode.
		// https://github.com/KhronosGroup/MoltenVK/issues/808
		mParams.mSwapChainInitDesc.BufferCount = 3;
	#endif

		std::vector<IDeviceContext*> ppContexts;
		switch (platformParams.mDeviceType)
		{
	#if D3D11_SUPPORTED
		case RENDER_DEVICE_TYPE_D3D11:
		{
			EngineD3D11CreateInfo EngineCI;
			capabilities.mD3D11(&EngineCI);

		#ifdef DILIGENT_DEVELOPMENT
			EngineCI.DebugFlags |=
				D3D11_DEBUG_FLAG_CREATE_DEBUG_DEVICE |
				D3D11_DEBUG_FLAG_VERIFY_COMMITTED_SHADER_RESOURCES;
		#endif
		#ifdef DILIGENT_DEBUG
			EngineCI.DebugFlags |= D3D11_DEBUG_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE;
		#endif

			if (mParams.mValidationLevel >= 1)
			{
				EngineCI.DebugFlags =
					D3D11_DEBUG_FLAG_CREATE_DEBUG_DEVICE |
					D3D11_DEBUG_FLAG_VERIFY_COMMITTED_SHADER_RESOURCES |
					D3D11_DEBUG_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE;
			}
			else if (mParams.mValidationLevel == 0)
			{
				EngineCI.DebugFlags = D3D11_DEBUG_FLAG_NONE;
			}

			GetEngineInitializationAttribs(platformParams.mDeviceType, EngineCI, mParams.mSwapChainInitDesc);

		#if ENGINE_DLL
			// Load the dll and import GetEngineFactoryD3D11() function
			auto GetEngineFactoryD3D11 = LoadGraphicsEngineD3D11();
		#endif
			auto* pFactoryD3D11 = GetEngineFactoryD3D11();
			mEngineFactory = pFactoryD3D11;
			Uint32 NumAdapters = 0;
			pFactoryD3D11->EnumerateAdapters(EngineCI.MinimumFeatureLevel, NumAdapters, 0);
			std::vector<GraphicsAdapterInfo> Adapters(NumAdapters);
			if (NumAdapters > 0)
			{
				pFactoryD3D11->EnumerateAdapters(EngineCI.MinimumFeatureLevel, NumAdapters, Adapters.data());
			}
			else
			{
				LOG_ERROR_AND_THROW("Failed to find Direct3D11-compatible hardware adapters");
			}

			if (mAdapterType == ADAPTER_TYPE_SOFTWARE)
			{
				for (Uint32 i = 0; i < Adapters.size(); ++i)
				{
					if (Adapters[i].Type == mAdapterType)
					{
						mAdapterId = i;
						LOG_INFO_MESSAGE("Found software adapter '", Adapters[i].Description, "'");
						break;
					}
				}
			}

			mAdapterAttribs = Adapters[mAdapterId];
			if (mAdapterType != ADAPTER_TYPE_SOFTWARE)
			{
				Uint32 NumDisplayModes = 0;
				pFactoryD3D11->EnumerateDisplayModes(EngineCI.MinimumFeatureLevel, mAdapterId, 0, TEX_FORMAT_RGBA8_UNORM_SRGB, NumDisplayModes, nullptr);
				mDisplayModes.resize(NumDisplayModes);
				pFactoryD3D11->EnumerateDisplayModes(EngineCI.MinimumFeatureLevel, mAdapterId, 0, TEX_FORMAT_RGBA8_UNORM_SRGB, NumDisplayModes, mDisplayModes.data());
			}

			EngineCI.AdapterId = mAdapterId;
			ppContexts.resize(1 + EngineCI.NumDeferredContexts);
			pFactoryD3D11->CreateDeviceAndContextsD3D11(EngineCI, &mDevice, ppContexts.data());
			if (!mDevice)
			{
				LOG_ERROR_AND_THROW("Unable to initialize Diligent Engine in Direct3D11 mode. The API may not be available, "
					"or required features may not be supported by this GPU/driver/OS version.");
			}

			if (pWindow != nullptr)
				pFactoryD3D11->CreateSwapChainD3D11(mDevice, ppContexts[0], mParams.mSwapChainInitDesc, FullScreenModeDesc{}, *pWindow, &mSwapChain);
		}
		break;
	#endif

	#if D3D12_SUPPORTED
		case RENDER_DEVICE_TYPE_D3D12:
		{
			EngineD3D12CreateInfo EngineCI;
			capabilities.mD3D12(&EngineCI);

		#ifdef DILIGENT_DEVELOPMENT
			EngineCI.EnableDebugLayer = true;
		#endif
			if (mParams.mValidationLevel >= 1)
			{
				EngineCI.EnableDebugLayer = true;
				if (mParams.mValidationLevel >= 2)
					EngineCI.EnableGPUBasedValidation = true;
			}
			else if (mParams.mValidationLevel == 0)
			{
				EngineCI.EnableDebugLayer = false;
			}

			GetEngineInitializationAttribs(platformParams.mDeviceType, EngineCI, mParams.mSwapChainInitDesc);

		#if ENGINE_DLL
			// Load the dll and import GetEngineFactoryD3D12() function
			auto GetEngineFactoryD3D12 = LoadGraphicsEngineD3D12();
		#endif
			auto* pFactoryD3D12 = GetEngineFactoryD3D12();
			if (!pFactoryD3D12->LoadD3D12())
			{
				LOG_ERROR_AND_THROW("Failed to load Direct3D12");
			}

			mEngineFactory = pFactoryD3D12;
			Uint32 NumAdapters = 0;
			pFactoryD3D12->EnumerateAdapters(EngineCI.MinimumFeatureLevel, NumAdapters, 0);
			std::vector<GraphicsAdapterInfo> Adapters(NumAdapters);
			if (NumAdapters > 0)
			{
				pFactoryD3D12->EnumerateAdapters(EngineCI.MinimumFeatureLevel, NumAdapters, Adapters.data());
			}
			else
			{
		#if D3D11_SUPPORTED
				LOG_ERROR_MESSAGE("Failed to find Direct3D12-compatible hardware adapters. Attempting to initialize the engine in Direct3D11 mode.");
				platformParams.mDeviceType = RENDER_DEVICE_TYPE_D3D11;
				InitializeDiligentEngine(pWindow);
				return;
		#else
				LOG_ERROR_AND_THROW("Failed to find Direct3D12-compatible hardware adapters.");
		#endif
			}

			if (mAdapterType == ADAPTER_TYPE_SOFTWARE)
			{
				for (Uint32 i = 0; i < Adapters.size(); ++i)
				{
					if (Adapters[i].Type == mAdapterType)
					{
						mAdapterId = i;
						LOG_INFO_MESSAGE("Found software adapter '", Adapters[i].Description, "'");
						break;
					}
				}
			}

			mAdapterAttribs = Adapters[mAdapterId];
			if (mAdapterType != ADAPTER_TYPE_SOFTWARE)
			{
				Uint32 NumDisplayModes = 0;
				pFactoryD3D12->EnumerateDisplayModes(EngineCI.MinimumFeatureLevel, mAdapterId, 0, TEX_FORMAT_RGBA8_UNORM_SRGB, NumDisplayModes, nullptr);
				mDisplayModes.resize(NumDisplayModes);
				pFactoryD3D12->EnumerateDisplayModes(EngineCI.MinimumFeatureLevel, mAdapterId, 0, TEX_FORMAT_RGBA8_UNORM_SRGB, NumDisplayModes, mDisplayModes.data());
			}

			EngineCI.AdapterId = mAdapterId;
			ppContexts.resize(1 + EngineCI.NumDeferredContexts);
			pFactoryD3D12->CreateDeviceAndContextsD3D12(EngineCI, &mDevice, ppContexts.data());
			if (!mDevice)
			{
				LOG_ERROR_AND_THROW("Unable to initialize Diligent Engine in Direct3D12 mode. The API may not be available, "
					"or required features may not be supported by this GPU/driver/OS version.");
			}

			if (!mSwapChain && pWindow != nullptr)
				pFactoryD3D12->CreateSwapChainD3D12(mDevice, ppContexts[0], mParams.mSwapChainInitDesc, FullScreenModeDesc{}, *pWindow, &mSwapChain);
		}
		break;
	#endif

	#if GL_SUPPORTED || GLES_SUPPORTED
			case RENDER_DEVICE_TYPE_GL:
			case RENDER_DEVICE_TYPE_GLES:
			{
	#    if !PLATFORM_MACOS
				VERIFY_EXPR(pWindow != nullptr);
	#    endif
	#    if EXPLICITLY_LOAD_ENGINE_GL_DLL
				// Load the dll and import GetEngineFactoryOpenGL() function
				auto GetEngineFactoryOpenGL = LoadGraphicsEngineOpenGL();
	#    endif
				auto* pFactoryOpenGL = GetEngineFactoryOpenGL();
				mEngineFactory     = pFactoryOpenGL;
				EngineGLCreateInfo EngineCI;
				EngineCI.Window = *pWindow;
				capabilities.mGL(&EngineCI);

	#    ifdef DILIGENT_DEVELOPMENT
				EngineCI.CreateDebugContext = true;
	#    endif
				EngineCI.ForceNonSeparablePrograms = mParams.bForceNonSeprblProgs;

				if (mParams.mValidationLevel >= 1)
				{
					EngineCI.CreateDebugContext = true;
				}
				else if (mParams.mValidationLevel == 0)
				{
					EngineCI.CreateDebugContext = false;
				}

				GetEngineInitializationAttribs(platformParams.mDeviceType, EngineCI, mParams.mSwapChainInitDesc);

				if (EngineCI.NumDeferredContexts != 0)
				{
					LOG_ERROR_MESSAGE("Deferred contexts are not supported in OpenGL mode");
					EngineCI.NumDeferredContexts = 0;
				}
				ppContexts.resize(1 + EngineCI.NumDeferredContexts);
				pFactoryOpenGL->CreateDeviceAndSwapChainGL(
					EngineCI, &mDevice, ppContexts.data(), mParams.mSwapChainInitDesc, &mSwapChain);
				if (!mDevice)
				{
					LOG_ERROR_AND_THROW("Unable to initialize Diligent Engine in OpenGL mode. The API may not be available, "
										"or required features may not be supported by this GPU/driver/OS version.");
				}
			}
			break;
	#endif

	#if VULKAN_SUPPORTED
			case RENDER_DEVICE_TYPE_VULKAN:
			{
	#    if EXPLICITLY_LOAD_ENGINE_VK_DLL
				// Load the dll and import GetEngineFactoryVk() function
				auto GetEngineFactoryVk = LoadGraphicsEngineVk();
	#    endif
				EngineVkCreateInfo EngVkAttribs;
				capabilities.mVK(&EngVkAttribs);

	#    ifdef DILIGENT_DEVELOPMENT
				EngVkAttribs.EnableValidation = true;
	#    endif
				if (mParams.mValidationLevel >= 1)
				{
					EngVkAttribs.EnableValidation = true;
				}
				else if (mParams.mValidationLevel == 0)
				{
					EngVkAttribs.EnableValidation = false;
				}

				//GetEngineInitializationAttribs(platformParams.mDeviceType, EngVkAttribs, mParams.mSwapChainInitDesc);
				ppContexts.resize(1 + EngVkAttribs.NumDeferredContexts);
				auto* pFactoryVk = GetEngineFactoryVk();
				mEngineFactory = pFactoryVk;
				pFactoryVk->CreateDeviceAndContextsVk(EngVkAttribs, &mDevice, ppContexts.data());
				if (!mDevice)
				{
					LOG_ERROR_AND_THROW("Unable to initialize Diligent Engine in Vulkan mode. The API may not be available, "
										"or required features may not be supported by this GPU/driver/OS version.");
				}

				if (!mSwapChain && pWindow != nullptr)
					pFactoryVk->CreateSwapChainVk(mDevice, ppContexts[0], mParams.mSwapChainInitDesc, *pWindow, &mSwapChain);
			}
			break;
	#endif


	#if METAL_SUPPORTED
			case RENDER_DEVICE_TYPE_METAL:
			{
				EngineMtlCreateInfo MtlAttribs;
				capabilities.mMTL(&MtlAttribs);

				m_TheSample->GetEngineInitializationAttribs(m_DeviceType, MtlAttribs, m_SwapChainInitDesc);
				ppContexts.resize(1 + MtlAttribs.NumDeferredContexts);
				auto* pFactoryMtl = GetEngineFactoryMtl();
				m_pEngineFactory  = pFactoryMtl;
				pFactoryMtl->CreateDeviceAndContextsMtl(MtlAttribs, &m_pDevice, ppContexts.data());

				if (!m_pSwapChain && pWindow != nullptr)
					pFactoryMtl->CreateSwapChainMtl(m_pDevice, ppContexts[0], m_SwapChainInitDesc, *pWindow, &m_pSwapChain);
			}
			break;
	#endif

			default:
				LOG_ERROR_AND_THROW("Unknown device type");
				break;
		}

		mImmediateContext = ppContexts[0];
		auto NumDeferredCtx = ppContexts.size() - 1;
		mDeferredContexts.resize(NumDeferredCtx);
		for (Uint32 ctx = 0; ctx < NumDeferredCtx; ++ctx)
			mDeferredContexts[ctx] = ppContexts[1 + ctx];

		bInitialized = true;

		mUserResizeDelegate = [this](uint width, uint height) {
			OnUserResize(width, height);
			return 0;
		};

		mPlatform->AddUserResizeHandler(&mUserResizeDelegate);
	}

	void RealtimeGraphics::OnUserResize(uint width, uint height) {
		mSwapChain->Resize(width, height);
	}

	void RealtimeGraphics::GetEngineInitializationAttribs(DG::RENDER_DEVICE_TYPE DeviceType, 
		DG::EngineCreateInfo& EngineCI, DG::SwapChainDesc& SCDesc) {
		
		if (!mParams.bUseSRGBSwapChain) {
			SCDesc.ColorBufferFormat = DG::TEX_FORMAT_RGBA8_UNORM;
		} else {
			SCDesc.ColorBufferFormat = DG::TEX_FORMAT_RGBA8_UNORM_SRGB;
		}
			
		switch (DeviceType)
 	   {
#if D3D11_SUPPORTED
			case RENDER_DEVICE_TYPE_D3D11:
			{
				//EngineD3D11CreateInfo& EngineD3D11CI = static_cast<EngineD3D11CreateInfo&>(EngineCI);
			}
			break;
#endif

#if D3D12_SUPPORTED
			case RENDER_DEVICE_TYPE_D3D12:
			{
				EngineD3D12CreateInfo& EngineD3D12CI                  = static_cast<EngineD3D12CreateInfo&>(EngineCI);
				EngineD3D12CI.GPUDescriptorHeapDynamicSize[0]         = 32768;
				EngineD3D12CI.GPUDescriptorHeapSize[1]                = 128;
				EngineD3D12CI.GPUDescriptorHeapDynamicSize[1]         = 2048 - 128;
				EngineD3D12CI.DynamicDescriptorAllocationChunkSize[0] = 32;
				EngineD3D12CI.DynamicDescriptorAllocationChunkSize[1] = 8; // D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
			}
			break;
#endif

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

#if GLES_SUPPORTED
			case RENDER_DEVICE_TYPE_GLES:
			{
				// Nothing to do
			}
			break;
#endif

#if METAL_SUPPORTED
			case RENDER_DEVICE_TYPE_METAL:
			{
				// Nothing to do
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
}