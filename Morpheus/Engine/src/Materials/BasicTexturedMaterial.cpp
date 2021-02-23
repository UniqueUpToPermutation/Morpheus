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

	void BasicTexturedMaterialPrototype(
		ResourceManager* manager,
		const std::string& path,
		const std::string& source,
		const nlohmann::json& config,
		const MaterialAsyncParams& asyncParams,
		MaterialResource* out) {

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

		TextureResource* color;
		PipelineResource* pipeline;

		if (asyncParams.bUseAsync) {
			color = manager->AsyncLoad<TextureResource>(params);
			pipeline = manager->AsyncLoad<PipelineResource>(pipeline_src);

			auto queue = asyncParams.mPool->GetQueue();
			auto triggerBarrier = queue.MakeTask(
				[](const TaskParams&) { }, out->GetLoadBarrier());
			queue.Dependencies(triggerBarrier)
				.After(color->GetLoadBarrier())
				.After(pipeline->GetLoadBarrier());
			queue.Schedule(triggerBarrier);

		} else {
			color = manager->Load<TextureResource>(params);
			pipeline = manager->Load<PipelineResource>(pipeline_src);
		}

		std::vector<TextureResource*> textures = { color };
		std::vector<DG::IBuffer*> buffers;

		out->Initialize(pipeline, textures, buffers, 
			[color](PipelineResource* pipeline, 
				MaterialResource* material, uint srbId) {

			auto& view = pipeline->GetView<BasicTexturedPipelineView>();
			view.mColor[srbId]->Set(color->GetShaderView());
		});

		for (auto tex : textures)
			tex->Release();
		pipeline->Release();
	}
}