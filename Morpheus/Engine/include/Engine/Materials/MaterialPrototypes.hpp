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

	typedef std::function<MaterialPrototype*(
		ResourceManager* manager,
		const std::string&,
		const std::string&,
		const nlohmann::json& config)> prototype_spawner_t;

	class MaterialPrototype {
	protected:
		void InternalInitialize(MaterialResource* material,
			DG::IShaderResourceBinding* binding, 
			PipelineResource* pipeline,
			const std::vector<TextureResource*>& textures,
			const std::vector<DG::IBuffer*>& buffers);

	public:
		virtual ~MaterialPrototype() {}
		virtual void InitializeMaterial(
			ResourceManager* manager,
			ResourceCache<MaterialResource>* cache, 
			MaterialResource* into) = 0;
		virtual MaterialPrototype* DeepCopy() const = 0;
	};

	template <typename T>
	MaterialPrototype* AbstractConstructor(
		ResourceManager* manager,
		const std::string& source, 
		const std::string& path,
		const nlohmann::json& config) {
		return new T(manager, source, path, config);
	}

	class MaterialPrototypeFactory {
	private:
		std::unordered_map<std::string, prototype_spawner_t> mMap;

	public:
		MaterialPrototypeFactory();
		MaterialPrototype* Spawn(
			const std::string& type,
			ResourceManager* manager,
			const std::string& source, 
			const std::string& path,
			const nlohmann::json& config) const;
	};

	DG::float4 ReadFloat4(
		const nlohmann::json& json, 
		const std::string& name, 
		const DG::float4& defaultValue);
}