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