#include <Engine/Engine.hpp>
#include <Engine/Platform.hpp>
#include <Engine/DefaultRenderer.hpp>

#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cmath>

#include "PlatformDefinitions.h"
#include "Errors.hpp"
#include "StringTools.hpp"
#include "MapHelper.hpp"
#include "Image.h"
#include "FileWrapper.hpp"

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

#include "imgui.h"
#include "ImGuiImplDiligent.hpp"
#include "ImGuiUtils.hpp"

#if PLATFORM_LINUX
#if VULKAN_SUPPORTED
#    include "ImGuiImplLinuxXCB.hpp"
#endif
#include "ImGuiImplLinuxX11.hpp"
#endif

#if PLATFORM_WIN32
#include "ImGuiImplWin32.hpp"
#endif

using namespace Diligent;

namespace Morpheus
{
	IRenderer* IEngineComponent::ToRenderer() {
		return nullptr;
	}

	const IRenderer* IEngineComponent::ToRenderer() const {
		return nullptr;
	}

	Engine* Engine::mGlobalInstance = nullptr;

	Engine::Engine()
	{
		assert(mGlobalInstance == nullptr);
		mGlobalInstance = this;
	}

	void Engine::Startup(const EngineParams& params) {
		ProcessConfigParams(params);

		// Start up thread pool
		int threadCount = params.mThreads.mThreadCount;
		if (threadCount <= 0) {
			threadCount = std::thread::hardware_concurrency();
		}
		mThreadPool.Startup(threadCount);

		// Create platform
		mPlatform = CreatePlatform();
		mPlatform->Initialize(this, params);

		mResourceManager = new ResourceManager(this, &mThreadPool);

		for (auto component : mComponents) {
			component->Initialize(this);
		}
	}

	DG::float4x4 Engine::GetSurfacePretransformMatrix(const DG::float3& f3CameraViewAxis) const
	{
		const auto& SCDesc = mSwapChain->GetDesc();
		switch (SCDesc.PreTransform)
		{
			case  DG::SURFACE_TRANSFORM_ROTATE_90:
				// The image content is rotated 90 degrees clockwise.
				return DG::float4x4::RotationArbitrary(f3CameraViewAxis, -DG::PI_F / 2.f);

			case  DG::SURFACE_TRANSFORM_ROTATE_180:
				// The image content is rotated 180 degrees clockwise.
				return DG::float4x4::RotationArbitrary(f3CameraViewAxis, -DG::PI_F);

			case  DG::SURFACE_TRANSFORM_ROTATE_270:
				// The image content is rotated 270 degrees clockwise.
				return DG::float4x4::RotationArbitrary(f3CameraViewAxis, -DG::PI_F * 3.f / 2.f);

			case  DG::SURFACE_TRANSFORM_OPTIMAL:
				UNEXPECTED("SURFACE_TRANSFORM_OPTIMAL is only valid as parameter during swap chain initialization.");
				return DG::float4x4::Identity();

			case DG::SURFACE_TRANSFORM_HORIZONTAL_MIRROR:
			case DG::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90:
			case DG::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180:
			case DG::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270:
				UNEXPECTED("Mirror transforms are not supported");
				return DG::float4x4::Identity();

			default:
				return DG::float4x4::Identity();
		}
	}

	float4x4 Engine::GetAdjustedProjectionMatrix(float FOV, float NearPlane, float FarPlane) const
	{
		const auto& SCDesc = mSwapChain->GetDesc();

		float AspectRatio = static_cast<float>(SCDesc.Width) / static_cast<float>(SCDesc.Height);
		float XScale, YScale;
		if (SCDesc.PreTransform == SURFACE_TRANSFORM_ROTATE_90 ||
			SCDesc.PreTransform == SURFACE_TRANSFORM_ROTATE_270 ||
			SCDesc.PreTransform == SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90 ||
			SCDesc.PreTransform == SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270)
		{
			// When the screen is rotated, vertical FOV becomes horizontal FOV
			XScale = 1.f / std::tan(FOV / 2.f);
			// Aspect ratio is inversed
			YScale = XScale * AspectRatio;
		}
		else
		{
			YScale = 1.f / std::tan(FOV / 2.f);
			XScale = YScale / AspectRatio;
		}

		float4x4 Proj;
		Proj._11 = XScale;
		Proj._22 = YScale;
		Proj.SetNearFarClipPlanes(NearPlane, FarPlane, mDevice->GetDeviceCaps().IsGLDevice());
		return Proj;
	}

	DG::float4x4 Engine::GetAdjustedOrthoMatrix(
		const DG::float2& fCameraSize,
		float zNear, float zFar) const {
		float XScale = (float)(2.0 / fCameraSize.x);
		float YScale = (float)(2.0 / fCameraSize.y);

		float4x4 Proj;
		Proj._11 = XScale;
		Proj._22 = YScale;

		bool bIsGL = mDevice->GetDeviceCaps().IsGLDevice();

		if (bIsGL)
        {
            Proj._33 = -(-(zFar + zNear) / (zFar - zNear));
            Proj._43 = -2 * zNear * zFar / (zFar - zNear);
        }
        else
        {
            Proj._33 = zFar / (zFar - zNear);
            Proj._43 = -zNear * zFar / (zFar - zNear);
        }

		Proj._44 = 1;

		return Proj;
	}

	void Engine::CollectGarbage() {
		std::cout << "Collecting garbage..." << std::endl;
		if (mResourceManager)
			mResourceManager->CollectGarbage();
	}

	void Engine::Shutdown() {

		mThreadPool.Shutdown();

		for (auto component : mComponents) {
			delete component;
		}

		mComponents.clear();
		mRenderer = nullptr;

		mImGui.reset();

		if (mResourceManager) {
			delete mResourceManager;
			mResourceManager = nullptr;
		}

		for (auto context : mDeferredContexts)
			context->Release();

		if (mImmediateContext) {
			mImmediateContext->Flush();
		}

		mDeferredContexts.clear();

		if (mImmediateContext) {
			mImmediateContext->Release();
			mImmediateContext = nullptr;
		}

		if (mSwapChain) {
			mSwapChain->Release();
			mSwapChain = nullptr;
		}

		if (mDevice) {
			mDevice->Release();
			mDevice = nullptr;
		}

		if (mPlatform) {
			mPlatform->Shutdown();
			delete mPlatform;
			mPlatform = nullptr;
		}
	}

	Engine::~Engine()
	{
		Shutdown();
		mGlobalInstance = nullptr;
	}

	void Engine::GetEngineInitializationAttribs(RENDER_DEVICE_TYPE DeviceType, 
		EngineCreateInfo& EngineCI, 
		SwapChainDesc& SCDesc) {

		if (!bUseSRGBSwapChain) {
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
				// EngineVkCreateInfo& EngVkAttribs = static_cast<EngineVkCreateInfo&>(EngineCI);
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

	void Engine::InitializeDiligentEngine(const NativeWindow* pWindow)
	{
	#if PLATFORM_MACOS
		// We need at least 3 buffers on Metal to avoid massive
		// peformance degradation in full screen mode.
		// https://github.com/KhronosGroup/MoltenVK/issues/808
		mSwapChainInitDesc.BufferCount = 3;
	#endif

		std::vector<IDeviceContext*> ppContexts;
		switch (mDeviceType)
		{
	#if D3D11_SUPPORTED
		case RENDER_DEVICE_TYPE_D3D11:
		{
			EngineD3D11CreateInfo EngineCI;

		#ifdef DILIGENT_DEVELOPMENT
			EngineCI.DebugFlags |=
				D3D11_DEBUG_FLAG_CREATE_DEBUG_DEVICE |
				D3D11_DEBUG_FLAG_VERIFY_COMMITTED_SHADER_RESOURCES;
		#endif
		#ifdef DILIGENT_DEBUG
			EngineCI.DebugFlags |= D3D11_DEBUG_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE;
		#endif

			if (mValidationLevel >= 1)
			{
				EngineCI.DebugFlags =
					D3D11_DEBUG_FLAG_CREATE_DEBUG_DEVICE |
					D3D11_DEBUG_FLAG_VERIFY_COMMITTED_SHADER_RESOURCES |
					D3D11_DEBUG_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE;
			}
			else if (mValidationLevel == 0)
			{
				EngineCI.DebugFlags = D3D11_DEBUG_FLAG_NONE;
			}

			GetEngineInitializationAttribs(mDeviceType, EngineCI, mSwapChainInitDesc);

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
				pFactoryD3D11->CreateSwapChainD3D11(mDevice, ppContexts[0], mSwapChainInitDesc, FullScreenModeDesc{}, *pWindow, &mSwapChain);
		}
		break;
	#endif

	#if D3D12_SUPPORTED
		case RENDER_DEVICE_TYPE_D3D12:
		{
			EngineD3D12CreateInfo EngineCI;

		#ifdef DILIGENT_DEVELOPMENT
			EngineCI.EnableDebugLayer = true;
		#endif
			if (mValidationLevel >= 1)
			{
				EngineCI.EnableDebugLayer = true;
				if (mValidationLevel >= 2)
					EngineCI.EnableGPUBasedValidation = true;
			}
			else if (mValidationLevel == 0)
			{
				EngineCI.EnableDebugLayer = false;
			}

			GetEngineInitializationAttribs(mDeviceType, EngineCI, mSwapChainInitDesc);

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
				mDeviceType = RENDER_DEVICE_TYPE_D3D11;
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
				pFactoryD3D12->CreateSwapChainD3D12(mDevice, ppContexts[0], mSwapChainInitDesc, FullScreenModeDesc{}, *pWindow, &mSwapChain);
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

	#    ifdef DILIGENT_DEVELOPMENT
				EngineCI.CreateDebugContext = true;
	#    endif
				EngineCI.ForceNonSeparablePrograms = bForceNonSeprblProgs;

				if (mValidationLevel >= 1)
				{
					EngineCI.CreateDebugContext = true;
				}
				else if (mValidationLevel == 0)
				{
					EngineCI.CreateDebugContext = false;
				}

				GetEngineInitializationAttribs(mDeviceType, EngineCI, mSwapChainInitDesc);

				if (EngineCI.NumDeferredContexts != 0)
				{
					LOG_ERROR_MESSAGE("Deferred contexts are not supported in OpenGL mode");
					EngineCI.NumDeferredContexts = 0;
				}
				ppContexts.resize(1 + EngineCI.NumDeferredContexts);
				pFactoryOpenGL->CreateDeviceAndSwapChainGL(
					EngineCI, &mDevice, ppContexts.data(), mSwapChainInitDesc, &mSwapChain);
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
	#    ifdef DILIGENT_DEVELOPMENT
				EngVkAttribs.EnableValidation = true;
	#    endif
				if (mValidationLevel >= 1)
				{
					EngVkAttribs.EnableValidation = true;
				}
				else if (mValidationLevel == 0)
				{
					EngVkAttribs.EnableValidation = false;
				}

				GetEngineInitializationAttribs(mDeviceType, EngVkAttribs, mSwapChainInitDesc);
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
					pFactoryVk->CreateSwapChainVk(mDevice, ppContexts[0], mSwapChainInitDesc, *pWindow, &mSwapChain);
			}
			break;
	#endif


	#if METAL_SUPPORTED
			case RENDER_DEVICE_TYPE_METAL:
			{
				EngineMtlCreateInfo MtlAttribs;

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

		switch (mDeviceType)
		{
			// clang-format off
			case RENDER_DEVICE_TYPE_D3D11:  mAppTitle.append(" (D3D11)");    break;
			case RENDER_DEVICE_TYPE_D3D12:  mAppTitle.append(" (D3D12)");    break;
			case RENDER_DEVICE_TYPE_GL:     mAppTitle.append(" (OpenGL)");   break;
			case RENDER_DEVICE_TYPE_GLES:   mAppTitle.append(" (OpenGLES)"); break;
			case RENDER_DEVICE_TYPE_VULKAN: mAppTitle.append(" (Vulkan)");   break;
			case RENDER_DEVICE_TYPE_METAL:  mAppTitle.append(" (Metal)");    break;
			default: UNEXPECTED("Unknown/unsupported device type");
				// clang-format on
		}

		mImmediateContext = ppContexts[0];
		auto NumDeferredCtx = ppContexts.size() - 1;
		mDeferredContexts.resize(NumDeferredCtx);
		for (Uint32 ctx = 0; ctx < NumDeferredCtx; ++ctx)
			mDeferredContexts[ctx] = ppContexts[1 + ctx];
	}

	void Engine::UpdateAdaptersDialog()
	{
	#if PLATFORM_WIN32 || PLATFORM_LINUX
		const auto& SCDesc = mSwapChain->GetDesc();

		Uint32 AdaptersWndWidth = std::min(330u, SCDesc.Width);
		ImGui::SetNextWindowSize(ImVec2(static_cast<float>(AdaptersWndWidth), 0), ImGuiCond_Always);
		ImGui::SetNextWindowPos(ImVec2(static_cast<float>(std::max(SCDesc.Width - AdaptersWndWidth, 10U) - 10), 10), ImGuiCond_Always);
		ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Adapters", nullptr, ImGuiWindowFlags_NoResize))
		{
			if (mAdapterAttribs.Type != ADAPTER_TYPE_UNKNOWN)
			{
				ImGui::TextDisabled("Adapter: %s (%d MB)", mAdapterAttribs.Description, static_cast<int>(mAdapterAttribs.DeviceLocalMemory >> 20));
			}

			if (!mDisplayModes.empty())
			{
				std::vector<const char*> DisplayModes(mDisplayModes.size());
				std::vector<std::string> DisplayModeStrings(mDisplayModes.size());
				for (int i = 0; i < static_cast<int>(mDisplayModes.size()); ++i)
				{
					static constexpr const char* ScalingModeStr[] =
						{
							""
							" Centered",
							" Stretched" //
						};
					const auto& Mode = mDisplayModes[i];

					std::stringstream ss;

					float RefreshRate = static_cast<float>(Mode.RefreshRateNumerator) / static_cast<float>(Mode.RefreshRateDenominator);
					ss << Mode.Width << "x" << Mode.Height << "@" << std::fixed << std::setprecision(2) << RefreshRate << " Hz" << ScalingModeStr[static_cast<int>(Mode.Scaling)];
					DisplayModeStrings[i] = ss.str();
					DisplayModes[i]       = DisplayModeStrings[i].c_str();
				}

				ImGui::SetNextItemWidth(220);
				ImGui::Combo("Display Modes", &mSelectedDisplayMode, DisplayModes.data(), static_cast<int>(DisplayModes.size()));
			}

			if (bFullScreenMode)
			{
				if (ImGui::Button("Go Windowed"))
				{
					SetWindowedMode();
				}
			}
			else
			{
				if (!mDisplayModes.empty())
				{
					if (ImGui::Button("Go Full Screen"))
					{
						const auto& SelectedMode = mDisplayModes[mSelectedDisplayMode];
						SetFullscreenMode(SelectedMode);
					}
				}
			}

			ImGui::Checkbox("VSync", &bVSync);

			if (mDevice->GetDeviceCaps().IsD3DDevice())
			{
				// clang-format off
				std::pair<Uint32, const char*> FrameLatencies[] = 
				{
					{1, "1"},
					{2, "2"},
					{3, "3"},
					{4, "4"},
					{5, "5"},
					{6, "6"},
					{7, "7"},
					{8, "8"},
					{9, "9"},
					{10, "10"}
				};
				// clang-format on

				if (SCDesc.BufferCount <= _countof(FrameLatencies) && mMaxFrameLatency <= _countof(FrameLatencies))
				{
					ImGui::SetNextItemWidth(120);
					auto NumFrameLatencyItems = std::max(std::max(mMaxFrameLatency, SCDesc.BufferCount), Uint32{4});
					if (ImGui::Combo("Max frame latency", &mMaxFrameLatency, FrameLatencies, NumFrameLatencyItems))
					{
						mSwapChain->SetMaximumFrameLatency(mMaxFrameLatency);
					}
				}
				else
				{
					// 10+ buffer swap chain or frame latency? Something is not quite right
				}
			}
		}
		ImGui::End();
	#endif
	}


	std::string GetArgument(const char*& pos, const char* ArgName)
	{
		size_t      ArgNameLen = 0;
		const char* delimeters = " \n\r";
		while (pos[ArgNameLen] != 0 && strchr(delimeters, pos[ArgNameLen]) == nullptr)
			++ArgNameLen;

		if (StrCmpNoCase(pos, ArgName, ArgNameLen) == 0)
		{
			pos += ArgNameLen;
			while (*pos != 0 && strchr(delimeters, *pos) != nullptr)
				++pos;
			std::string Arg;
			while (*pos != 0 && strchr(delimeters, *pos) == nullptr)
				Arg.push_back(*(pos++));
			return Arg;
		}
		else
		{
			return std::string{};
		}
	}

	// Command line example to capture frames:
	//
	//     -mode d3d11 -adapters_dialog 0 -capture_path . -capture_fps 15 -capture_name frame -width 640 -height 480 -capture_format jpg -capture_quality 100 -capture_frames 3 -capture_alpha 0
	//
	// Image magick command to create animated gif:
	//
	//     magick convert  -delay 6  -loop 0 -layers Optimize -compress LZW -strip -resize 240x180   frame*.png   Animation.gif
	//
	void Engine::ProcessConfigParams(const EngineParams& params)
	{
		mDeviceType = 			params.mRenderer.mBackendType;
		mValidationLevel = 		params.mRenderer.mValidationLevel;
		mInitialWindowWidth = 	params.mDisplay.mWidth;
		mInitialWindowHeight = 	params.mDisplay.mHeight;
		bVSync = 				params.mDisplay.bVSync;
		bFullScreenMode	=		params.mDisplay.bFullscreen;

		switch (mDeviceType) {
		#if D3D11_SUPPORTED
			case DG::RENDER_DEVICE_TYPE_D3D11:
				break;
		#endif
		#if D3D12_SUPPORTED
			case DG::RENDER_DEVICE_TYPE_D3D12:
				break;
		#endif
		#if GL_SUPPORTED
			case DG::RENDER_DEVICE_TYPE_GL:
				break;
		#endif
		#if GLES_SUPPORTED
			case DG::RENDER_DEVICE_TYPE_GLES:
				break;
		#endif
		#if METAL_SUPPORTED
			case DG::RENDER_DEVICE_TYPE_METAL:
				break;
		#endif
		#if VULKAN_SUPPORTED
			case DG::RENDER_DEVICE_TYPE_VULKAN:
				break;
		#endif

			case DG::RENDER_DEVICE_TYPE_UNDEFINED:
		#if D3D12_SUPPORTED
				mDeviceType = RENDER_DEVICE_TYPE_D3D12;
		#elif VULKAN_SUPPORTED
				mDeviceType = RENDER_DEVICE_TYPE_VULKAN;
		#elif D3D11_SUPPORTED
				mDeviceType = RENDER_DEVICE_TYPE_D3D11;
		#elif GL_SUPPORTED || GLES_SUPPORTED
				mDeviceType = RENDER_DEVICE_TYPE_GL;
		#endif
				break;

			default:
				throw std::runtime_error("Unsupported device type!");
				break;
		}
	}

	void Engine::OnPreWindowResized() {

	}
	
	void Engine::OnWindowResized(uint width, uint height) {
		if (mRenderer) {
			mRenderer->OnWindowResized(width, height);
		}
	}

	void Engine::WindowResize(int width, int height)
	{
		if (mSwapChain)
		{
			OnPreWindowResized();
			mSwapChain->Resize(width, height);
			auto SCWidth  = mSwapChain->GetDesc().Width;
			auto SCHeight = mSwapChain->GetDesc().Height;
			OnWindowResized(SCWidth, SCHeight);
		}
	}

	void Engine::Update(Scene* activeScene) {
		update_callback_t updater = [activeScene](double CurrTime, double ElapsedTime) {
			activeScene->Update(CurrTime, ElapsedTime);
		};
		Update(updater);
	}

	void Engine::Update(const update_callback_t& callback) {
		update_callback_t callbackCopy = callback;
		update_callback_t updater = [this, callbackCopy](double CurrTime, double ElapsedTime) {
			Update(CurrTime, ElapsedTime);
			callbackCopy(CurrTime, ElapsedTime);
		};

		mPlatform->MessageLoop(updater);
	}

	void Engine::Update(double CurrTime, double ElapsedTime) {
		mCurrentTime = CurrTime;

		if (mImGui) {
			const auto& SCDesc = mSwapChain->GetDesc();
			mImGui->NewFrame(SCDesc.Width, SCDesc.Height, SCDesc.PreTransform);
			if (bShowAdaptersDialog) {
				UpdateAdaptersDialog();
			}
		}
		
		if (mDevice) {
			mInputController.ClearState();
		}
	}

	void Engine::Render(Scene* activeScene) {
		EntityNode camera = EntityNode::Invalid();

		if (activeScene) {
			camera = activeScene->GetCameraNode();
		}

		if (mRenderer) {
			mRenderer->Render(activeScene, camera);
		} else {
			throw std::runtime_error("Engine does not have renderer!");
		}
	}

	void Engine::RenderUI() {
		auto imGui = GetUI();

		if (imGui)
		{
			if (GetShowUI())
			{
				// No need to call EndFrame as ImGui::Render calls it automatically
				imGui->Render(mImmediateContext);
			}
			else
			{
				imGui->EndFrame();
			}
		}
	}

	void Engine::Present()
	{
		if (!mSwapChain)
			return;

		mSwapChain->Present(bVSync ? 1 : 0);

		mInputController.NewFrame();
	}

	void Engine::InitializeDefaultSystems(Scene* scene) {
		std::cout << "Initializing default systems for new scene..." << std::endl;

		for (auto& component : mComponents) {
			component->InitializeSystems(scene);
		}

		scene->bInitializedByEngine = true;
	}


#if PLATFORM_WIN32
	void Engine::OnWindowCreated(HWND hWnd, LONG WindowWidth, LONG WindowHeight) {
        try
        {
            Win32NativeWindow Window{hWnd};
            InitializeDiligentEngine(&Window);

            // Initialize Dear ImGUI
            const auto& SCDesc = mSwapChain->GetDesc();
            mImGui.reset(new ImGuiImplWin32(hWnd, mDevice, SCDesc.ColorBufferFormat, SCDesc.DepthBufferFormat));
        }
        catch (...)
        {
            LOG_ERROR("Failed to initialize Diligent Engine.");
        }
	}

	LRESULT Engine::HandleWin32Message(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        if (mImGui)
        {
            auto Handled = static_cast<DG::ImGuiImplWin32*>(mImGui.get())->Win32_ProcHandler(hWnd, message, wParam, lParam);
            if (Handled)
                return Handled;
        }

        struct WindowsMessageData
        {
            HWND   hWnd;
            UINT   message;
            WPARAM wParam;
            LPARAM lParam;
        } MsgData = {hWnd, message, wParam, lParam};
        
		return mInputController.HandleNativeMessage(&MsgData);
	}
#endif


// Platform specific stuff
#if PLATFORM_LINUX
	bool Engine::OnGLContextCreated(Display* display, Window window) 
	{
		try
		{
			LinuxNativeWindow LinuxWindow;
			LinuxWindow.pDisplay = display;
			LinuxWindow.WindowId = window;
			InitializeDiligentEngine(&LinuxWindow);
			const auto& SCDesc = mSwapChain->GetDesc();
			mImGui.reset(new ImGuiImplLinuxX11(mDevice, SCDesc.ColorBufferFormat, SCDesc.DepthBufferFormat, SCDesc.Width, SCDesc.Height));
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	int Engine::HandleXEvent(XEvent* xev)
	{
		auto handled = static_cast<ImGuiImplLinuxX11*>(mImGui.get())->HandleXEvent(xev);
		// Always handle mouse move, button release and key release events
		if (!handled || xev->type == ButtonRelease || xev->type == MotionNotify || xev->type == KeyRelease)
		{
			handled = mInputController.HandleXEvent(xev);
		}
		return handled;
	}

#if VULKAN_SUPPORTED
	bool Engine::InitVulkan(xcb_connection_t* connection, uint32_t window)
	{
		try
		{
			mDeviceType = RENDER_DEVICE_TYPE_VULKAN;
			LinuxNativeWindow LinuxWindow;
			LinuxWindow.WindowId       = window;
			LinuxWindow.pXCBConnection = connection;
			InitializeDiligentEngine(&LinuxWindow);
			const auto& SCDesc = mSwapChain->GetDesc();
			mImGui.reset(new ImGuiImplLinuxXCB(connection, mDevice, SCDesc.ColorBufferFormat, SCDesc.DepthBufferFormat, SCDesc.Width, SCDesc.Height));
			mInputController.InitXCBKeysms(connection);
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
	void Engine::HandleXCBEvent(xcb_generic_event_t* event) 
	{
		auto handled   = static_cast<ImGuiImplLinuxXCB*>(mImGui.get())->HandleXCBEvent(event);
		auto EventType = event->response_type & 0x7f;
		// Always handle mouse move, button release and key release events
		if (!handled || EventType == XCB_MOTION_NOTIFY || EventType == XCB_BUTTON_RELEASE || EventType == XCB_KEY_RELEASE)
		{
			handled = mInputController.HandleXCBEvent(event);
		}
	}
#endif
#endif
}
