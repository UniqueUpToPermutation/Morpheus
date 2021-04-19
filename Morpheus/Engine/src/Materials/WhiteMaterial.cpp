#include <Engine/Materials/WhiteMaterial.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/MaterialResource.hpp>

namespace Morpheus {

	Task WhiteMaterialPrototype(
		ResourceManager* manager,
		const std::string& path,
		const std::string& source,
		const nlohmann::json& config,
		MaterialResource* out) {

		Task task;
		task.mType = TaskType::RENDER;
		task.mSyncPoint = out->GetLoadBarrier();
		task.mFunc = [manager, path, source, config, out](const TaskParams& e) {
			std::string pipeline_src = config.value("Pipeline", "White");

			auto usePath = [](const std::string& s) {
				if (s == "WHITE_TEXTURE")
					return false;
				else if (s == "BLACK_TEXTURE")
					return false;
				else if (s == "DEFAULT_NORMAL_TEXTURE")
					return false;
				else 
					return true;
			};

			PipelineResource* pipeline = nullptr;
			e.mQueue->Submit(manager->LoadTask<PipelineResource>(pipeline_src, &pipeline));
			e.mQueue->YieldUntil(pipeline->GetLoadBarrier());

			std::vector<TextureResource*> textures;
			std::vector<DG::IBuffer*> buffers;

			out->Initialize(pipeline, textures, buffers, 
				[](PipelineResource*, MaterialResource*, uint) {
			});

			pipeline->Release();
		};

		return task;
	}
}