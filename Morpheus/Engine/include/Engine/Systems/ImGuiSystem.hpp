#pragma once

#include <Engine/Systems/System.hpp>
#include <Engine/Platform.hpp>
#include <Engine/Graphics.hpp>

#ifdef PLATFORM_LINUX
#include <Engine/Linux/PlatformLinux.hpp>
#endif

#ifdef PLATFORM_WIN32
#include <Engine/Win32/PlatformWin32.hpp>
#endif

namespace Diligent
{
	class ImGuiImplDiligent;
}

namespace Morpheus {

	class ImGuiSystem : public ISystem {
	private:
		std::unique_ptr<DG::ImGuiImplDiligent> mImGui;

#ifdef PLATFORM_LINUX
		linux_event_handler_x_t mLinuxXEventHandler;
#if VULKAN_SUPPORTED
		linux_event_handler_xcb_t mLinuxXCBEventHandler;
#endif
#endif

		RealtimeGraphics* mGraphics;
		IPlatform* mPlatform;
		bool bShow = true;

	public:
		inline DG::ImGuiImplDiligent* GetImGui() const {
			return mImGui.get();
		}

		inline ImGuiSystem(RealtimeGraphics& graphics) :
			mPlatform(graphics.GetPlatform()), mGraphics(&graphics) {
		}

		Task Startup(SystemCollection& systems) override;
		bool IsInitialized() const override;
		void Shutdown() override;
		void NewFrame(Frame* frame) override;
		void OnAddedTo(SystemCollection& collection) override;
	};
}