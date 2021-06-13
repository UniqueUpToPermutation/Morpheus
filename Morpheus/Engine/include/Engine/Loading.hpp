#pragma once

#include <vector>

#include <Engine/Systems/ImGuiSystem.hpp>

namespace Morpheus {
	
	class TaskBarrier;
	class ITaskQueue;
	class SystemCollection;
	class Graphics;
	class Platform;

	// Presents a loading screen until all resources are loaded
	void BasicLoadingScreen(Platform& platform, Graphics& graphics,
		DG::ImGuiImplDiligent* imgui, TaskBarrier* barrier, ITaskQueue* queue);
}