#include <Engine/Materials/MaterialPrototypes.hpp>
#include <Engine/Resources/TextureResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/MaterialResource.hpp>
#include <Engine/Materials/StaticMeshPBRMaterial.hpp>
#include <Engine/Materials/BasicTexturedMaterial.hpp>
#include <Engine/Materials/WhiteMaterial.hpp>

#include "MapHelper.hpp"

namespace Morpheus {

	MaterialFactory::MaterialFactory() {
		mMap["BasicTexturedMaterial"] = &BasicTexturedMaterialPrototype;
		mMap["StaticMeshPBRMaterial"] = &StaticMeshPBRMaterialPrototype;
		mMap["WhiteMaterial"] = &WhiteMaterialPrototype;
	}

	void MaterialFactory::Spawn(
		const std::string& type,
		ResourceManager* manager,
		const std::string& source, 
		const std::string& path,
		const nlohmann::json& config,
		MaterialResource* materialOut) const {

		auto it = mMap.find(type);
		if (it != mMap.end()) {
			MaterialAsyncParams params;
			params.bUseAsync = false;
			it->second(manager, path, source, config, params, materialOut);
		} else {
			throw std::runtime_error("Requested material type could not be found!");
		}
	}

	void MaterialFactory::SpawnAsync(
		const std::string& type,
		ResourceManager* manager,
		const std::string& source, 
		const std::string& path,
		const nlohmann::json& config,
		ThreadPool* pool,
		MaterialResource* materialOut) const {
		auto it = mMap.find(type);
		if (it != mMap.end()) {
			MaterialAsyncParams params;
			params.bUseAsync = true;
			params.mPool = pool;
			it->second(manager, source, path, config, params, materialOut);
		} else {
			throw std::runtime_error("Requested material type could not be found!");
		}
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