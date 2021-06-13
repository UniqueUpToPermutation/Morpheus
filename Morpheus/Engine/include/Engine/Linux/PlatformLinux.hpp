#pragma once

#include <Engine/Platform.hpp>
#include <Engine/InputController.hpp>

#include <set>

#include "PlatformDefinitions.h"
#include "NativeAppBase.hpp"
#include "StringTools.hpp"
#include "Timer.hpp"
#include "Errors.hpp"

#if VULKAN_SUPPORTED
#    include "ImGuiImplLinuxXCB.hpp"
#endif
#include "ImGuiImplLinuxX11.hpp"

namespace DG = Diligent;

namespace Morpheus {

#ifdef VULKAN_SUPPORTED
	struct XCBInfo
	{
		xcb_connection_t*        connection            = nullptr;
		uint32_t                 window                = 0;
		uint16_t                 width                 = 0;
		uint16_t                 height                = 0;
		xcb_intern_atom_reply_t* atom_wm_delete_window = nullptr;
	};
#endif

	typedef std::function<int(XEvent*)> linux_event_handler_x_t;
#if VULKAN_SUPPORTED
	typedef std::function<int(xcb_generic_event_t*)> linux_event_handler_xcb_t;
#endif 

	enum class PlatformLinuxMode {
		X11,
		XCB
	};

	class PlatformLinux : public IPlatform {
	private:
		InputController	mInput;
		PlatformParams mParams;
		Display* mDisplay;
		Window mWindow;
		GLXContext mGLXContext;
		std::string mTitle;
		XCBInfo mXCBInfo;
		bool bQuit;
		bool bIsInitialized = false;
		PlatformLinuxMode mMode;

		std::set<user_window_resize_t*> mWindowResizeHandlers;
		std::set<linux_event_handler_x_t*> mEventHandlersX;
		
		int InitializeGL(const PlatformParams& params);
		int HandleXEvent(XEvent* xev);
		void HandleWindowResize(uint width, uint height);

#if VULKAN_SUPPORTED
		int InitializeVulkan(const PlatformParams& params);
		std::set<linux_event_handler_xcb_t*> mEventHandlersXCB;
		void HandleXCBEvent(xcb_generic_event_t* event);
#endif

	public:
		inline void AddXEventHandler(linux_event_handler_x_t* handler) {
			mEventHandlersX.emplace(handler);
		}
		void RemoveXEventHandler(linux_event_handler_x_t* handler) {
			mEventHandlersX.erase(handler);
		}

#if VULKAN_SUPPORTED
		void AddXCBEventHandler(linux_event_handler_xcb_t* handler) {
			mEventHandlersXCB.emplace(handler);
		}
		void RemoveXCBEventHandler(linux_event_handler_xcb_t* handler) {
			mEventHandlersXCB.erase(handler);
		}
#endif
	
		PlatformLinux();

		DG::LinuxNativeWindow GetNativeWindow() const;

		int Startup(const PlatformParams& params) override;
		void Shutdown() override;
		bool IsValid() const override;
		void MessagePump() override;
		void Flush() override;
		void Show() override;
		void Hide() override;
		void SetCursorVisible(bool value) override;
		const PlatformParams& GetParameters() const override;
		const InputController& GetInput() const override;

		void AddUserResizeHandler(user_window_resize_t* handler) override;
		void RemoveUserResizeHandler(user_window_resize_t* handler) override;

		inline bool IsX11() const {
			return mMode == PlatformLinuxMode::X11;
		}
		inline bool IsXCB() const {
			return mMode == PlatformLinuxMode::XCB;
		}

#if VULKAN_SUPPORTED	
		inline const XCBInfo& GetXCBInfo() const {
			return mXCBInfo;
		}
#endif

		PlatformLinux* ToLinux() override;
		PlatformWin32* ToWindows() override;
	};
}