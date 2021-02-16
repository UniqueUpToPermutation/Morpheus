#pragma once

#include <Engine/Materials/MaterialPrototypes.hpp>

namespace Morpheus {
	class BasicTexturedMaterialPrototype : public MaterialPrototype {
	private:
		TextureResource* mColor = nullptr;
		PipelineResource* mPipeline = nullptr;

	public:
		BasicTexturedMaterialPrototype(
			const BasicTexturedMaterialPrototype& other);
		inline BasicTexturedMaterialPrototype() {
		}
		BasicTexturedMaterialPrototype(
			PipelineResource* pipeline,
			TextureResource* color);

		~BasicTexturedMaterialPrototype();

		void InitializeMaterial(
			DG::IRenderDevice* device,
			MaterialResource* into) override;
		TaskId InitializePrototype(
			ResourceManager* manager,
			const std::string& source,
			const std::string& path,
			const nlohmann::json& config,
			const MaterialAsyncParams& asyncParams) override;
		MaterialPrototype* DeepCopy() const override;

		void ScheduleLoadBefore(TaskNodeDependencies dependencies) override;
	};
}