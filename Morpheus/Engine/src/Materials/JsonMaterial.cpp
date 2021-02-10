#include <Engine/Materials/JsonMaterial.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/TextureResource.hpp>

namespace Morpheus {

	JsonMaterialPrototype::JsonMaterialPrototype(
		const JsonMaterialPrototype& other) : 
		mTextures(other.mTextures),
		mVariableIndices(other.mVariableIndices),
		mPipeline(other.mPipeline) {

		for (auto tex : mTextures)
			tex->AddRef();
	
		mPipeline->AddRef();
	}

	JsonMaterialPrototype::JsonMaterialPrototype() :
		mPipeline(nullptr) {
	}

	JsonMaterialPrototype::~JsonMaterialPrototype() {
		for (auto tex : mTextures)
			tex->Release();
		mPipeline->Release();
	}

	void JsonMaterialPrototype::InitializeMaterial(
		DG::IRenderDevice* device,
		MaterialResource* into) {

		DG::IShaderResourceBinding* srb = nullptr;
		mPipeline->GetState()->CreateShaderResourceBinding(&srb, true);
		
		for (uint i = 0; i < mTextures.size(); ++i) {
			srb->GetVariableByIndex(
				DG::SHADER_TYPE_PIXEL, mVariableIndices[i])->Set(
					mTextures[i]->GetTexture());
		}

		std::vector<DG::IBuffer*> bufs;
		InternalInitialize(into, srb, mPipeline, mTextures, bufs);
	}

	TaskId JsonMaterialPrototype::InitializePrototype(
		ResourceManager* manager,
		const std::string& source,
		const std::string& path,
		const nlohmann::json& config,
		const MaterialAsyncParams& asyncParams) {

		std::string pipeline_str;
		config["Pipeline"].get_to(pipeline_str);

		mPipeline = manager->Load<PipelineResource>(pipeline_str);

		std::vector<TaskId> asyncTasks;
		std::vector<TextureResource*> textures;
		if (config.contains("Textures")) {
			for (const auto& item : config["Textures"]) {

				std::string binding_loc;
				config["Binding"].get_to(binding_loc);

				DG::SHADER_TYPE shader_type = ReadShaderType(config["ShaderType"]);

				std::string source;
				config["Source"].get_to(source);

				TextureResource* texture = nullptr;
				if (asyncParams.bUseAsync) {
					asyncTasks.emplace_back(manager->AsyncLoadDeferred<TextureResource>(source, &texture));
				} else {
					texture = manager->Load<TextureResource>(source);
				}
				mTextures.emplace_back(texture);

				auto variable = mPipeline->GetState()->GetStaticVariableByName(shader_type, binding_loc.c_str());

				if (variable) {
					mVariableIndices.emplace_back(variable->GetIndex());
				} else {
					std::cout << "Warning could not find binding " << binding_loc << "!" << std::endl;
				}
			}
		}

		if (asyncTasks.size() > 0) {
			auto queue = asyncParams.mPool->GetQueue();

			return queue.MakeTask([asyncTasks](const TaskParams& params) {
				auto queue = params.mPool->GetQueue();
				for (auto task : asyncTasks) {
					queue.Schedule(task);
				}
			});
		} else {
			return TASK_NONE;
		}
	}

	void JsonMaterialPrototype::ScheduleLoadBefore(TaskNodeDependencies dependencies) {
		for (auto texture : mTextures) {
			dependencies.After(texture->GetLoadBarrier());
		}
	}

	MaterialPrototype* JsonMaterialPrototype::DeepCopy() const {
		return new JsonMaterialPrototype(*this);
	}
}