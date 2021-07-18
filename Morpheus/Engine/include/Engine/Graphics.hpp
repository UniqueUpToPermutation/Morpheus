#pragma once

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"

#include <Engine/Platform.hpp>


namespace Morpheus {
	
	class IExternalGraphicsDevice {
	public:
		virtual ExtObjectId CreateTexture(const Texture& raw) = 0;
		virtual ExtObjectId CreateGeometry(const Geometry& raw) = 0;

		virtual void DestroyTexture(ExtObjectId id) = 0;
		virtual void DestroyGeometry(ExtObjectId id) = 0;
	};

	struct GraphicsDevice {
		IExternalGraphicsDevice* mExternal = nullptr;
		
		inline GraphicsDevice(IExternalGraphicsDevice* rt) :
			mExternal(rt) {
		}

		inline operator IExternalGraphicsDevice*() {
			return mExternal;
		}

		DG::IRenderDevice* mGpuDevice = nullptr;

		inline GraphicsDevice(DG::IRenderDevice* gpu) :
			mGpuDevice(gpu) {
		}

		inline operator DG::IRenderDevice*() {
			return mGpuDevice;
		}
	};

	template <ExtObjectType T>
	struct ExternalAspect {
		IExternalGraphicsDevice* mDevice = nullptr;
		ExtObjectId mId = NullExtObjectId;

		inline ExternalAspect() {
		}
		inline ExternalAspect(IExternalGraphicsDevice* device, ExtObjectId id) :
			mDevice(device),
			mId(id) {	
		}
		inline ~ExternalAspect() {
			if (mId != NullExtObjectId) {
				if constexpr (T == ExtObjectType::GEOMETRY) {
					mDevice->DestroyGeometry(mId);
				} else if constexpr (T == ExtObjectType::TEXTURE) {
					mDevice->DestroyTexture(mId);
				}
			}
		}

		ExternalAspect(const ExternalAspect&) = delete;
		ExternalAspect& operator=(const ExternalAspect&) = delete;

		inline ExternalAspect(ExternalAspect&& other) {
			std::swap(mDevice, other.mDevice);
			std::swap(mId, other.mId);
		}
		ExternalAspect& operator=(ExternalAspect&& other) {
			std::swap(mDevice, other.mDevice);
			std::swap(mId, other.mId);
			return *this;
		}
	};

	struct GraphicsParams {
		DG::Uint32      	mAdapterId   			= 0;
		bool         		bVSync               	= false;
		bool         		bUseSRGBSwapChain		= false;
		DG::Uint32   		mMaxFrameLatency      	= DG::SwapChainDesc{}.BufferCount;
		bool 				bRendererWarningGiven 	= false;
		int         		mValidationLevel     	= -1;
		bool				bForceNonSeprblProgs	= true;
	
		// We will need this when we have to recreate the swap chain (on Android)
		DG::SwapChainDesc mSwapChainInitDesc;
	};

	template <typename Info>
	using request_graphics_config_t = std::function<void(Info*)>;

	template <typename Info>
	void DefaultCapabilityRequest(Info* info) {
	}

	struct GraphicsCapabilityConfig {
		request_graphics_config_t<DG::EngineD3D12CreateInfo> mD3D12 = 
			&DefaultCapabilityRequest<DG::EngineD3D12CreateInfo>;
		request_graphics_config_t<DG::EngineD3D11CreateInfo> mD3D11 =
			&DefaultCapabilityRequest<DG::EngineD3D11CreateInfo>;
		request_graphics_config_t<DG::EngineGLCreateInfo> mGL = 
			&DefaultCapabilityRequest<DG::EngineGLCreateInfo>;
		request_graphics_config_t<DG::EngineVkCreateInfo> mVK = 
			&DefaultCapabilityRequest<DG::EngineVkCreateInfo>;
		request_graphics_config_t<DG::EngineMtlCreateInfo> mMTL = 
			&DefaultCapabilityRequest<DG::EngineMtlCreateInfo>;
	};

	class RealtimeGraphics {
	private:
		IPlatform*			mPlatform = nullptr;
		IRenderer*			mRenderer = nullptr;

		GraphicsParams 		mParams;

		DG::IEngineFactory*           		mEngineFactory 		= nullptr;
		DG::IRenderDevice*              	mDevice 			= nullptr;
		DG::IDeviceContext*              	mImmediateContext 	= nullptr;
		std::vector<DG::IDeviceContext*> 	mDeferredContexts;
		DG::ISwapChain*                  	mSwapChain 			= nullptr;
		std::vector<DG::DisplayModeAttribs>	mDisplayModes;

		DG::GraphicsAdapterInfo            	mAdapterAttribs;
		DG::ADAPTER_TYPE 					mAdapterType 		= DG::ADAPTER_TYPE_UNKNOWN;
		std::string  						mAdapterDetailsString;
		bool 								bInitialized = false;
		int          						mSelectedDisplayMode  	= 0;

		user_window_resize_t				mUserResizeDelegate;

	void OnUserResize(uint width, uint height);
	void GetEngineInitializationAttribs(DG::RENDER_DEVICE_TYPE DeviceType, 
		DG::EngineCreateInfo& EngineCI, DG::SwapChainDesc& SCDesc);

	public:
		inline RealtimeGraphics(IPlatform* platform) : mPlatform(platform) {
		}

		inline ~RealtimeGraphics() {
			if (bInitialized) {
				Shutdown();
			}
		}

		inline void Present(DG::Uint32 syncInterval) {
			mSwapChain->Present(syncInterval);
		}

		void Startup(const GraphicsParams& parameters = GraphicsParams(), 
			const GraphicsCapabilityConfig& capabilities = GraphicsCapabilityConfig());
		void Shutdown();

		inline const GraphicsParams& Parameters() const {
			return mParams;
		}

		inline DG::IRenderDevice* Device() const {
			return mDevice;
		}

		inline bool IsGL() const {
			return mDevice->GetDeviceCaps().IsGLDevice();
		}

		inline bool IsVulkan() const {
			return mDevice->GetDeviceCaps().IsVulkanDevice();
		}

		inline bool IsD3D() const {
			return mDevice->GetDeviceCaps().IsD3DDevice();
		}

		inline bool IsMetal() const {
			return mDevice->GetDeviceCaps().IsMetalDevice();
		}

		inline IPlatform* GetPlatform() const {
			return mPlatform;
		}

		inline DG::IDeviceContext* ImmediateContext() const {
			return mImmediateContext;
		}

		inline const std::vector<DG::IDeviceContext*>& DeferredContexts() const {
			return mDeferredContexts;
		}

		inline DG::ISwapChain* SwapChain() const {
			return mSwapChain;
		}

		inline const std::vector<DG::DisplayModeAttribs>& DisplayModes() const {
			return mDisplayModes;
		}

		inline const DG::GraphicsAdapterInfo& AdapterAttribsInfo() const {
			return mAdapterAttribs;
		}

		inline DG::ADAPTER_TYPE AdapterType() const {
			return mAdapterType;
		}

		inline uint CurrentDisplayMode() const {
			return mSelectedDisplayMode;
		}

		inline std::string AdapterDetails() {
			return mAdapterDetailsString;
		}
	};
}