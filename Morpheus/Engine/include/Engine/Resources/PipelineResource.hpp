#pragma once

#include <shared_mutex>

#include <Engine/Geometry.hpp>
#include <Engine/Resources/Resource.hpp>
#include <Engine/Resources/ShaderLoader.hpp>
#include <Engine/Pipelines/PipelineFactory.hpp>
#include <Engine/InputController.hpp>

#include <nlohmann/json.hpp>

#include "EngineFactory.h"
#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "BasicMath.hpp"

namespace DG = Diligent;

namespace Morpheus {

	enum class InstancingType {
		// No instancing
		NONE,
		// A float4x4 is written to an instance buffer and passed as input to VS
		INSTANCED_STATIC_TRANSFORMS
	};

	class PipelineResource : public IResource {
	private:
		DG::IPipelineState* mState;
		factory_func_t mFactory;
		VertexLayout mLayout;
		InstancingType mInstancingType;
		bool bSourced;
		std::unordered_map<std::string, PipelineResource*>::iterator mIterator;
		entt::registry* mPipelineViewRegistry;
		entt::entity mPipelineEntity;
		std::vector<DG::IShaderResourceBinding*> mShaderResourceBindings;

		inline void SetSource(const std::unordered_map<std::string, PipelineResource*>::iterator& it) {
			mIterator = it;
			bSourced = true;
		}

	public:
		~PipelineResource();

		template <typename T, typename... Args> 
		void AddView(Args&& ... args) {
			mPipelineViewRegistry->emplace<T>(mPipelineEntity, std::forward<Args>(args)...);
		}

		std::vector<DG::IShaderResourceBinding*>& 
			GetShaderResourceBindings() {
			return mShaderResourceBindings;
		}

		inline uint GetMaxThreadCount() const {
			return mShaderResourceBindings.size();
		}

		inline PipelineResource(ResourceManager* manager,
			entt::registry* pipelineViewRegistry) :
			IResource(manager),
			mState(nullptr),
			mInstancingType(InstancingType::INSTANCED_STATIC_TRANSFORMS),
			bSourced(false),
			mPipelineViewRegistry(pipelineViewRegistry) {
			mPipelineEntity = mPipelineViewRegistry->create();
		}

		inline void SetAll(DG::IPipelineState* state,
			const std::vector<DG::IShaderResourceBinding*>& shaderResourceBindings,
			const VertexLayout& layout,
			InstancingType instancingType) {

			mShaderResourceBindings = shaderResourceBindings;

			if (mState) {
				mState->Release();
				mState = nullptr;
			}
				
			mState = state;
			mLayout = layout;
			mInstancingType = instancingType;
		}

		template <typename T> 
		T& GetView() {
			return mPipelineViewRegistry->get<T>(mPipelineEntity);
		}

		template <typename T>
		T* TryGetView() {
			return mPipelineViewRegistry->try_get<T>(mPipelineEntity);
		}

		inline bool IsReady() const {
			return mState != nullptr;
		}

		inline DG::IPipelineState* GetState() {
			return mState;
		}

		inline const DG::IPipelineState* GetState() const {
			return mState;
		}

		inline const factory_func_t& GetFactory() const {
			return mFactory;
		}

		entt::id_type GetType() const override {
			return resource_type::type<PipelineResource>;
		}

		inline const VertexLayout& GetVertexLayout() const {
			return mLayout;
		}

		inline bool IsSourced() const {
			return bSourced;
		}

		PipelineResource* ToPipeline() override;

		friend class PipelineLoader;
		friend class ResourceCache<PipelineResource>;
	};

	template <>
	struct LoadParams<PipelineResource> {
		std::string mSource;
		ShaderPreprocessorConfig mOverrides;

		static LoadParams<PipelineResource> FromString(const std::string& str) {
			LoadParams<PipelineResource> p;
			p.mSource = str;
			return p;
		}
	};

	template <>
	class ResourceCache<PipelineResource> : public IResourceCache {
	private:
		std::unordered_map<std::string, PipelineResource*> mCachedResources;
		std::unordered_map<std::string, factory_func_t> mPipelineFactories;
		ResourceManager* mManager;

		entt::registry mPipelineViewRegistry;

		void InitFactories();

		Task ActuallyLoad(const std::string& source, PipelineResource* into,
			const ShaderPreprocessorConfig* overrides=nullptr);
		Task ActuallyLoadFromFactory(factory_func_t factory, 
			PipelineResource** output, const LoadParams<PipelineResource>& params);

		std::shared_mutex mMutex;

	public:
		inline ResourceCache(ResourceManager* manager) :
			mManager(manager) {
			InitFactories();
		}
		~ResourceCache();

		Task LoadFromFactoryTask(factory_func_t factory, 
			PipelineResource** output, const LoadParams<PipelineResource>& params);

		inline PipelineResource* LoadFromFactory(factory_func_t factory, const LoadParams<PipelineResource>& params) {
			PipelineResource* result = nullptr;
			LoadFromFactoryTask(factory, &result, params)();
			return result;
		}

		inline PipelineResource* LoadFromFactory(factory_func_t factory, const ShaderPreprocessorConfig* overrides = nullptr) {
			PipelineResource* result = nullptr;
			LoadParams<PipelineResource> params;
			if (overrides)
				params.mOverrides = *overrides;
			LoadFromFactoryTask(factory, &result, params)();
			return result;
		}

		Task LoadTask(const void* params, IResource** output) override;

		void Add(IResource* resource, const void* params) override;
		void Unload(IResource* resource) override;
		void Clear() override;
	};
}