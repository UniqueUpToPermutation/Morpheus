#pragma once

#include <Engine/Materials/MaterialPrototypes.hpp>

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
		inline StaticMeshPBRMaterialPrototype() {
		}
		StaticMeshPBRMaterialPrototype(
			PipelineResource* pipeline,
			TextureResource* albedo,
			TextureResource* normal,
			TextureResource* metallic,
			TextureResource* roughness);
		~StaticMeshPBRMaterialPrototype();

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