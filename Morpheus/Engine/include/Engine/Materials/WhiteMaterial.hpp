#pragma once

#include <Engine/Materials/MaterialPrototypes.hpp>

namespace Morpheus {
	class WhiteMaterialPrototype : public MaterialPrototype {
	private:
		PipelineResource* mPipeline = nullptr;

	public:
		WhiteMaterialPrototype(
			const WhiteMaterialPrototype& other);
		inline WhiteMaterialPrototype() {
		}
		WhiteMaterialPrototype(
			PipelineResource* pipeline);
		~WhiteMaterialPrototype();

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

		void ScheduleLoadBefore(TaskNodeDependencies dependencies) override;
	};
}