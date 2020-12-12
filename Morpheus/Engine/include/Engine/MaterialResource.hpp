#pragma once 

#include <Engine/Resource.hpp>
#include <Engine/MaterialPrototypes.hpp>

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
	template <>
	class ResourceCache<MaterialResource>;

	class MaterialResource : public IResource {
	private:
		DG::IShaderResourceBinding* mResourceBinding;
		PipelineResource* mPipeline;
		std::vector<TextureResource*> mTextures;
		std::vector<DG::IBuffer*> mUniformBuffers;
		std::string mSource;
		ResourceCache<MaterialResource>* mCache;
		entt::entity mEntity;
		std::unique_ptr<MaterialPrototype> mPrototype;

		void Init(DG::IShaderResourceBinding* binding, 
			PipelineResource* pipeline,
			const std::vector<TextureResource*>& textures,
			const std::vector<DG::IBuffer*>& uniformBuffers,
			const std::string& source);

	public:

		MaterialResource(ResourceManager* manager,
			ResourceCache<MaterialResource>* cache);
		MaterialResource(ResourceManager* manager,
			DG::IShaderResourceBinding* binding, 
			PipelineResource* pipeline,
			const std::vector<TextureResource*>& textures,
			const std::vector<DG::IBuffer*>& uniformBuffers,
			const std::string& source,
			ResourceCache<MaterialResource>* cache);
		~MaterialResource();

		inline bool IsReady() const {
			return mResourceBinding != nullptr;
		}

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

		template <typename ViewType> 
		inline ViewType* GetView();

		friend class MaterialLoader;
		friend class ResourceCache<MaterialResource>;
		friend class MaterialPrototype;
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
		ResourceCache<MaterialResource>* mCache;

	public:
		MaterialLoader(ResourceManager* manager,
			ResourceCache<MaterialResource>* cache);

		void Load(const std::string& source,
			const MaterialPrototypeFactory& prototypeFactory,
			MaterialResource* loadinto);

		void Load(const nlohmann::json& json, 
			const std::string& source, 
			const std::string& path,
			const MaterialPrototypeFactory& prototypeFactory,
			MaterialResource* loadInto);
	};

	template <>
	class ResourceCache<MaterialResource> : public IResourceCache {
	private:
		std::unordered_map<std::string, MaterialResource*> mResourceMap;
		ResourceManager* mManager;
		MaterialLoader mLoader;
		std::vector<std::pair<MaterialResource*, LoadParams<MaterialResource>>> mDeferredResources;
		entt::registry mViewRegistry;
		MaterialPrototypeFactory mPrototypeFactory;

	public:
		ResourceCache(ResourceManager* manager);
		~ResourceCache();

		IResource* Load(const void* params) override;
		IResource* DeferredLoad(const void* params) override;
		void ProcessDeferred() override;
		void Add(IResource* resource, const void* params) override;
		void Unload(IResource* resource) override;
		void Clear() override;

		template <typename T, typename... Args>
		inline T* CreateView(MaterialResource* resource, Args &&... args) {
			return &mViewRegistry.template emplace<T, Args...>(
				resource->mEntity, std::forward<Args>(args)...);
		}

		friend class MaterialResource;
	};

	template <typename ViewType> 
	ViewType* MaterialResource::GetView() {
		return mCache->mViewRegistry.try_get<ViewType>(mEntity);
	}
}