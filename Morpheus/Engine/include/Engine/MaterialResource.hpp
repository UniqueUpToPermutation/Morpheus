#pragma once 

#include <Engine/Resource.hpp>

#include "EngineFactory.h"
#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "InputController.hpp"
#include "BasicMath.hpp"

#include <nlohmann/json.hpp>

namespace DG = Diligent;

namespace Morpheus {
	class MaterialResource : public Resource {
	private:
		DG::IShaderResourceBinding* mResourceBinding;
		PipelineResource* mPipeline;
		std::vector<TextureResource*> mTextures;
		std::string mSource;

	public:

		MaterialResource(ResourceManager* manager,
			DG::IShaderResourceBinding* binding, 
			PipelineResource* pipeline,
			const std::vector<TextureResource*>& textures,
			const std::string& source);
		~MaterialResource();

		MaterialResource* ToMaterial() override;

		inline DG::IShaderResourceBinding* GetResourceBinding() {
			return mResourceBinding;
		}

		inline PipelineResource* GetPipeline() {
			return mPipeline;
		}

		inline std::vector<TextureResource*> GetTextures() const {
			return mTextures;
		}

		inline std::string GetSource() const {
			return mSource;
		}

		entt::id_type GetType() const override {
			return resource_type::type<MaterialResource>;
		}
	};

	template <>
	struct LoadParams<MaterialResource> {
	public:
		std::string mSource;

		static LoadParams<MaterialResource> FromString(const std::string& str) {
			LoadParams<MaterialResource> params;
			params.mSource = str;
			return params;
		}
	};

	class MaterialLoader {
	private:
		ResourceManager* mManager;

	public:
		MaterialLoader(ResourceManager* manager);

		MaterialResource* Load(const std::string& source);
		MaterialResource* Load(const nlohmann::json& json, 
			const std::string& source, const std::string& path);
	};

	template <>
	class ResourceCache<MaterialResource> : public IResourceCache {
	private:
		std::unordered_map<std::string, MaterialResource*> mResources;
		ResourceManager* mManager;
		MaterialLoader mLoader;

	public:
		ResourceCache(ResourceManager* manager);
		~ResourceCache();

		Resource* Load(const void* params) override;
		void Add(Resource* resource, const void* params) override;
		void Unload(Resource* resource) override;
		void Clear() override;
	};
}