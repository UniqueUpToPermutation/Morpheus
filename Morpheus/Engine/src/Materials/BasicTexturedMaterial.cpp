#include <Engine/Materials/BasicTexturedMaterial.hpp>
#include <Engine/Materials/MaterialView.hpp>

#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/TextureResource.hpp>
#include <Engine/Resources/MaterialResource.hpp>

namespace Morpheus {
	TaskId BasicTexturedMaterialPrototype::InitializePrototype(
		ResourceManager* manager,
		const std::string& source,
		const std::string& path,
		const nlohmann::json& config,
		const MaterialAsyncParams& asyncParams) {

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

		if (asyncParams.bUseAsync) {
			auto loadColor = manager->AsyncLoadDeferred<TextureResource>(params, &mColor);
			auto loadPipeline = manager->AsyncLoadDeferred<PipelineResource>(pipeline_src, &mPipeline);

			auto queue = asyncParams.mPool->GetQueue();

			return queue.MakeTask([loadColor, loadPipeline](const TaskParams& params) {
				auto queue = params.mPool->GetQueue();

				queue.Schedule(loadColor);
				queue.Schedule(loadPipeline);
			});
		} else {
			mColor = manager->Load<TextureResource>(params);
			mPipeline = manager->Load<PipelineResource>(pipeline_src);

			return TASK_NONE;
		}
	}

	BasicTexturedMaterialPrototype::BasicTexturedMaterialPrototype(
		PipelineResource* pipeline,
		TextureResource* color) {
		mColor = color;
		mColor->AddRef();

		mPipeline = pipeline;
		mPipeline->AddRef();
	}

	BasicTexturedMaterialPrototype::~BasicTexturedMaterialPrototype() {
		mPipeline->Release();
		mColor->Release();
	}

	void BasicTexturedMaterialPrototype::InitializeMaterial(
		DG::IRenderDevice* device,
		MaterialResource* into) {

		DG::IShaderResourceBinding* srb = nullptr;
		mPipeline->GetState()->CreateShaderResourceBinding(&srb, true);
		
		srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mTexture")->Set(mColor->GetShaderView());

		std::vector<TextureResource*> textures;
		textures.emplace_back(mColor);
		std::vector<DG::IBuffer*> bufs;

		InternalInitialize(into, srb, mPipeline, textures, bufs);
	}

	void BasicTexturedMaterialPrototype::ScheduleLoadBefore(TaskNodeDependencies dependencies) {
		dependencies
			.After(mPipeline->GetLoadBarrier())
			.After(mColor->GetLoadBarrier());
	}

	BasicTexturedMaterialPrototype::BasicTexturedMaterialPrototype(const BasicTexturedMaterialPrototype& other) : 
		mPipeline(other.mPipeline),
		mColor(other.mColor) {
		mPipeline->AddRef();
		mColor->AddRef();
	}

	MaterialPrototype* BasicTexturedMaterialPrototype::DeepCopy() const {
		return new BasicTexturedMaterialPrototype(*this);
	}
}