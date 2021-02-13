#pragma once

#include <vector>
#include <string>
#include <memory>
#include <queue>

#include "BasicMath.hpp"
#include "NativeAppBase.hpp"
#include "RefCntAutoPtr.hpp"
#include "EngineFactory.h"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "ScreenCapture.hpp"
#include "Image.h"

#include <Engine/Platform.hpp>
#include <Engine/InputController.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Scene.hpp>
#include <Engine/Renderer.hpp>
#include <Engine/Entity.hpp>
#include <Engine/ThreadPool.hpp>

namespace Diligent
{
	class ImGuiImplDiligent;
}

namespace DG = Diligent;

namespace Morpheus {

	class IRenderer;
	class Scene;

	class Engine : public DG::NativeAppBase {
	public:
		Engine();
		~Engine();

		void ProcessCommandLine(const char* CmdLine) override final;
		const char* GetAppTitle() const override final { 
			return mAppTitle.c_str(); 
		}
		void Update();
		void Update(double CurrTime, double ElapsedTime) override;
		void WindowResize(int width, int height) override;
		void Render() override;
		void Present() override;

		void SelectDeviceType();
		void Startup(int argc, char** argv);
		void Shutdown();
		void CollectGarbage();
		
		void GetDesiredInitialWindowSize(int& width, int& height) override final
		{
			width  = mInitialWindowWidth;
			height = mInitialWindowHeight;
		}

		GoldenImageMode GetGoldenImageMode() const override final
		{
			return mGoldenImgMode;
		}

		int GetExitCode() const override final
		{
			return mExitCode;
		}

#if PLATFORM_LINUX
		bool OnGLContextCreated(Display* display, Window window) override final;
		int HandleXEvent(XEvent* xev) override final;

#if VULKAN_SUPPORTED
		bool InitVulkan(xcb_connection_t* connection, uint32_t window) override final;
		void HandleXCBEvent(xcb_generic_event_t* event) override final;
#endif
#endif

	private:
		void OnPreWindowResized();
		void OnWindowResized(uint width, uint height);

		void GetEngineInitializationAttribs(DG::RENDER_DEVICE_TYPE DeviceType, 
			DG::EngineCreateInfo& EngineCI, DG::SwapChainDesc& SCDesc);
		void InitializeDiligentEngine(const DG::NativeWindow* pWindow);
		void UpdateAdaptersDialog();

		virtual void SetFullscreenMode(const DG::DisplayModeAttribs& DisplayMode)
		{
			bFullScreenMode = true;
			mSwapChain->SetFullscreenMode(DisplayMode);
		}

		virtual void SetWindowedMode()
		{
			bFullScreenMode = false;
			mSwapChain->SetWindowedMode();
		}

		void CompareGoldenImage(const std::string& FileName, 
			DG::ScreenCapture::CaptureInfo& Capture);
		void SaveScreenCapture(const std::string& FileName, 
			DG::ScreenCapture::CaptureInfo& Capture);

		DG::RENDER_DEVICE_TYPE        		mDeviceType 		= DG::RENDER_DEVICE_TYPE_UNDEFINED;
		DG::IEngineFactory*           		mEngineFactory 		= nullptr;
		DG::IRenderDevice*              	mDevice 			= nullptr;
		DG::IDeviceContext*              	mImmediateContext 	= nullptr;
		std::vector<DG::IDeviceContext*> 	mDeferredContexts;
		DG::ISwapChain*                  	mSwapChain 			= nullptr;
		DG::GraphicsAdapterInfo            	mAdapterAttribs;
		std::vector<DG::DisplayModeAttribs>	mDisplayModes;

		InputController		mInputController;
		IPlatform*			mPlatform			= nullptr;
		ResourceManager* 	mResourceManager 	= nullptr;
		Scene* 				mScene 				= nullptr;
		IRenderer*			mRenderer 			= nullptr;
		ThreadPool			mThreadPool;

		int          mInitialWindowWidth  	= 0;
		int          mInitialWindowHeight 	= 0;
		int          mValidationLevel     	= -1;
		std::string  mAppTitle    			= "Morpheus";
		DG::Uint32       mAdapterId   		= 0;
		DG::ADAPTER_TYPE mAdapterType 		= DG::ADAPTER_TYPE_UNKNOWN;
		std::string  mAdapterDetailsString;
		int          mSelectedDisplayMode  	= 0;
		bool         bVSync               	= false;
		bool         bFullScreenMode      	= false;
		bool         bUseSRGBSwapChain		= false;
		bool         bShowAdaptersDialog  	= true;
		bool         bShowUI              	= true;
		bool         bForceNonSeprblProgs 	= true;
		bool 		 bValid 				= true;
		double       mCurrentTime          	= 0;
		DG::Uint32   mMaxFrameLatency      	= DG::SwapChainDesc{}.BufferCount;

		// We will need this when we have to recreate the swap chain (on Android)
		DG::SwapChainDesc mSwapChainInitDesc;

		struct ScreenCaptureInfo
		{
			bool              		AllowCapture 	= false;
			std::string       		Directory;
			std::string       		FileName        = "frame";
			double            		CaptureFPS      = 30;
			double            		LastCaptureTime = -1e+10;
			DG::Uint32            	FramesToCapture = 0;
			DG::Uint32            	CurrentFrame    = 0;
			DG::IMAGE_FILE_FORMAT 	FileFormat      = DG::IMAGE_FILE_FORMAT_PNG;
			int               		JpegQuality     = 95;
			bool              		KeepAlpha       = false;

		} mScreenCaptureInfo;

		std::unique_ptr<DG::ScreenCapture> mScreenCapture;
		std::unique_ptr<DG::ImGuiImplDiligent> mImGui;

		GoldenImageMode mGoldenImgMode           = GoldenImageMode::None;
		int             mGoldenImgPixelTolerance = 0;
		int             mExitCode                = 0;

		static Engine* mGlobalInstance;

	public:

		inline void Yield() {
			mThreadPool.Yield();
		}
		inline void YieldUntil(TaskBarrier* barrier) {
			mThreadPool.YieldUntil(barrier);
		}
		inline void YieldFor(const std::chrono::high_resolution_clock::duration& duration) {
			mThreadPool.YieldFor(duration);
		}
		inline void YieldUntil(const std::chrono::high_resolution_clock::time_point& time) {
			mThreadPool.YieldUntil(time);
		}

		DG::float4x4 GetSurfacePretransformMatrix(const DG::float3& f3CameraViewAxis) const;
		DG::float4x4 GetAdjustedProjectionMatrix(float FOV, float NearPlane, float FarPlane) const;
		DG::float4x4 GetAdjustedOrthoMatrix(const DG::float2& fCameraSize, float NearPlane, float FarPlane) const;

		virtual bool IsReady() const override final
		{
			return mPlatform && 
				mPlatform->IsValid() && 
				bValid && 
				mDevice && 
				mSwapChain && 
				mImmediateContext;
		}

		void SetScene(Scene* scene, bool bUnloadOld = true);

		inline InputController& GetInputController() {
			return mInputController;
		}
		inline DG::IEngineFactory* GetEngineFactory() {
			return mEngineFactory;
		}
		inline DG::IRenderDevice* GetDevice() {
			return mDevice;
		}
		inline DG::IDeviceContext* GetImmediateContext() {
			return mImmediateContext;
		}
		inline std::vector<DG::IDeviceContext*> GetDeferredContexts() {
			return mDeferredContexts;
		}
		inline DG::ISwapChain* GetSwapChain() {
			return mSwapChain;
		}
		inline IPlatform* GetPlatform() {
			return mPlatform;
		}
		inline IRenderer* GetRenderer() {
			return mRenderer;
		}
		inline ResourceManager* GetResourceManager() {
			return mResourceManager;
		}
		inline Scene* GetScene() {
			return mScene;
		}
		inline bool GetShowUI() const {
			return bShowUI;
		}
		inline void SetShowUI(bool value) {
			bShowUI = value;
		}
		inline DG::ImGuiImplDiligent* GetUI() {
			return mImGui.get();
		}
		inline ThreadPool* GetThreadPool() {
			return &mThreadPool;
		}

		void InitializeDefaultSystems(Scene* scene);

		friend Engine* GetEngine();
		friend class Scene;
	};

	inline Engine* GetEngine() {
		return Engine::mGlobalInstance;
	}
}