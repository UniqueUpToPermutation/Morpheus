#include <Engine/Materials/MaterialPrototypes.hpp>
#include <Engine/Resources/TextureResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/MaterialResource.hpp>
#include <Engine/Materials/MaterialView.hpp>
#include <Engine/Materials/StaticMeshPBRMaterial.hpp>
#include <Engine/Materials/BasicTexturedMaterial.hpp>
#include <Engine/Materials/WhiteMaterial.hpp>

#include "MapHelper.hpp"

namespace Morpheus {

	MaterialPrototypeFactory::MaterialPrototypeFactory() {
		Add<BasicTexturedMaterialPrototype>("BasicTexturedMaterial");
		Add<StaticMeshPBRMaterialPrototype>("StaticMeshPBRMaterial");
		Add<WhiteMaterialPrototype>("WhiteMaterial");
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

	TaskId MaterialPrototypeFactory::SpawnAsyncDeferred(
		const std::string& type,
		ResourceManager* manager,
		const std::string& source, 
		const std::string& path,
		const nlohmann::json& config,
		ThreadPool* pool,
		MaterialPrototype** out) const {
		auto it = mAsyncMap.find(type);
		if (it != mAsyncMap.end()) {
			return it->second(manager, source, path, config, pool, out);
		}
		return TASK_NONE;
	}

	void MaterialPrototype::InternalInitialize(
		MaterialResource* material,
		DG::IShaderResourceBinding* binding, 
		PipelineResource* pipeline,
		const std::vector<TextureResource*>& textures,
		const std::vector<DG::IBuffer*>& buffers) {

		material->Init(binding, pipeline, textures, buffers);
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