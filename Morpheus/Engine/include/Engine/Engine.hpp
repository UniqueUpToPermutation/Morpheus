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

#include <Engine/Defines.hpp>
#include <Engine/Platform.hpp>
#include <Engine/InputController.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Scene.hpp>
#include <Engine/Entity.hpp>
#include <Engine/ThreadPool.hpp>
#include <Engine/Components/ScriptComponent.hpp>

namespace Diligent
{
	class ImGuiImplDiligent;
}

namespace DG = Diligent;

namespace Morpheus {

	class IRenderer;
	class Scene;

	struct WindowParams {
		std::string mWindowTitle = "Morpheus";
	};

	struct ThreadParams {
		int mThreadCount = -1;
	};

	struct DisplayParams {
		uint mWidth = 1024;
		uint mHeight = 756;
		bool bFullscreen = false;
		bool bVSync = false;
	};

	struct ImFontLoad {
		std::string mFile;
		float mFontSize;

		ImFontLoad(const std::string& file, float fontSize) : 
			mFile(file), mFontSize(fontSize) {
		}
	};

	struct ImGuiParams {
		float mUIScale = 1.0f;
		std::vector<ImFontLoad> mExternalFonts;
	};

	struct RendererParams {
		DG::RENDER_DEVICE_TYPE 	mBackendType		= DG::RENDER_DEVICE_TYPE_UNDEFINED;
		DG::ADAPTER_TYPE 		mAdapterType 		= DG::ADAPTER_TYPE_UNKNOWN;
		DG::Uint32      		mAdapterId 			= 0;
		DG::Int32 				mValidationLevel 	= -1;
	};

	struct EngineParams {
		DisplayParams mDisplay;
		ThreadParams mThreads;
		WindowParams mWindow;
		RendererParams mRenderer;
		ImGuiParams mImGui;
	};

	class IEngineComponent {
	private:
		bool bShowUI = true;
	
	public:
		virtual void Initialize(Engine* engine) = 0;
		virtual void InitializeSystems(Scene* scene) = 0;

		virtual void RenderUI();

		inline bool GetShowUI() const {
			return bShowUI;
		}

		inline void SetShowUI(bool value) {
			bShowUI = value;
		}

		virtual IRenderer* ToRenderer();
		virtual const IRenderer* ToRenderer() const;

		virtual ~IEngineComponent() {
		}
	};

	class Engine {
	public:
		Engine();
		~Engine();

		void Update(Scene* activeScene);
		void Update(const update_callback_t& callback);
		void WindowResize(int width, int height);
		void Render(Scene* activeScene);
		void RenderUI();
		void Present();

		void SelectDeviceType();
		void Startup(const EngineParams& params);
		inline void Startup() {
			Startup(EngineParams());
		}
		void Shutdown();
		void CollectGarbage();
		
		inline void GetDesiredInitialWindowSize(int& width, int& height) const
		{
			width  = mInitialWindowWidth;
			height = mInitialWindowHeight;
		}

#if PLATFORM_LINUX
		bool OnGLContextCreated(Display* display, Window window);
		int HandleXEvent(XEvent* xev);

#if VULKAN_SUPPORTED
		bool InitVulkan(xcb_connection_t* connection, uint32_t window);
		void HandleXCBEvent(xcb_generic_event_t* event);
#endif
#endif

#if PLATFORM_WIN32
		void OnWindowCreated(HWND hWnd, LONG WindowWidth, LONG WindowHeight);
		LRESULT HandleWin32Message(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#endif

	private:
		void ProcessConfigParams(const EngineParams& params);
		void Update(double CurrTime, double ElapsedTime);
		void OnPreWindowResized();
		void OnWindowResized(uint width, uint height);

		void GetEngineInitializationAttribs(DG::RENDER_DEVICE_TYPE DeviceType, 
			DG::EngineCreateInfo& EngineCI, DG::SwapChainDesc& SCDesc);
		void InitializeDiligentEngine(const DG::NativeWindow* pWindow);
		void UpdateAdaptersDialog();
		void SetImGuiDefaults(const EngineParams& params);

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

		DG::RENDER_DEVICE_TYPE        		mDeviceType 		= DG::RENDER_DEVICE_TYPE_UNDEFINED;
		DG::IEngineFactory*           		mEngineFactory 		= nullptr;
		DG::IRenderDevice*              	mDevice 			= nullptr;
		DG::IDeviceContext*              	mImmediateContext 	= nullptr;
		std::vector<DG::IDeviceContext*> 	mDeferredContexts;
		DG::ISwapChain*                  	mSwapChain 			= nullptr;
		DG::GraphicsAdapterInfo            	mAdapterAttribs;
		std::vector<DG::DisplayModeAttribs>	mDisplayModes;
		std::vector<IEngineComponent*> 		mComponents;

		InputController		mInputController;
		IPlatform*			mPlatform				= nullptr;
		ResourceManager* 	mResourceManager 		= nullptr;
		IRenderer*			mRenderer 				= nullptr;
		ThreadPool			mThreadPool;
		ScriptFactory		mScriptFactory;

		bool 				bRendererWarningGiven 	= false;
		int          		mInitialWindowWidth  	= 0;
		int          		mInitialWindowHeight 	= 0;
		int         		mValidationLevel     	= -1;
		DG::Uint32      	mAdapterId   			= 0;
		DG::ADAPTER_TYPE 	mAdapterType 			= DG::ADAPTER_TYPE_UNKNOWN;
		std::string  		mAdapterDetailsString;
		int          		mSelectedDisplayMode  	= 0;
		bool         		bVSync               	= false;
		bool         		bFullScreenMode      	= false;
		bool         		bUseSRGBSwapChain		= false;
		bool         		bShowAdaptersDialog  	= true;
		bool         		bShowUI              	= true;
		bool         		bForceNonSeprblProgs 	= true;
		bool 		 		bValid 					= true;
		double       		mCurrentTime          	= 0;
		DG::Uint32   		mMaxFrameLatency      	= DG::SwapChainDesc{}.BufferCount;

		// We will need this when we have to recreate the swap chain (on Android)
		DG::SwapChainDesc mSwapChainInitDesc;

		std::unique_ptr<DG::ImGuiImplDiligent> mImGui;

		static Engine* mGlobalInstance;

	public:

		// Creates a component on this engine, should be called before startup.
		// The engine assumes ownership of the component.
		template <typename T, typename... Args>
		T* AddComponent(Args &&... args) {
			T* result = new T(std::forward(args)...);
			mComponents.emplace_back(result);

			IRenderer* rendererInterface = result->ToRenderer();

			if (rendererInterface) {
				if (mRenderer) {
					throw std::runtime_error("Engine already has renderer!");
				}
				mRenderer = rendererInterface;
			}

			return result;
		}

		inline void YieldUntilEmpty() {
			mThreadPool.YieldUntilEmpty();
		}

		inline void YieldFor(const std::chrono::high_resolution_clock::duration& time) {
			mThreadPool.YieldFor(time);
		}

		DG::float4x4 GetSurfacePretransformMatrix(const DG::float3& f3CameraViewAxis) const;
		DG::float4x4 GetAdjustedProjectionMatrix(float FOV, float NearPlane, float FarPlane) const;
		DG::float4x4 GetAdjustedOrthoMatrix(const DG::float2& fCameraSize, float NearPlane, float FarPlane) const;

		inline bool IsReady() const
		{
			return mPlatform && 
				mPlatform->IsValid() && 
				bValid && 
				mDevice && 
				mSwapChain && 
				mImmediateContext;
		}
		
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
		inline const IRenderer* GetRenderer() const {
			return mRenderer;
		}
		inline ResourceManager* GetResourceManager() {
			return mResourceManager;
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
		inline ScriptFactory* GetScriptFactory() {
			return &mScriptFactory;
		}

		void InitializeDefaultSystems(Scene* scene);

		friend Engine* GetEngine();
		friend class Scene;
	};

	inline Engine* GetEngine() {
		return Engine::mGlobalInstance;
	}
}