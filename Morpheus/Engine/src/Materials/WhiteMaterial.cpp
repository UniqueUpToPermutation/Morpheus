#include <Engine/Materials/WhiteMaterial.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/PipelineResource.hpp>

namespace Morpheus {
	WhiteMaterialPrototype::WhiteMaterialPrototype(
		ResourceManager* manager,
		const std::string& source, 
		const std::string& path,
		const nlohmann::json& config) {

		std::string pipeline_src = config.value("Pipeline", "White");

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

		mPipeline = manager->Load<PipelineResource>(pipeline_src);
	}

	WhiteMaterialPrototype::WhiteMaterialPrototype(
		PipelineResource* pipeline) {
		mPipeline = pipeline;
		mPipeline->AddRef();
	}

	WhiteMaterialPrototype::~WhiteMaterialPrototype() {
		mPipeline->Release();
	}

	void WhiteMaterialPrototype::InitializeMaterial(
		ResourceManager* manager,
		ResourceCache<MaterialResource>* cache,
		MaterialResource* into) {

		DG::IShaderResourceBinding* srb = nullptr;
		mPipeline->GetState()->CreateShaderResourceBinding(&srb, true);
		
		std::vector<TextureResource*> textures;
		std::vector<DG::IBuffer*> bufs;

		InternalInitialize(into, srb, mPipeline, textures, bufs);
	}

	WhiteMaterialPrototype::WhiteMaterialPrototype(const WhiteMaterialPrototype& other) : 
		mPipeline(other.mPipeline) {
		mPipeline->AddRef();
	}

	MaterialPrototype* WhiteMaterialPrototype::DeepCopy() const {
		return new WhiteMaterialPrototype(*this);
	}
}