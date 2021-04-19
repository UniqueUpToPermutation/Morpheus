#pragma once

#include <Engine/Materials/MaterialPrototypes.hpp>

namespace Morpheus {
	Task WhiteMaterialPrototype(
		ResourceManager* manager,
		const std::string& path,
		const std::string& source,
		const nlohmann::json& config,
		MaterialResource* out);
}