#pragma once

#include <Engine/Materials/MaterialPrototypes.hpp>

namespace Morpheus {
	class JsonMaterialPrototype : public MaterialPrototype {
	private:
		PipelineResource* mPipeline;
		std::vector<DG::Uint32> mVariableIndices;
		std::vector<TextureResource*> mTextures;

	public:
		JsonMaterialPrototype(const JsonMaterialPrototype& other);
		JsonMaterialPrototype();
		~JsonMaterialPrototype();

		void ScheduleLoadBefore(TaskNodeDependencies dependencies) override;

		TaskId InitializePrototype(
			ResourceManager* manager,
			const std::string& source,
			const std::string& path,
			const nlohmann::json& config,
			const MaterialAsyncParams& asyncParams) override;

		void InitializeMaterial(
			DG::IRenderDevice* device,
			MaterialResource* into) override;
		MaterialPrototype* DeepCopy() const override;
	};
}