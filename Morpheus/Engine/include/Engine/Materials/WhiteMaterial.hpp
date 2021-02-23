#pragma once

#include <Engine/Materials/MaterialPrototypes.hpp>

namespace Morpheus {
	void WhiteMaterialPrototype(
		ResourceManager* manager,
		const std::string& path,
		const std::string& source,
		const nlohmann::json& config,
		const MaterialAsyncParams& params,
		MaterialResource* out);
}