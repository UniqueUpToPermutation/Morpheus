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

		mColor = manager->Load<TextureResource>(path + "/" + color_src);
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

	StaticMeshPBRMaterialPrototype::StaticMeshPBRMaterialPrototype(
		const StaticMeshPBRMaterialPrototype& other) :
		mAlbedo(other.mAlbedo),
		mRoughness(other.mRoughness),
		mMetallic(other.mMetallic),
		mNormal(other.mNormal),
		mAO(other.mAO),
		mEmissive(other.mEmissive),
		mPipeline(other.mPipeline),
		mMaterialInfo(other.mMaterialInfo) {

		mPipeline->AddRef();
		mAlbedo->AddRef();
		mRoughness->AddRef();
		mMetallic->AddRef();
		mNormal->AddRef();

		if (mAO)
			mAO->AddRef();

		if (mEmissive)
			mEmissive->AddRef();
	}

	DG::IBuffer* StaticMeshPBRMaterialPrototype::CreateMaterialInfoBuffer(
		const GLTFMaterialShaderInfo& info,
		ResourceManager* manager) {

		auto dev = manager->GetParent()->GetDevice();

		DG::BufferDesc CBDesc;
		CBDesc.Name           = "MaterialShaderInfo";
		CBDesc.uiSizeInBytes  = sizeof(GLTFMaterialShaderInfo);
		CBDesc.Usage          = DG::USAGE_IMMUTABLE;
		CBDesc.BindFlags      = DG::BIND_UNIFORM_BUFFER;
		CBDesc.CPUAccessFlags = DG::CPU_ACCESS_NONE;

		DG::BufferData data;
		data.DataSize = sizeof(GLTFMaterialShaderInfo);
		data.pData = &info;

		DG::IBuffer* mBuffer = nullptr;
		dev->CreateBuffer(CBDesc, &data, &mBuffer);
		return mBuffer;
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

	void LoadPBRShaderInfo(const nlohmann::json& json, GLTFMaterialShaderInfo* result) {
		result->AlphaMaskCutoff = json.value("AlphaMaskCutoff", 0.5f);
		result->BaseColorFactor = ReadFloat4(json, "BaseColorFactor", DG::float4(1.0f, 1.0f, 1.0f, 1.0f));
		result->IBLScale = json.value("IBLScale", 1.0f);
		result->MetallicFactor = json.value("MetallicFactor", 1.0f);
		result->EmissionScale = json.value("EmissionScale", 1.0f);
		result->EmissiveFactor = ReadFloat4(json, "EmissiveFactor", DG::float4(1.0f, 1.0f, 1.0f, 1.0f));
		result->OcclusionStrength = json.value("OcclusionStrength", 1.0f);
		result->RoughnessFactor = json.value("RoughnessFactor", 1.0f);
		result->SpecularFactor = ReadFloat4(json, "SpecularFactor", DG::float4(1.0f, 1.0f, 1.0f, 1.0f));
		result->UseAlphaMask = json.value("UseAlphaMask", false);
	}

	StaticMeshPBRMaterialPrototype::StaticMeshPBRMaterialPrototype(
		ResourceManager* manager,
		const std::string& source, 
		const std::string& path,
		const nlohmann::json& config) {

		std::string pipeline_src = "PBRStaticMesh";
		std::string albedo_src = "WHITE_TEXTURE";
		std::string roughness_src = "BLACK_TEXTURE";
		std::string metallic_src = "BLACK_TEXTURE";
		std::string normal_src = "DEFAULT_NORMAL_TEXTURE";
		
		if (config.contains("Pipeline"))
			config["Pipeline"].get_to(pipeline_src);

		if (config.contains("Albedo")) {
			config["Albedo"].get_to(albedo_src);
			albedo_src = path + "/" + albedo_src;
		}
		if (config.contains("Roughness")) {
			config["Roughness"].get_to(roughness_src);
			roughness_src = path + "/" + roughness_src;
		}
		if (config.contains("Metallic")) {
			config["Metallic"].get_to(metallic_src);
			metallic_src = path + "/" + metallic_src;
		}
		if (config.contains("Normal")) {
			config["Normal"].get_to(normal_src);
			normal_src = path + "/" + normal_src;
		}

		mPipeline = manager->Load<PipelineResource>(pipeline_src);
		mAlbedo = manager->Load<TextureResource>(albedo_src);
		mRoughness = manager->Load<TextureResource>(roughness_src);
		mMetallic = manager->Load<TextureResource>(metallic_src);
		mNormal = manager->Load<TextureResource>(normal_src);

		if (config.contains("AO")) {
			std::string ao_src;
			config["AO"].get_to(ao_src);
			mAO = manager->Load<TextureResource>(path + "/" + ao_src);
		}

		if (config.contains("Emissive")) {
			std::string emissive_src;
			config["Emissive"].get_to(emissive_src);
			mEmissive = manager->Load<TextureResource>(path + "/" + emissive_src);
		}

		LoadPBRShaderInfo(config, &mMaterialInfo);
	}

	StaticMeshPBRMaterialPrototype::StaticMeshPBRMaterialPrototype(
		PipelineResource* pipeline,
		const GLTFMaterialShaderInfo& info,
		TextureResource* albedo,
		TextureResource* roughness,
		TextureResource* metallic,
		TextureResource* normal,
		TextureResource* ao,
		TextureResource* emissive) :
		mPipeline(pipeline),
		mMaterialInfo(info),
		mAlbedo(albedo),
		mRoughness(roughness),
		mMetallic(metallic),
		mNormal(normal),
		mAO(ao),
		mEmissive(emissive) {

		mPipeline->AddRef();
		mAlbedo->AddRef();
		mRoughness->AddRef();
		mMetallic->AddRef();
		mNormal->AddRef();

		if (mAO)
			mAO->AddRef();

		if (mEmissive)
			mEmissive->AddRef();
	}

	StaticMeshPBRMaterialPrototype::~StaticMeshPBRMaterialPrototype() {
		mPipeline->Release();
		mAlbedo->Release();
		mRoughness->Release();
		mMetallic->Release();
		mNormal->Release();
		if (mAO)
			mAO->Release();
		if (mEmissive)
			mEmissive->Release();
	}

	void StaticMeshPBRMaterialPrototype::InitializeMaterial(
		ResourceManager* manager,
		ResourceCache<MaterialResource>* cache,
		MaterialResource* into) {

		DG::IShaderResourceBinding* srb = nullptr;
		mPipeline->GetState()->CreateShaderResourceBinding(&srb, true);
		
		// DO STUFF
		srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "g_ColorMap")->Set(mAlbedo->GetShaderView());
		srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "g_RoughnessMap")->Set(mRoughness->GetShaderView());
		srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "g_MetallicMap")->Set(mMetallic->GetShaderView());
		srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "g_NormalMap")->Set(mNormal->GetShaderView());

		std::vector<TextureResource*> textures;
		textures.emplace_back(mAlbedo);
		textures.emplace_back(mRoughness);
		textures.emplace_back(mMetallic);
		textures.emplace_back(mNormal);

		if (mAO) {
			srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "g_AOMap")->Set(mAO->GetShaderView());
			textures.emplace_back(mAO);
		}

		if (mEmissive) {
			srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "g_EmissiveMap")->Set(mEmissive->GetShaderView());
			textures.emplace_back(mEmissive);
		}

		auto irradianceMapLoc = srb->GetVariableByName(
			DG::SHADER_TYPE_PIXEL, "g_IrradianceMap");
		auto prefilteredEnvMapLoc = srb->GetVariableByName(
			DG::SHADER_TYPE_PIXEL, "g_PrefilteredEnvMap");

		// Create image based lighting view
		cache->CreateView<ImageBasedLightingView>(into, 
			irradianceMapLoc, prefilteredEnvMapLoc);

		// Tranfers material info to the GPU
		std::vector<DG::IBuffer*> buffers;
		buffers.emplace_back(
			CreateMaterialInfoBuffer(mMaterialInfo, manager));
		srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "cbGLTFAttribs")->Set(buffers[0]);

		InternalInitialize(into, srb, mPipeline, textures, buffers);
	}

	MaterialPrototype* StaticMeshPBRMaterialPrototype::DeepCopy() const {
		return new StaticMeshPBRMaterialPrototype(*this);
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