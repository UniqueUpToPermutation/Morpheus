
#include <Engine/Engine.hpp>
#include <Engine/Linux/PlatformLinux.hpp>

#include "PlatformDefinitions.h"
#include "NativeAppBase.hpp"
#include "StringTools.hpp"
#include "Timer.hpp"
#include "Errors.hpp"

#if VULKAN_SUPPORTED
#    include "ImGuiImplLinuxXCB.hpp"
#endif
#include "ImGuiImplLinuxX11.hpp"

namespace Morpheus
{
#if VULKAN_SUPPORTED
	struct xcb_size_hints_t
	{
		uint32_t flags;                          /** User specified flags */
		int32_t  x, y;                           /** User-specified position */
		int32_t  width, height;                  /** User-specified size */
		int32_t  min_width, min_height;          /** Program-specified minimum size */
		int32_t  max_width, max_height;          /** Program-specified maximum size */
		int32_t  width_inc, height_inc;          /** Program-specified resize increments */
		int32_t  min_aspect_num, min_aspect_den; /** Program-specified minimum aspect ratios */
		int32_t  max_aspect_num, max_aspect_den; /** Program-specified maximum aspect ratios */
		int32_t  base_width, base_height;        /** Program-specified base size */
		uint32_t win_gravity;                    /** Program-specified window gravity */
	};

	enum XCB_SIZE_HINT
	{
		XCB_SIZE_HINT_US_POSITION   = 1 << 0,
		XCB_SIZE_HINT_US_SIZE       = 1 << 1,
		XCB_SIZE_HINT_P_POSITION    = 1 << 2,
		XCB_SIZE_HINT_P_SIZE        = 1 << 3,
		XCB_SIZE_HINT_P_MIN_SIZE    = 1 << 4,
		XCB_SIZE_HINT_P_MAX_SIZE    = 1 << 5,
		XCB_SIZE_HINT_P_RESIZE_INC  = 1 << 6,
		XCB_SIZE_HINT_P_ASPECT      = 1 << 7,
		XCB_SIZE_HINT_BASE_SIZE     = 1 << 8,
		XCB_SIZE_HINT_P_WIN_GRAVITY = 1 << 9
	};
#endif

	typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, int, const int*);

	static constexpr int None = 0;

	static constexpr uint16_t WindowWidth     = 1024;
	static constexpr uint16_t WindowHeight    = 768;
	static constexpr uint16_t MinWindowWidth  = 320;
	static constexpr uint16_t MinWindowHeight = 240;

	PlatformLinux::PlatformLinux() : mEngine(nullptr),
		bQuit(false) {
	}

	int PlatformLinux::InitializeVulkan() {

		int scr         = 0;
		mXCBInfo.connection = xcb_connect(nullptr, &scr);
		if (mXCBInfo.connection == nullptr || xcb_connection_has_error(mXCBInfo.connection))
		{
			std::cerr << "Unable to make an XCB connection\n";
			exit(-1);
		}

		const xcb_setup_t*    setup = xcb_get_setup(mXCBInfo.connection);
		xcb_screen_iterator_t iter  = xcb_setup_roots_iterator(setup);
		while (scr-- > 0)
			xcb_screen_next(&iter);

		auto screen = iter.data;

		mXCBInfo.width  = WindowWidth;
		mXCBInfo.height = WindowHeight;

		uint32_t value_mask, value_list[32];

		mXCBInfo.window = xcb_generate_id(mXCBInfo.connection);

		value_mask    = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
		value_list[0] = screen->black_pixel;
		value_list[1] =
			XCB_EVENT_MASK_KEY_RELEASE |
			XCB_EVENT_MASK_KEY_PRESS |
			XCB_EVENT_MASK_EXPOSURE |
			XCB_EVENT_MASK_STRUCTURE_NOTIFY |
			XCB_EVENT_MASK_POINTER_MOTION |
			XCB_EVENT_MASK_BUTTON_PRESS |
			XCB_EVENT_MASK_BUTTON_RELEASE;

		xcb_create_window(mXCBInfo.connection, XCB_COPY_FROM_PARENT, mXCBInfo.window, screen->root, 0, 0, mXCBInfo.width, mXCBInfo.height, 0,
						XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, value_mask, value_list);

		// Magic code that will send notification when window is destroyed
		xcb_intern_atom_cookie_t cookie = xcb_intern_atom(mXCBInfo.connection, 1, 12, "WM_PROTOCOLS");
		xcb_intern_atom_reply_t* reply  = xcb_intern_atom_reply(mXCBInfo.connection, cookie, 0);

		xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(mXCBInfo.connection, 0, 16, "WM_DELETE_WINDOW");
		mXCBInfo.atom_wm_delete_window       = xcb_intern_atom_reply(mXCBInfo.connection, cookie2, 0);

		xcb_change_property(mXCBInfo.connection, XCB_PROP_MODE_REPLACE, mXCBInfo.window, (*reply).atom, 4, 32, 1,
							&(*mXCBInfo.atom_wm_delete_window).atom);
		free(reply);

		mTitle = mEngine->GetAppTitle();

		// https://stackoverflow.com/a/27771295
		xcb_size_hints_t hints = {};
		hints.flags            = XCB_SIZE_HINT_P_MIN_SIZE;
		hints.min_width        = MinWindowWidth;
		hints.min_height       = MinWindowHeight;
		xcb_change_property(mXCBInfo.connection, XCB_PROP_MODE_REPLACE, mXCBInfo.window, XCB_ATOM_WM_NORMAL_HINTS, XCB_ATOM_WM_SIZE_HINTS,
							32, sizeof(xcb_size_hints_t), &hints);

		xcb_map_window(mXCBInfo.connection, mXCBInfo.window);

		// Force the x/y coordinates to 100,100 results are identical in consecutive
		// runs
		const uint32_t coords[] = {100, 100};
		xcb_configure_window(mXCBInfo.connection, mXCBInfo.window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
		xcb_flush(mXCBInfo.connection);

		xcb_generic_event_t* e;
		while ((e = xcb_wait_for_event(mXCBInfo.connection)))
		{
			if ((e->response_type & ~0x80) == XCB_EXPOSE) break;
		}

		xcb_change_property(mXCBInfo.connection, XCB_PROP_MODE_REPLACE, mXCBInfo.window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING,
			8, mTitle.length(), mTitle.c_str());

		if (!mEngine->InitVulkan(mXCBInfo.connection, mXCBInfo.window)) {
        	throw std::runtime_error("Could not initialize Vulkan!");
		}

		xcb_flush(mXCBInfo.connection);

		mDeviceType = DG::RENDER_DEVICE_TYPE_VULKAN;

		return 1;
	}

	int PlatformLinux::InitializeGL() {
		mDisplay = XOpenDisplay(0);

		// clang-format off
		static int visual_attribs[] =
		{
			GLX_RENDER_TYPE,    GLX_RGBA_BIT,
			GLX_DRAWABLE_TYPE,  GLX_WINDOW_BIT,
			GLX_DOUBLEBUFFER,   true,

			// The largest available total RGBA color buffer size (sum of GLX_RED_SIZE, 
			// GLX_GREEN_SIZE, GLX_BLUE_SIZE, and GLX_ALPHA_SIZE) of at least the minimum
			// size specified for each color component is preferred.
			GLX_RED_SIZE,       8,
			GLX_GREEN_SIZE,     8,
			GLX_BLUE_SIZE,      8,
			GLX_ALPHA_SIZE,     8,

			// The largest available depth buffer of at least GLX_DEPTH_SIZE size is preferred
			GLX_DEPTH_SIZE,     24,

			GLX_SAMPLE_BUFFERS, 0,

			// Setting GLX_SAMPLES to 1 results in 2x MS backbuffer, which is 
			// against the spec that states:
			//     if GLX SAMPLE BUFFERS is zero, then GLX SAMPLES will also be zero
			// GLX_SAMPLES, 1,

			None
		};
		// clang-format on

		int          fbcount = 0;
		GLXFBConfig* fbc     = glXChooseFBConfig(mDisplay, DefaultScreen(mDisplay), visual_attribs, &fbcount);
		if (!fbc)
		{
			LOG_ERROR_MESSAGE("Failed to retrieve a framebuffer config");
			return 0;
		}

		XVisualInfo* vi = glXGetVisualFromFBConfig(mDisplay, fbc[0]);

		XSetWindowAttributes swa;
		swa.colormap     = XCreateColormap(mDisplay, RootWindow(mDisplay, vi->screen), vi->visual, AllocNone);
		swa.border_pixel = 0;
		swa.event_mask =
			StructureNotifyMask |
			ExposureMask |
			KeyPressMask |
			KeyReleaseMask |
			ButtonPressMask |
			ButtonReleaseMask |
			PointerMotionMask;

		Window mWindow = XCreateWindow(mDisplay, RootWindow(mDisplay, vi->screen), 0, 0, 
			WindowWidth, WindowHeight, 0, vi->depth, InputOutput, vi->visual, CWBorderPixel | CWColormap | CWEventMask, &swa);
		if (!mWindow)
		{
			LOG_ERROR_MESSAGE("Failed to create window.");
			return 0;
		}

		{
			auto SizeHints        = XAllocSizeHints();
			SizeHints->flags      = PMinSize;
			SizeHints->min_width  = MinWindowWidth;
			SizeHints->min_height = MinWindowHeight;
			XSetWMNormalHints(mDisplay, mWindow, SizeHints);
			XFree(SizeHints);
		}

		XMapWindow(mDisplay, mWindow);

		glXCreateContextAttribsARBProc glXCreateContextAttribsARB = nullptr;
		{
			// Create an oldstyle context first, to get the correct function pointer for glXCreateContextAttribsARB
			GLXContext ctx_old         = glXCreateContext(mDisplay, vi, 0, GL_TRUE);
			glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");
			glXMakeCurrent(mDisplay, None, NULL);
			glXDestroyContext(mDisplay, ctx_old);
		}

		if (glXCreateContextAttribsARB == nullptr)
		{
			LOG_ERROR("glXCreateContextAttribsARB entry point not found. Aborting.");
			return 0;
		}

		int Flags = GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
	#ifdef _DEBUG
		Flags |= GLX_CONTEXT_DEBUG_BIT_ARB;
	#endif

		int major_version = 4;
		int minor_version = 3;

		static int context_attribs[] =
			{
				GLX_CONTEXT_MAJOR_VERSION_ARB, major_version,
				GLX_CONTEXT_MINOR_VERSION_ARB, minor_version,
				GLX_CONTEXT_FLAGS_ARB, Flags,
				None //
			};

		constexpr int True = 1;
		mGLXContext  = glXCreateContextAttribsARB(mDisplay, fbc[0], NULL, True, context_attribs);
		if (!mGLXContext)
		{
			LOG_ERROR("Failed to create GL context.");
			return 0;
		}
		XFree(fbc);

		glXMakeCurrent(mDisplay, mWindow, mGLXContext);

		mDeviceType = DG::RENDER_DEVICE_TYPE_GL;
	
		mTitle = mEngine->GetAppTitle();
		if (!mEngine->OnGLContextCreated(mDisplay, mWindow))
		{
			LOG_ERROR("Unable to initialize the application in OpenGL mode. Aborting");
			return 0;
		}
		XStoreName(mDisplay, mWindow, mTitle.c_str());

		mTimer.Restart();
		mPrevTime = mTimer.GetElapsedTime();
		return 1;
	}

	void PlatformLinux::Flush() {
#if VULKAN_SUPPORTED
		if (mDeviceType == DG::RENDER_DEVICE_TYPE_VULKAN) {
			xcb_flush(mXCBInfo.connection);
		}
#endif
	}

	void PlatformLinux::MessageLoop() {
		if (mDeviceType == DG::RENDER_DEVICE_TYPE_GL) {
			XEvent xev;
			// Handle all events in the queue
			while (XCheckMaskEvent(mDisplay, 0xFFFFFFFF, &xev))
			{
				mEngine->HandleXEvent(&xev);
				switch (xev.type)
				{
					case ConfigureNotify:
					{
						XConfigureEvent& xce = reinterpret_cast<XConfigureEvent&>(xev);
						if (xce.width != 0 && xce.height != 0)
							mEngine->WindowResize(xce.width, xce.height);
						break;
					}
				}
			}

			// Render the scene
			auto CurrTime    = mTimer.GetElapsedTime();
			auto ElapsedTime = CurrTime - mPrevTime;
			mPrevTime         = CurrTime;

			mEngine->Update(CurrTime, ElapsedTime);
		}
#if VULKAN_SUPPORTED
		else if (mDeviceType == DG::RENDER_DEVICE_TYPE_VULKAN) {
			xcb_generic_event_t* event = nullptr;

			while ((event = xcb_poll_for_event(mXCBInfo.connection)) != nullptr)
			{
				mEngine->HandleXCBEvent(event);
				switch (event->response_type & 0x7f)
				{
					case XCB_CLIENT_MESSAGE:
						if ((*(xcb_client_message_event_t*)event).data.data32[0] ==
							(*mXCBInfo.atom_wm_delete_window).atom)
						{
							bQuit = true;
						}
						break;

					case XCB_DESTROY_NOTIFY:
						bQuit = true;
						break;

					case XCB_CONFIGURE_NOTIFY:
					{
						const auto* cfgEvent = reinterpret_cast<const xcb_configure_notify_event_t*>(event);
						if ((cfgEvent->width != mXCBInfo.width) || (cfgEvent->height != mXCBInfo.height))
						{
							mXCBInfo.width  = cfgEvent->width;
							mXCBInfo.height = cfgEvent->height;
							if ((mXCBInfo.width > 0) && (mXCBInfo.height > 0))
							{
								mEngine->WindowResize(mXCBInfo.width, mXCBInfo.height);
							}
						}
					}
					break;

					default:
						break;
				}
				free(event);
			}

			// Update
			auto CurrTime    = mTimer.GetElapsedTime();
			auto ElapsedTime = CurrTime - mPrevTime;
			mPrevTime         = CurrTime;

			mEngine->Update(CurrTime, ElapsedTime);
		}
#endif
	}

	int PlatformLinux::Initialize(Engine* engine, int argc, char** argv) {
		bool UseVulkan = false;

		mEngine = engine;

#if VULKAN_SUPPORTED
		UseVulkan = true;
		if (argc > 1)
		{
			const auto* Key = "-mode ";
			const auto* pos = strstr(argv[1], Key);
			if (pos != nullptr)
			{
				pos += strlen(Key);
				while (*pos != 0 && *pos == ' ') ++pos;
				if (strcasecmp(pos, "GL") == 0)
				{
					UseVulkan = false;
				}
				else if (strcasecmp(pos, "VK") == 0)
				{
					UseVulkan = true;
				}
				else
				{
					std::cerr << "Unknown device type. Only the following types are supported: GL, VK";
					return 0;
				}
			}
		}

		if (UseVulkan)
		{
			auto ret = InitializeVulkan();
			if (ret)
			{
				return ret;
			}
			else
			{
				LOG_ERROR_MESSAGE("Failed to initialize the engine in Vulkan mode. Attempting to use OpenGL");
			}
		}
	#endif

		return InitializeGL();
	}

	void PlatformLinux::Shutdown() {
		if (mDeviceType == DG::RENDER_DEVICE_TYPE_GL) {
			glXMakeCurrent(mDisplay, None, NULL);
			glXDestroyContext(mDisplay, mGLXContext);
			XDestroyWindow(mDisplay, mWindow);
			XCloseDisplay(mDisplay);
		}
#if VULKAN_SUPPORTED
		else if (mDeviceType == DG::RENDER_DEVICE_TYPE_VULKAN) {
			xcb_destroy_window(mXCBInfo.connection, mXCBInfo.window);
    		xcb_disconnect(mXCBInfo.connection);
		}
#endif
		else {
			throw std::runtime_error("Unknown device type!");
		}
	}

	bool PlatformLinux::IsValid() const {
		return !bQuit;
	}

	PlatformLinux* PlatformLinux::ToLinux() {
		return this;
	}

	PlatformWindows* PlatformLinux::ToWindows() {
		return nullptr;
	}

	IPlatform* CreatePlatform() {
		return new PlatformLinux();
	}
}