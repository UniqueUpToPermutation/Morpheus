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

	void StaticMeshPBRMaterialPrototype(
		ResourceManager* manager,
		const std::string& path,
		const std::string& source,
		const nlohmann::json& config,
		const MaterialAsyncParams& asyncParams,
		MaterialResource* out) {

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

		if (asyncParams.bUseAsync) {
			pipeline 	= manager->AsyncLoad<PipelineResource>(pipeline_src);
			albedo 		= manager->AsyncLoad<TextureResource>(albedo_params);
			roughness 	= manager->AsyncLoad<TextureResource>(roughness_src);
			normal 		= manager->AsyncLoad<TextureResource>(normal_src);
			metallic 	= manager->AsyncLoad<TextureResource>(metallic_src);

			auto queue = asyncParams.mPool->GetQueue();
			auto triggerBarrier = queue.MakeTask([](const TaskParams&) { },
				out->GetLoadBarrier());

			queue.Dependencies(triggerBarrier)
				.After(pipeline->GetLoadBarrier())
				.After(albedo->GetLoadBarrier())
				.After(roughness->GetLoadBarrier())
				.After(normal->GetLoadBarrier())
				.After(metallic->GetLoadBarrier());

			queue.Schedule(triggerBarrier);
		} else {
			pipeline 	= manager->Load<PipelineResource>(pipeline_src);
			albedo 		= manager->Load<TextureResource>(albedo_params);
			roughness 	= manager->Load<TextureResource>(roughness_src);
			normal 		= manager->Load<TextureResource>(normal_src);
			metallic 	= manager->Load<TextureResource>(metallic_src);
		}

		std::vector<TextureResource*> textures = {
			albedo, normal, roughness, metallic
		};
		std::vector<DG::IBuffer*> buffers;

		out->Initialize(pipeline, textures, buffers, 
			[albedo, normal, roughness, metallic](PipelineResource* pipeline, 
			MaterialResource* material, uint srbId) {
			auto& view = pipeline->GetView<StaticMeshPBRPipelineView>();

			view.mAlbedo[srbId]->Set(albedo->GetShaderView());
			view.mNormal[srbId]->Set(normal->GetShaderView());
			view.mRoughness[srbId]->Set(roughness->GetShaderView());
			view.mMetallic[srbId]->Set(metallic->GetShaderView());
		});

		for (auto tex : textures)
			tex->Release();
		pipeline->Release();
	}
}