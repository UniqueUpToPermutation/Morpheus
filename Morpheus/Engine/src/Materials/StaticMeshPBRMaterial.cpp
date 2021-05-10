#include <Engine/Materials/StaticMeshPBRMaterial.hpp>

#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/TextureResource.hpp>
#include <Engine/Resources/MaterialResource.hpp>

namespace Morpheus {

	StaticMeshPBRPipelineView::StaticMeshPBRPipelineView(PipelineResource* pipeline) {
		auto& srb = pipeline->GetShaderResourceBindings();

		mAlbedo.reserve(srb.size());
		mNormal.reserve(srb.size());
		mRoughness.reserve(srb.size());
		mMetallic.reserve(srb.size());

		for (auto binding : srb) {
			mAlbedo.emplace_back(binding->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mAlbedo"));
			mMetallic.emplace_back(binding->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mMetallic"));
			mRoughness.emplace_back(binding->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mRoughness"));
			mNormal.emplace_back(binding->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mNormalMap"));
		}
	}

	Task StaticMeshPBRMaterialPrototype(
		ResourceManager* manager,
		const std::string& path,
		const std::string& source,
		const nlohmann::json& config,
		MaterialResource* out) {

		struct Data {
			PipelineResource* mPipeline = nullptr;
			TextureResource* mRoughness = nullptr;
			TextureResource* mAlbedo = nullptr;
			TextureResource* mNormal = nullptr;
			TextureResource* mMetallic = nullptr;

			~Data() {
				if (mPipeline)
					mPipeline->Release();
				if (mRoughness)
					mRoughness->Release();
				if (mAlbedo)
					mAlbedo->Release();
				if (mNormal)
					mNormal->Release();
				if (mMetallic)
					mMetallic->Release();
			}
		};

		Task task([manager, path, source, config, out, data = Data()](const TaskParams& e) mutable {

			if (e.mTask->SubTask()) {
				std::string pipeline_src = 	"PBRStaticMesh";
				std::string albedo_src = 	"WHITE_TEXTURE";
				std::string normal_src = 	"DEFAULT_NORMAL_TEXTURE";
				std::string roughness_src =	"BLACK_TEXTURE";
				std::string metallic_src = 	"BLACK_TEXTURE";

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

				if (config.contains("Albedo")) {
					config["Albedo"].get_to(albedo_src);
					if (usePath(albedo_src))
						albedo_src = path + "/" + albedo_src;
				}

				if (config.contains("Metallic")) {
					config["Metallic"].get_to(metallic_src);
					if (usePath(metallic_src))
						metallic_src = path + "/" + metallic_src;
				}

				if (config.contains("Roughness")) {
					config["Roughness"].get_to(roughness_src);
					if (usePath(roughness_src))
						roughness_src = path + "/" + roughness_src;
				}

				if (config.contains("Normal")) {
					config["Normal"].get_to(normal_src);
					if (usePath(normal_src))
						normal_src = path + "/" + normal_src;
				}

				LoadParams<TextureResource> albedo_params;
				albedo_params.mSource = albedo_src;
				albedo_params.bIsSRGB = true; // Gamma correct albedo!

				PipelineResource* pipeline;
				TextureResource* roughness;
				TextureResource* albedo;
				TextureResource* normal;
				TextureResource* metallic;

				e.mQueue->AdoptAndTrigger(manager->LoadTask<PipelineResource>(pipeline_src, &data.mPipeline));
				e.mQueue->AdoptAndTrigger(manager->LoadTask<TextureResource>(albedo_params, &data.mAlbedo));
				e.mQueue->AdoptAndTrigger(manager->LoadTask<TextureResource>(roughness_src, &data.mRoughness));
				e.mQueue->AdoptAndTrigger(manager->LoadTask<TextureResource>(normal_src, &data.mNormal));
				e.mQueue->AdoptAndTrigger(manager->LoadTask<TextureResource>(metallic_src, &data.mMetallic));

				if (e.mTask->In().Lock()
					.Connect(&data.mPipeline->GetLoadBarrier()->mOut)
					.Connect(&data.mRoughness->GetLoadBarrier()->mOut)
					.Connect(&data.mAlbedo->GetLoadBarrier()->mOut)
					.Connect(&data.mNormal->GetLoadBarrier()->mOut)
					.Connect(&data.mMetallic->GetLoadBarrier()->mOut)
					.ShouldWait())
					return TaskResult::WAITING;
			}

			if (e.mTask->SubTask()) {
				std::vector<TextureResource*> textures = {
					data.mAlbedo, data.mNormal, data.mRoughness, data.mMetallic
				};
				std::vector<DG::IBuffer*> buffers;

				out->Initialize(data.mPipeline, textures, buffers, 
					[albedo = data.mAlbedo, 
					normal = data.mNormal, 
					roughness = data.mRoughness, 
					metallic = data.mMetallic](PipelineResource* pipeline, 
					MaterialResource* material, uint srbId) {
					auto& view = pipeline->GetView<StaticMeshPBRPipelineView>();

					view.mAlbedo[srbId]->Set(albedo->GetShaderView());
					view.mNormal[srbId]->Set(normal->GetShaderView());
					view.mRoughness[srbId]->Set(roughness->GetShaderView());
					view.mMetallic[srbId]->Set(metallic->GetShaderView());
				}); 
			}
			return TaskResult::FINISHED;
		},
		"Upload Static Mesh PBR Material",
		TaskType::UNSPECIFIED,
		ASSIGN_THREAD_MAIN);

		return task;
	}
}