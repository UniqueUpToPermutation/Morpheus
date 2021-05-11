#pragma once

#include <vector>

namespace Morpheus {
	class Engine;
	class IResource;

	// Presents a loading screen until all resources are loaded
	void LoadingScreen(Engine* en, const std::vector<IResource*>& resources);
}