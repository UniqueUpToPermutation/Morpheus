#include <Engine/Systems/ImGuiSystem.hpp>
#include <Engine/Platform.hpp>

#include "imgui.h"
#include "ImGuiImplDiligent.hpp"
#include "ImGuiUtils.hpp"

#if PLATFORM_LINUX
#if VULKAN_SUPPORTED
#    include "ImGuiImplLinuxXCB.hpp"
#endif
#include "ImGuiImplLinuxX11.hpp"
#endif

#if PLATFORM_WIN32
#include "ImGuiImplWin32.hpp"
#endif

namespace Morpheus {
	std::unique_ptr<Task> ImGuiSystem::Startup(SystemCollection& systems) {

		const auto& scDesc = mGraphics->SwapChain()->GetDesc();

#ifdef PLATFORM_LINUX
		auto linuxPlat = mPlatform->ToLinux();

#ifdef VULKAN_SUPPORTED
		if (linuxPlat->IsXCB()) {
			const auto& xcb_info = linuxPlat->GetXCBInfo();

			auto xcbImgui = new DG::ImGuiImplLinuxXCB(xcb_info.connection, mGraphics->Device(),
				scDesc.ColorBufferFormat, scDesc.DepthBufferFormat, scDesc.Width, scDesc.Height);

			mImGui.reset(xcbImgui);
		
			mLinuxXCBEventHandler = [xcbImgui](xcb_generic_event_t* e) {
				return xcbImgui->HandleXCBEvent(e);
			};

			linuxPlat->AddXCBEventHandler(&mLinuxXCBEventHandler);
		}
#endif

		if (linuxPlat->IsX11()) {

			auto x11Imgui = new DG::ImGuiImplLinuxX11(mGraphics->Device(), scDesc.ColorBufferFormat, 
				scDesc.DepthBufferFormat, scDesc.Width, scDesc.Height);

			mImGui.reset(x11Imgui);

			mLinuxXEventHandler = [imgui = x11Imgui](XEvent* e) {
				return imgui->HandleXEvent(e);
			};

			linuxPlat->AddXEventHandler(&mLinuxXEventHandler);
		}
		
#else 
		throw std::runtime_error("ImGui not supported on this platform!");
#endif



		return nullptr;
	}

	bool ImGuiSystem::IsInitialized() const {
		return mImGui.get();
	}

	void ImGuiSystem::Shutdown() {

#ifdef PLATFORM_LINUX
		auto linuxPlat = mPlatform->ToLinux();

#ifdef VULKAN_SUPPORTED
		if (linuxPlat->IsXCB()) {
			linuxPlat->RemoveXCBEventHandler(&mLinuxXCBEventHandler);
		}
#endif

		if (linuxPlat->IsX11()) {
			linuxPlat->RemoveXEventHandler(&mLinuxXEventHandler);
		}
#endif

		mImGui.reset();
	}

	void ImGuiSystem::NewFrame(Frame* frame) {
	}

	void ImGuiSystem::OnAddedTo(SystemCollection& collection) {
	}
}