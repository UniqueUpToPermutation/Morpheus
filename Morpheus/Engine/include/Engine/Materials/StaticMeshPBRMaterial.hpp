#pragma once

#include <Engine/MaterialPrototypes.hpp>

namespace Morpheus {
	class StaticMeshPBRMaterialPrototype : public MaterialPrototype {
	private:
		PipelineResource* mPipeline = nullptr;
		TextureResource* mAlbedo = nullptr;
		TextureResource* mNormal = nullptr;
		TextureResource* mRoughness = nullptr;
		TextureResource* mMetallic = nullptr;

	public:
		StaticMeshPBRMaterialPrototype(
			const StaticMeshPBRMaterialPrototype& other);
		StaticMeshPBRMaterialPrototype(
			ResourceManager* manager,
			const std::string& source, 
			const std::string& path,
			const nlohmann::json& config);
		StaticMeshPBRMaterialPrototype(
			PipelineResource* pipeline,
			TextureResource* albedo,
			TextureResource* normal,
			TextureResource* metallic,
			TextureResource* roughness);
		~StaticMeshPBRMaterialPrototype();

		void InitializeMaterial(
			ResourceManager* manager,
			ResourceCache<MaterialResource>* cache,
			MaterialResource* into) override;

		MaterialPrototype* DeepCopy() const override;
	};
}