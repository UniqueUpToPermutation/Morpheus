#pragma once

#include <Engine/MaterialPrototypes.hpp>

namespace Morpheus {
	class WhiteMaterialPrototype : public MaterialPrototype {
	private:
		PipelineResource* mPipeline;

	public:
		WhiteMaterialPrototype(
			const WhiteMaterialPrototype& other);

		WhiteMaterialPrototype(
			ResourceManager* manager,
			const std::string& source, 
			const std::string& path,
			const nlohmann::json& config);
		WhiteMaterialPrototype(
			PipelineResource* pipeline);
		~WhiteMaterialPrototype();

		void InitializeMaterial(
			ResourceManager* manager,
			ResourceCache<MaterialResource>* cache,
			MaterialResource* into) override;
		MaterialPrototype* DeepCopy() const override;
	};
}