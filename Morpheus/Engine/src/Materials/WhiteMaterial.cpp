#include <Engine/Materials/WhiteMaterial.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/MaterialResource.hpp>

namespace Morpheus {

	void WhiteMaterialPrototype(
		ResourceManager* manager,
		const std::string& path,
		const std::string& source,
		const nlohmann::json& config,
		const MaterialAsyncParams& asyncParams,
		MaterialResource* out) {

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

		PipelineResource* pipeline;

		if (asyncParams.bUseAsync) {
			pipeline = manager->AsyncLoad<PipelineResource>(pipeline_src);

			auto queue = asyncParams.mPool->GetQueue();
			auto triggerBarrier = queue.MakeTask([](const TaskParams&) { },
				out->GetLoadBarrier());

			queue.Dependencies(triggerBarrier)
				.After(pipeline->GetLoadBarrier());

			queue.Schedule(triggerBarrier);
		} else {
			pipeline = manager->Load<PipelineResource>(pipeline_src);
		}

		std::vector<TextureResource*> textures;
		std::vector<DG::IBuffer*> buffers;

		out->Initialize(pipeline, textures, buffers, 
			[](PipelineResource*, MaterialResource*, uint) {
		});

		pipeline->Release();
	}
}