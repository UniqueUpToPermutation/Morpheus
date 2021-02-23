#pragma once

#include "BasicMath.hpp"
#include "RenderDevice.h"

using float2 = Diligent::float2;
using float3 = Diligent::float3;
using float4 = Diligent::float4;
using float4x4 = Diligent::float4x4;

#include <nlohmann/json.hpp>
#include <Engine/Resources/Resource.hpp>

namespace DG = Diligent;

namespace Morpheus {

	class MaterialResource;
	class ResourceManager;
	class PipelineResource;
	class TextureResource;
	class MaterialPrototype;

	struct MaterialAsyncParams {
		bool bUseAsync;
		ThreadPool* mPool;
	};

	typedef std::function<void(
		ResourceManager*,
		const std::string&,
		const std::string&,
		const nlohmann::json&,
		const MaterialAsyncParams&,
		MaterialResource*)> material_prototype_t;

	class MaterialFactory {
	private:
		std::unordered_map<std::string, material_prototype_t> mMap;

	public:
		MaterialFactory();
		void Spawn(
			const std::string& type,
			ResourceManager* manager,
			const std::string& source, 
			const std::string& path,
			const nlohmann::json& config,
			MaterialResource* materialOut) const;

		void SpawnAsync(
			const std::string& type,
			ResourceManager* mananager,
			const std::string& source, 
			const std::string& path,
			const nlohmann::json& config,
			ThreadPool* pool,
			MaterialResource* materialOut) const;
	};

	DG::float4 ReadFloat4(
		const nlohmann::json& json, 
		const std::string& name, 
		const DG::float4& defaultValue);
}