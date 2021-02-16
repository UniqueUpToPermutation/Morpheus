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

	typedef std::function<MaterialPrototype*(
		ResourceManager*,
		const std::string&,
		const std::string&,
		const nlohmann::json&)> prototype_spawner_t;

	typedef std::function<TaskId(
		ResourceManager*,
		const std::string&,
		const std::string&,
		const nlohmann::json&,
		ThreadPool*,
		MaterialPrototype**)> prototype_spawner_async_t;

	class MaterialPrototype {
	protected:
		void InternalInitialize(MaterialResource* material,
			DG::IShaderResourceBinding* binding, 
			PipelineResource* pipeline,
			const std::vector<TextureResource*>& textures,
			const std::vector<DG::IBuffer*>& buffers);

	public:
		virtual ~MaterialPrototype() {}
		virtual TaskId InitializePrototype(
			ResourceManager* manager,
			const std::string& source,
			const std::string& path,
			const nlohmann::json& config,
			const MaterialAsyncParams& asyncParams) = 0;

		virtual void InitializeMaterial(
			DG::IRenderDevice* device,
			MaterialResource* into) = 0;
		virtual MaterialPrototype* DeepCopy() const = 0;

		virtual void ScheduleLoadBefore(TaskNodeDependencies dependencies) = 0;
	};

	template <typename T>
	MaterialPrototype* AbstractConstructor(
		ResourceManager* manager,
		const std::string& source, 
		const std::string& path,
		const nlohmann::json& config) {

		auto prototype = new T();
		MaterialAsyncParams params;
		params.bUseAsync = false;
		prototype->InitializePrototype(manager, source, path, config, params);
		return prototype;
	}

	template <typename T>
	TaskId AbstractAsyncConstructor(
		ResourceManager* manager,
		const std::string& source,
		const std::string& path,
		const nlohmann::json& config,
		ThreadPool* pool,
		MaterialPrototype** result) {

		auto prototype = new T();
		MaterialAsyncParams params;
		params.bUseAsync = true;
		params.mPool = pool;
		*result = prototype;
		return prototype->InitializePrototype(manager, source, path, config, params);
	};

	class MaterialPrototypeFactory {
	private:
		std::unordered_map<std::string, prototype_spawner_t> mMap;
		std::unordered_map<std::string, prototype_spawner_async_t> mAsyncMap;

	public:
		template <typename T>
		void Add(const std::string& name) {
			mMap[name] = &AbstractConstructor<T>;
			mAsyncMap[name] = &AbstractAsyncConstructor<T>;
		}

		MaterialPrototypeFactory();
		MaterialPrototype* Spawn(
			const std::string& type,
			ResourceManager* manager,
			const std::string& source, 
			const std::string& path,
			const nlohmann::json& config) const;

		TaskId SpawnAsyncDeferred(
			const std::string& type,
			ResourceManager* mananager,
			const std::string& source, 
			const std::string& path,
			const nlohmann::json& config,
			ThreadPool* pool,
			MaterialPrototype** out) const;
	};

	DG::float4 ReadFloat4(
		const nlohmann::json& json, 
		const std::string& name, 
		const DG::float4& defaultValue);
}