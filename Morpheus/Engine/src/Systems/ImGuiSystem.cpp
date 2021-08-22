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

		return nullptr;
	}

	bool ImGuiSystem::IsInitialized() const {
		return mImGui.get();
	}

	void ImGuiSystem::Shutdown() {

		mImGui.reset();
	}

	void ImGuiSystem::NewFrame(Frame* frame) {
	}

	void ImGuiSystem::OnAddedTo(SystemCollection& collection) {
	}
}