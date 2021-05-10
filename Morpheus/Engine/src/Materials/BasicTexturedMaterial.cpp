#include <Engine/Materials/BasicTexturedMaterial.hpp>

#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/TextureResource.hpp>
#include <Engine/Resources/MaterialResource.hpp>

namespace Morpheus {

	BasicTexturedPipelineView::BasicTexturedPipelineView(PipelineResource* pipeline) {
		auto& bindings = pipeline->GetShaderResourceBindings();
		mColor.reserve(bindings.size());

		for (auto binding : bindings) {
			mColor.emplace_back(binding->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mTexture"));
		}
	}

	Task BasicTexturedMaterialPrototype(
		ResourceManager* manager,
		const std::string& path,
		const std::string& source,
		const nlohmann::json& config,
		MaterialResource* out) {

		struct Data {
			PipelineResource* mPipeline = nullptr;
			TextureResource* mColor = nullptr;

			~Data() {
				if (mPipeline)
					mPipeline->Release();
				if (mColor)
					mColor->Release();
			}
		};

		Task task([manager, path, source, config, out, data = Data()](const TaskParams& e) mutable {

			if (e.mTask->SubTask()) {
				std::string color_src = config.value("Color", "WHITE_TEXTURE");
				std::string pipeline_src = config.value("Pipeline", "BasicTextured");

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

				if (config.contains("Pipeline"))
					config["Pipeline"].get_to(pipeline_src);

				if (config.contains("Color")) {
					config["Color"].get_to(color_src);
					if (usePath(color_src))
						color_src = path + "/" + color_src;
				}

				LoadParams<TextureResource> params;
				params.mSource = color_src;
				params.bIsSRGB = true; // Gamma correct albedo!

				e.mQueue->AdoptAndTrigger(manager->LoadTask<TextureResource>(params, &data.mColor));
				e.mQueue->AdoptAndTrigger(manager->LoadTask<PipelineResource>(pipeline_src, &data.mPipeline));

				if (e.mTask->In().Lock()
					.Connect(&data.mColor->GetLoadBarrier()->mOut)
					.Connect(&data.mPipeline->GetLoadBarrier()->mOut)
					.ShouldWait())
					return TaskResult::WAITING;
			}

			if (e.mTask->SubTask()) {
				std::vector<TextureResource*> textures = { data.mColor };
				std::vector<DG::IBuffer*> buffers;

				out->Initialize(data.mPipeline, textures, buffers, 
					[color = data.mColor](PipelineResource* pipeline, 
						MaterialResource* material, uint srbId) {

					auto& view = pipeline->GetView<BasicTexturedPipelineView>();
					view.mColor[srbId]->Set(color->GetShaderView());
				});
			}

			return TaskResult::FINISHED;
		}, 
		"Upload Basic Textured Material",
		TaskType::UNSPECIFIED,
		ASSIGN_THREAD_MAIN);

		return task;
	}
}