#pragma once

#include "GraphicsTypes.h"

#include <Engine/Defines.hpp>

#include <functional>

namespace DG = Diligent;

#if PLATFORM_WIN32
#define MAIN() int __stdcall WinMain( \
	HINSTANCE hInstance, \
	HINSTANCE hPrevInstance, \
	LPSTR     lpCmdLine, \
	int       nShowCmd) 
#elif PLATFORM_LINUX
#define MAIN() int main(int argc, char** argv) 
#endif

#ifdef USE_GLFW
typedef struct GLFWwindow GLFWwindow;
#endif

namespace Morpheus {

	
	typedef std::function<void(double, double)> update_callback_t;

	struct EngineParams;

	struct PlatformParams {
		std::string mWindowTitle 	= "Morpheus";
		uint mWindowWidth 			= 1024;
		uint mWindowHeight   		= 756;
		bool bFullscreen 			= false;
		bool bShowOnCreation		= true;

		// Device type for use by Graphics class
		DG::RENDER_DEVICE_TYPE   mDeviceType 		= DG::RENDER_DEVICE_TYPE_UNDEFINED;
	};

	typedef std::function<void(uint, uint)> user_window_resize_t;

	class IPlatform {
	public:
		virtual ~IPlatform() = default;

		virtual void Startup(const PlatformParams& params = PlatformParams()) = 0;
		virtual void Shutdown() = 0;

		// Adds a delegate that will be called when the window is resized.
		virtual void AddUserResizeHandler(user_window_resize_t* handler) = 0;
		// Removes a delegate that is called when the window is resized.
		virtual void RemoveUserResizeHandler(user_window_resize_t* handler) = 0;

		virtual bool IsValid() const = 0;
		virtual void MessagePump() = 0;

		virtual PlatformParams GetParams() const  = 0;
		virtual PlatformGLFW* ToGLFW() = 0;
	};

	IPlatform* CreatePlatform();
#ifdef USE_GLFW
	IPlatform* CreatePlatformGLFW();
	IPlatform* CreatePlatformGLFW(GLFWwindow* window);
#endif

	class Platform {
	private:
		IPlatform* mPlatform = nullptr;

	public:
		inline Platform() : mPlatform(CreatePlatform()) {
		}

		inline Platform(IPlatform* platform) : mPlatform(platform) {
		}

#ifdef USE_GLFW
		inline Platform(GLFWwindow* window) : mPlatform(CreatePlatformGLFW(window)) {
		}
#endif

		inline ~Platform() {
			if (mPlatform) {
				mPlatform->Shutdown();
				delete mPlatform;
			}
		}

		inline IPlatform* operator->() {
			return mPlatform;
		}

		inline operator IPlatform*() {
			return mPlatform;
		}

		Platform(const Platform& plat) = delete;
		Platform& operator=(const Platform& plat) = delete;

		Platform(Platform&& plat) = default;
		Platform& operator=(Platform&& plat) = default;

		// Adds a delegate that will be called when the window is resized.
		inline void AddUserResizeHandler(user_window_resize_t* handler) {
			mPlatform->AddUserResizeHandler(handler);
		}
		// Removes a delegate that is called when the window is resized.
		inline void RemoveUserResizeHandler(user_window_resize_t* handler) {
			mPlatform->RemoveUserResizeHandler(handler);
		}

		inline void Startup(const PlatformParams& params = PlatformParams()) {
			mPlatform->Startup(params);
		}

		inline void Shutdown() {
			mPlatform->Shutdown();
		}

		inline bool IsValid() const {
			return mPlatform->IsValid();
		}

		inline void MessagePump() {
			mPlatform->MessagePump();
		}

#ifdef USE_GLFW
		PlatformGLFW* ToGLFW();
		GLFWwindow* GetWindowGLFW();
#endif
	};
}