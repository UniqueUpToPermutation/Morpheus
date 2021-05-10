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

		struct Data {
			PipelineResource* mPipeline = nullptr;

			~Data() {
				if (mPipeline)
					mPipeline->Release();
			}
		};

		Task task([manager, path, source, config, out, data = Data()](const TaskParams& e) mutable {

			if (e.mTask->SubTask()) {
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

				e.mQueue->AdoptAndTrigger(manager->LoadTask<PipelineResource>(pipeline_src, &data.mPipeline));

				if (e.mTask->In().Lock().Connect(&data.mPipeline->GetLoadBarrier()->mOut).ShouldWait())
					return TaskResult::WAITING;
			}

			if (e.mTask->SubTask()) {
				std::vector<TextureResource*> textures;
				std::vector<DG::IBuffer*> buffers;

				out->Initialize(data.mPipeline, textures, buffers, 
					[](PipelineResource*, MaterialResource*, uint) {
				});
			}

			return TaskResult::FINISHED;
		}, 
		"Upload White Material", 
		TaskType::UNSPECIFIED, 
		ASSIGN_THREAD_MAIN);

		return task;
	}
}