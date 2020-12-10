#include <Engine/MaterialPrototypes.hpp>
#include <Engine/TextureResource.hpp>
#include <Engine/ResourceManager.hpp>
#include <Engine/PipelineResource.hpp>
#include <Engine/MaterialResource.hpp>

namespace Morpheus {

	MaterialPrototypeFactory::MaterialPrototypeFactory() {
		mMap["BasicTexturedMaterial"] = 
			&AbstractConstructor<BasicTexturedMaterialPrototype>;
		/*mMap["StaticMeshPBRMaterial"] = 
			&AbstractConstructor<StaticMeshPBRMaterialPrototype>;*/
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

	void MaterialPrototype::InitMaterial(
		MaterialResource* material,
		DG::IShaderResourceBinding* binding, 
		PipelineResource* pipeline,
		const std::vector<TextureResource*>& textures) {

		material->Init(binding, pipeline, textures, "");
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
		MaterialResource* into) {

		DG::IShaderResourceBinding* srb = nullptr;
		mPipeline->GetState()->CreateShaderResourceBinding(&srb, true);
		
		srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mTexture")->Set(mColor->GetTexture());

		std::vector<TextureResource*> textures;
		textures.emplace_back(mColor);

		InitMaterial(into, srb, mPipeline, textures);
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