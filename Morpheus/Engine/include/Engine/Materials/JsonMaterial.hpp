#pragma once

#include <Engine/Materials/MaterialPrototypes.hpp>

namespace Morpheus {
	class JsonMaterialPrototype : public MaterialPrototype {
	private:
	PipelineResource* mPipeline;
		std::vector<DG::Uint32> mVariableIndices;
		std::vector<TextureResource*> mTextures;

	public:
		JsonMaterialPrototype(
			const JsonMaterialPrototype& other);
		JsonMaterialPrototype(
			ResourceManager* manager,
			const std::string& source, 
			const std::string& path,
			const nlohmann::json& config);
		~JsonMaterialPrototype();

		void InitializeMaterial(
			ResourceManager* manager,
			ResourceCache<MaterialResource>* cache,
			MaterialResource* into) override;
		MaterialPrototype* DeepCopy() const override;
	};
}