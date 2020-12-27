#include <Engine/MaterialPrototypes.hpp>
#include <Engine/TextureResource.hpp>
#include <Engine/ResourceManager.hpp>
#include <Engine/PipelineResource.hpp>
#include <Engine/MaterialResource.hpp>
#include <Engine/MaterialView.hpp>

#include "MapHelper.hpp"

namespace Morpheus {

	MaterialPrototypeFactory::MaterialPrototypeFactory() {
		mMap["BasicTexturedMaterial"] = 
			&AbstractConstructor<BasicTexturedMaterialPrototype>;
		mMap["StaticMeshPBRMaterial"] = 
			&AbstractConstructor<StaticMeshPBRMaterialPrototype>;
	}

	MaterialPrototype* MaterialPrototypeFactory::Spawn(
		const std::string& type,
		ResourceManager* manager,
		const std::string& source, 
		const std::string& path,
		const nlohmann::json& config) const {

		auto it = mMap.find(type);
		if (it != mMap.end()) {
			return it->second(manager, source, path, config);
		}
		return nullptr;
	}

	void MaterialPrototype::InternalInitialize(
		MaterialResource* material,
		DG::IShaderResourceBinding* binding, 
		PipelineResource* pipeline,
		const std::vector<TextureResource*>& textures,
		const std::vector<DG::IBuffer*>& buffers) {

		material->Init(binding, pipeline, textures, buffers, "");
		material->mPrototype.reset(DeepCopy());
	}

	BasicTexturedMaterialPrototype::BasicTexturedMaterialPrototype(
		ResourceManager* manager,
		const std::string& source, 
		const std::string& path,
		const nlohmann::json& config) {

		std::string color_src;
		std::string pipeline_src;

		config["Color"].get_to(color_src);
		config["Pipeline"].get_to(pipeline_src);

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

		mColor = manager->Load<TextureResource>(params);
		mPipeline = manager->Load<PipelineResource>(path + "/" + pipeline_src);
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
		ResourceManager* manager,
		ResourceCache<MaterialResource>* cache,
		MaterialResource* into) {

		DG::IShaderResourceBinding* srb = nullptr;
		mPipeline->GetState()->CreateShaderResourceBinding(&srb, true);
		
		srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mTexture")->Set(mColor->GetTexture());

		std::vector<TextureResource*> textures;
		textures.emplace_back(mColor);
		std::vector<DG::IBuffer*> bufs;

		InternalInitialize(into, srb, mPipeline, textures, bufs);
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
	
	DG::float4 ReadFloat4(const nlohmann::json& json, const std::string& name, const DG::float4& defaultValue) {
		if (json.contains(name)) {
			std::vector<float> fvec;
			json[name].get_to(fvec);
			assert(fvec.size() == 4);
			return DG::float4(fvec[0], fvec[1], fvec[2], fvec[3]);
		} else {
			return defaultValue;
		}
	}

	JsonMaterialPrototype::JsonMaterialPrototype(
		const JsonMaterialPrototype& other) : 
		mTextures(other.mTextures),
		mVariableIndices(other.mVariableIndices),
		mPipeline(other.mPipeline) {

		for (auto tex : mTextures)
			tex->AddRef();
	
		mPipeline->AddRef();
	}

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

	JsonMaterialPrototype::JsonMaterialPrototype(
		ResourceManager* manager,
		const std::string& source, 
		const std::string& path,
		const nlohmann::json& config) {

		std::string pipeline_str;
		config["Pipeline"].get_to(pipeline_str);

		mPipeline = manager->Load<PipelineResource>(pipeline_str);

		std::vector<TextureResource*> textures;
		if (config.contains("Textures")) {
			for (const auto& item : config["Textures"]) {

				std::string binding_loc;
				config["Binding"].get_to(binding_loc);

				DG::SHADER_TYPE shader_type = ReadShaderType(config["ShaderType"]);

				std::string source;
				config["Source"].get_to(source);

				auto texture = manager->Load<TextureResource>(source);
				mTextures.emplace_back(texture);

				auto variable = mPipeline->GetState()->GetStaticVariableByName(shader_type, binding_loc.c_str());

				if (variable) {
					mVariableIndices.emplace_back(variable->GetIndex());
				} else {
					std::cout << "Warning could not find binding " << binding_loc << "!" << std::endl;
				}
			}
		}
	}

	JsonMaterialPrototype::~JsonMaterialPrototype() {
		for (auto tex : mTextures)
			tex->Release();
		mPipeline->Release();
	}

	void JsonMaterialPrototype::InitializeMaterial(
		ResourceManager* manager,
		ResourceCache<MaterialResource>* cache,
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

	MaterialPrototype* JsonMaterialPrototype::DeepCopy() const {
		return new JsonMaterialPrototype(*this);
	}
}