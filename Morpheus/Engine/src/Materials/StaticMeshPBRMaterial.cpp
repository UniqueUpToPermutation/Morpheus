#include <Engine/Materials/StaticMeshPBRMaterial.hpp>
#include <Engine/Materials/MaterialView.hpp>

#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/TextureResource.hpp>
#include <Engine/Resources/MaterialResource.hpp>

namespace Morpheus {

	StaticMeshPBRMaterialPrototype::StaticMeshPBRMaterialPrototype(
		const StaticMeshPBRMaterialPrototype& other) :
		mPipeline(other.mPipeline),
		mAlbedo(other.mAlbedo),
		mRoughness(other.mRoughness),
		mNormal(other.mNormal),
		mMetallic(other.mMetallic) {
		mPipeline->AddRef();
		mAlbedo->AddRef();
		mRoughness->AddRef();
		mNormal->AddRef();
		mMetallic->AddRef();
	}

	StaticMeshPBRMaterialPrototype::StaticMeshPBRMaterialPrototype(
		ResourceManager* manager,
		const std::string& source, 
		const std::string& path,
		const nlohmann::json& config) {
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

		mPipeline = 	manager->Load<PipelineResource>(pipeline_src);

		LoadParams<TextureResource> albedo_params;
		albedo_params.mSource = albedo_src;
		albedo_params.bIsSRGB = true; // Gamma correct albedo!

		mAlbedo = 		manager->Load<TextureResource>(albedo_params);
		mRoughness =	manager->Load<TextureResource>(roughness_src);
		mNormal =		manager->Load<TextureResource>(normal_src);
		mMetallic = 	manager->Load<TextureResource>(metallic_src);
	}

	StaticMeshPBRMaterialPrototype::StaticMeshPBRMaterialPrototype(
		PipelineResource* pipeline,
		TextureResource* albedo,
		TextureResource* normal,
		TextureResource* metallic,
		TextureResource* roughness):
		mPipeline(pipeline),
		mAlbedo(albedo),
		mNormal(normal),
		mMetallic(metallic),
		mRoughness(roughness)
	{
		mPipeline->AddRef();
		mAlbedo->AddRef();
		mNormal->AddRef();
		mMetallic->AddRef();
		mRoughness->AddRef();
	}

	StaticMeshPBRMaterialPrototype::~StaticMeshPBRMaterialPrototype() {
		mPipeline->Release();
		mAlbedo->Release();
		mNormal->Release();
		mMetallic->Release();
		mRoughness->Release();
	}

	void StaticMeshPBRMaterialPrototype::InitializeMaterial(
		ResourceManager* manager,
		ResourceCache<MaterialResource>* cache,
		MaterialResource* into) {
		DG::IShaderResourceBinding* srb = nullptr;
		mPipeline->GetState()->CreateShaderResourceBinding(&srb, true);
		
		auto albedoVar = srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mAlbedo");
		if (albedoVar)
			albedoVar->Set(mAlbedo->GetShaderView());
		auto metallicVar = srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mMetallic");
		if (metallicVar)
			metallicVar->Set(mMetallic->GetShaderView());
		auto roughnessVar = srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mRoughness");
		if (roughnessVar)
			roughnessVar->Set(mRoughness->GetShaderView());
		auto normalVar = srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mNormalMap");
		if (normalVar)
			normalVar->Set(mNormal->GetShaderView());

		std::vector<TextureResource*> textures;
		textures.emplace_back(mAlbedo);
		textures.emplace_back(mMetallic);
		textures.emplace_back(mRoughness);
		textures.emplace_back(mNormal);

		auto irradianceMapLoc = srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mIrradianceMap");
		auto irradianceSHLoc = srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "IrradianceSH");
		auto prefilteredEnvMapLoc = srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mPrefilteredEnvMap");

		// Create image based lighting view
		cache->CreateView<ImageBasedLightingView>(into, 
			irradianceMapLoc, irradianceSHLoc, prefilteredEnvMapLoc);

		std::vector<DG::IBuffer*> buffers;
		InternalInitialize(into, srb, mPipeline, textures, buffers);
	}
		
	MaterialPrototype* StaticMeshPBRMaterialPrototype::DeepCopy() const {
		return new StaticMeshPBRMaterialPrototype(*this);
	}
}