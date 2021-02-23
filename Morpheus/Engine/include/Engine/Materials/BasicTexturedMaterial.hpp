#pragma once

#include <Engine/Materials/MaterialPrototypes.hpp>

namespace Morpheus {
	void BasicTexturedMaterialPrototype(
		ResourceManager* manager,
		const std::string& path,
		const std::string& source,
		const nlohmann::json& config,
		const MaterialAsyncParams& params,
		MaterialResource* out);

	struct BasicTexturedPipelineView {
		std::vector<DG::IShaderResourceVariable*> mColor;
		
		BasicTexturedPipelineView(PipelineResource* pipeline);
	};
}