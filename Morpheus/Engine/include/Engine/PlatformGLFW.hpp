#pragma once

#ifdef USE_GLFW

#include <Engine/Platform.hpp>

typedef struct GLFWwindow GLFWwindow;

namespace Morpheus {
	typedef std::function<bool(GLFWwindow* window, int key, int scancode, int action, int mods)>
		glfw_key_callback_t;
	typedef std::function<bool(GLFWwindow* window, double xoffset, double yoffset)>	
		glfw_scroll_callback_t;
	typedef std::function<bool(GLFWwindow* window, unsigned int c)>
		glfw_char_callback_t;

	class PlatformGLFW : public IPlatform {
	private:
		GLFWwindow* mWindow = nullptr;
		bool bOwnsWindow;
		PlatformParams mParams;

	public:
		PlatformGLFW();
		PlatformGLFW(GLFWwindow* window);
		~PlatformGLFW();

		void Startup(const PlatformParams& params) override;
		void Shutdown() override;

		inline GLFWwindow* GetWindow() const {
			return mWindow;
		}

		// Adds a delegate that will be called when the window is resized.
		void AddUserResizeHandler(user_window_resize_t* handler) override;
		// Removes a delegate that is called when the window is resized.
		void RemoveUserResizeHandler(user_window_resize_t* handler) override;

		bool IsValid() const override;
		void MessagePump() override;

		PlatformParams GetParams() const override;
		PlatformGLFW* ToGLFW() override;
	};
}

#endif