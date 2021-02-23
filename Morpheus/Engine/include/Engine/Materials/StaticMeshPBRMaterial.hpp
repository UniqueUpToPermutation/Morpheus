#pragma once

#include <Engine/Materials/MaterialPrototypes.hpp>

namespace Morpheus {
	void StaticMeshPBRMaterialPrototype(
		ResourceManager* manager,
		const std::string& path,
		const std::string& source,
		const nlohmann::json& config,
		const MaterialAsyncParams& params,
		MaterialResource* out);

	struct StaticMeshPBRPipelineView {
		std::vector<DG::IShaderResourceVariable*> mAlbedo;
		std::vector<DG::IShaderResourceVariable*> mNormal;
		std::vector<DG::IShaderResourceVariable*> mRoughness;
		std::vector<DG::IShaderResourceVariable*> mMetallic;

		StaticMeshPBRPipelineView(PipelineResource* pipeline);
	};
}