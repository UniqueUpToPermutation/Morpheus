#pragma once

#include <Engine/Platform.hpp>

#include "Timer.hpp"

namespace DG = Diligent;

namespace Morpheus {

	struct XCBInfo
	{
		xcb_connection_t*        connection            = nullptr;
		uint32_t                 window                = 0;
		uint16_t                 width                 = 0;
		uint16_t                 height                = 0;
		xcb_intern_atom_reply_t* atom_wm_delete_window = nullptr;
	};

	class PlatformLinux : public IPlatform {
	private:
		Display* mDisplay;
		Window mWindow;
		GLXContext mGLXContext;
		DG::RENDER_DEVICE_TYPE mDeviceType;
		double mPrevTime;
		DG::Timer mTimer;
		std::string mTitle;
		XCBInfo mXCBInfo;
		Engine* mEngine;
		bool bQuit;

		int InitializeGL(const EngineParams& params);
		int InitializeVulkan(const EngineParams& params);

	public:
		PlatformLinux();

		int Initialize(Engine* engine, 
			const EngineParams& params) override;
		void Shutdown() override;
		bool IsValid() const override;
		void MessageLoop(const update_callback_t& callback) override;
		void Flush() override;

		PlatformLinux* ToLinux() override;
		PlatformWin32* ToWindows() override;
	};
}