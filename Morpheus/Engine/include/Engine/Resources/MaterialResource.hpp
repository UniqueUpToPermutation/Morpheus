#pragma once 

#include <Engine/Resources/Resource.hpp>
#include <Engine/Materials/MaterialPrototypes.hpp>
#include <Engine/InputController.hpp>

#include <shared_mutex>

#include "EngineFactory.h"
#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
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
		bool bSourced;
		std::unordered_map<std::string, MaterialResource*>::iterator mSourceIterator;
		ResourceCache<MaterialResource>* mCache;
		entt::entity mEntity;
		std::unique_ptr<MaterialPrototype> mPrototype;

		void Init(DG::IShaderResourceBinding* binding, 
			PipelineResource* pipeline,
			const std::vector<TextureResource*>& textures,
			const std::vector<DG::IBuffer*>& uniformBuffers);
		
		void SetSource(const std::unordered_map<std::string, MaterialResource*>::iterator& it);

	public:

		MaterialResource(ResourceManager* manager,
			ResourceCache<MaterialResource>* cache);
		MaterialResource(ResourceManager* manager,
			DG::IShaderResourceBinding* binding, 
			PipelineResource* pipeline,
			const std::vector<TextureResource*>& textures,
			const std::vector<DG::IBuffer*>& uniformBuffers,
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
			if (bSourced) {
				mSourceIterator->first;
			} else {
				return "No Source";
			}
		}

		entt::id_type GetType() const override {
			return resource_type::type<MaterialResource>;
		}

		template <typename ViewType> 
		inline ViewType* GetView();

		template <typename T, typename... Args>
		inline T* CreateView(MaterialResource* resource, Args &&... args);

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

		TaskId AsyncLoad(const std::string& source,
			const MaterialPrototypeFactory& prototypeFactory,
			ThreadPool* pool,
			TaskBarrierCallback barrierCallback,
			MaterialResource* loadInto);
	};

	template <>
	class ResourceCache<MaterialResource> : public IResourceCache {
	private:
		std::unordered_map<std::string, MaterialResource*> mResourceMap;
		ResourceManager* mManager;
		MaterialLoader mLoader;
		entt::registry mViewRegistry;
		MaterialPrototypeFactory mPrototypeFactory;

		std::shared_mutex mResourceMapMutex;

	public:
		ResourceCache(ResourceManager* manager);
		~ResourceCache();

		IResource* Load(const void* params) override;
		TaskId AsyncLoadDeferred(const void* params,
			ThreadPool* threadPool,
			IResource** output,
			const TaskBarrierCallback& callback = nullptr) override;
		void Add(IResource* resource, const void* params) override;
		void Unload(IResource* resource) override;
		void Clear() override;

		template <typename T, typename... Args>
		inline T* CreateView(MaterialResource* resource, Args &&... args) {
			return &mViewRegistry.template emplace<T, Args...>(
				resource->mEntity, std::forward<Args>(args)...);
		}

		template <typename T>
		inline T* GetView(MaterialResource* resource) {
			return mViewRegistry.template try_get<T>(resource->mEntity);
		}

		friend class MaterialResource;
	};

	template <typename ViewType> 
	inline ViewType* MaterialResource::GetView() {
		return mCache->GetView<ViewType>(this);
	}

	template <typename ViewType, typename... Args>
	inline ViewType* MaterialResource::CreateView(MaterialResource* resource, Args &&... args) {
		return mCache->template CreateView<ViewType, Args...>(
			this, std::forward<Args>(args)...);
	}
}