#pragma once

#include <Engine/Materials/MaterialPrototypes.hpp>

namespace Morpheus {
	class BasicTexturedMaterialPrototype : public MaterialPrototype {
	private:
		TextureResource* mColor;
		PipelineResource* mPipeline;

	public:
		BasicTexturedMaterialPrototype(
			const BasicTexturedMaterialPrototype& other);

		BasicTexturedMaterialPrototype(
			ResourceManager* manager,
			const std::string& source, 
			const std::string& path,
			const nlohmann::json& config);
		BasicTexturedMaterialPrototype(
			PipelineResource* pipeline,
			TextureResource* color);
		~BasicTexturedMaterialPrototype();

		void InitializeMaterial(
			ResourceManager* manager,
			ResourceCache<MaterialResource>* cache,
			MaterialResource* into) override;
		MaterialPrototype* DeepCopy() const override;
	};
}