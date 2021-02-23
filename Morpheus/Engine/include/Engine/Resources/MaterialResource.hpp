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
	
	typedef std::function<void(PipelineResource*, 
		MaterialResource*, uint)>
		apply_material_func_t;

	template <>
	class ResourceCache<MaterialResource>;

	class MaterialResource : public IResource {
	private:
		PipelineResource* mPipeline;
		std::vector<TextureResource*> mTextures;
		std::vector<DG::IBuffer*> mUniformBuffers;
		bool bSourced;
		std::unordered_map<std::string, MaterialResource*>::iterator mSourceIterator;
		entt::entity mEntity;
		apply_material_func_t mApplyFunc;
		
		void SetSource(const std::unordered_map<std::string, MaterialResource*>::iterator& it);

	public:

		void Initialize(PipelineResource* pipeline,
			const std::vector<TextureResource*>& textures,
			const std::vector<DG::IBuffer*>& uniformBuffers,
			const apply_material_func_t& applyFunc);

		MaterialResource(ResourceManager* manager);
		MaterialResource(ResourceManager* manager,
			PipelineResource* pipeline,
			const std::vector<TextureResource*>& textures,
			const std::vector<DG::IBuffer*>& uniformBuffers,
			const apply_material_func_t& applyFunc);
		~MaterialResource();

		inline void Apply(uint pipelineSRBId) {
			mApplyFunc(mPipeline, this, pipelineSRBId);
		}

		MaterialResource* ToMaterial() override;

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
	public:
		static void Load(ResourceManager* manager,
			const std::string& source,
			const MaterialFactory& prototypeFactory,
			MaterialResource* loadinto);

		static TaskId AsyncLoad(ResourceManager* manager,
			const std::string& source,
			const MaterialFactory& prototypeFactory,
			ThreadPool* pool,
			TaskBarrierCallback barrierCallback,
			MaterialResource* loadInto);
	};

	template <>
	class ResourceCache<MaterialResource> : public IResourceCache {
	private:
		std::unordered_map<std::string, MaterialResource*> mResourceMap;
		ResourceManager* mManager;
		entt::registry mViewRegistry;
		MaterialFactory mPrototypeFactory;

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
}