#include <Engine/MaterialPrototypes.hpp>
#include <Engine/TextureResource.hpp>
#include <Engine/ResourceManager.hpp>
#include <Engine/PipelineResource.hpp>
#include <Engine/MaterialResource.hpp>
#include <Engine/MaterialView.hpp>
#include <Engine/Materials/StaticMeshPBRMaterial.hpp>
#include <Engine/Materials/BasicTexturedMaterial.hpp>

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
}