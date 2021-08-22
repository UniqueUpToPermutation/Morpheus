#ifdef USE_GLFW

#include <GLFW/glfw3.h>

#include <Engine/PlatformGLFW.hpp>

namespace Morpheus {
	PlatformGLFW::PlatformGLFW() : bOwnsWindow(true) {
	}

	PlatformGLFW::PlatformGLFW(GLFWwindow* window) : mWindow(window), bOwnsWindow(false) {
	}

	void PlatformGLFW::Startup(const PlatformParams& params) {
		if (bOwnsWindow) {
			if (!glfwInit()) {
				throw std::runtime_error("Failed to initialize glfw!");
			}

			int glfwApiHint = GLFW_NO_API;
			if (params.mDeviceType == DG::RENDER_DEVICE_TYPE_GL) {
				glfwApiHint = GLFW_OPENGL_API;
			}

			glfwWindowHint(GLFW_CLIENT_API, glfwApiHint);
			if (glfwApiHint == GLFW_OPENGL_API)
			{
				// We need compute shaders, so request OpenGL 4.2 at least
				glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
				glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
			}

			GLFWwindow* window = glfwCreateWindow(
				params.mWindowWidth, 
				params.mWindowHeight,
				params.mWindowTitle.c_str(),
				nullptr, 
				nullptr);

			glfwSetWindowUserPointer(window, this);
			glfwSetWindowSizeLimits(window, 320, 240, GLFW_DONT_CARE, GLFW_DONT_CARE);

			if (!window) {
				throw std::runtime_error("Failed to create window!");
			}

			mWindow = window;
			mParams = params;
		}
	}

	void PlatformGLFW::Shutdown() {
		if (bOwnsWindow) {
			glfwDestroyWindow(mWindow);
			glfwTerminate();
		}
	}

	PlatformGLFW::~PlatformGLFW() {
		Shutdown();
	}

	PlatformParams PlatformGLFW::GetParams() const {
		return mParams;
	}

	bool PlatformGLFW::IsValid() const {
		return !glfwWindowShouldClose(mWindow);
	}

	void PlatformGLFW::MessagePump() {
		glfwPollEvents();
	}

	IPlatform* CreatePlatform() {
		return CreatePlatformGLFW();
	}

	IPlatform* CreatePlatformGLFW() {
		return new PlatformGLFW();
	}
	IPlatform* CreatePlatformGLFW(GLFWwindow* window) {
		return new PlatformGLFW(window);
	}

	// Adds a delegate that will be called when the window is resized.
	void PlatformGLFW::AddUserResizeHandler(user_window_resize_t* handler) {

	}

	// Removes a delegate that is called when the window is resized.
	void PlatformGLFW::RemoveUserResizeHandler(user_window_resize_t* handler) {

	}

	PlatformGLFW* PlatformGLFW::ToGLFW() {
		return this;
	}

	inline PlatformGLFW* Platform::ToGLFW() {
		return mPlatform->ToGLFW();
	}

	GLFWwindow* Platform::GetWindowGLFW() {
		auto glfwPlat = mPlatform->ToGLFW();
		if (glfwPlat) {
			return glfwPlat->GetWindow();
		}
		return nullptr;
	}
}

#endif