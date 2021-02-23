#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Engine.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/ShaderLoader.hpp>
#include <Engine/Resources/ShaderResource.hpp>
#include <Engine/ThreadTasks.hpp>
#include <Engine/Renderer.hpp>

#include <fstream>

#include <shared_mutex>
namespace Morpheus {
	PipelineResource* PipelineResource::ToPipeline() {
		return this;
	}

	ResourceCache<PipelineResource>::~ResourceCache() {
		Clear();
	}

	PipelineResource::~PipelineResource() {
		for (auto srb : mShaderResourceBindings) {
			srb->Release();
		}

		if (mState)
			mState->Release();

		mPipelineViewRegistry->destroy(mPipelineEntity);
	}

	TaskId ResourceCache<PipelineResource>::AsyncLoadFromFactory(ThreadPool* threadPool, factory_func_t factory, 
		PipelineResource** output, const LoadParams<PipelineResource>& params,
		const TaskBarrierCallback& callback) {

		auto params_cast = &params;
		auto src = params_cast->mSource;

		{
			std::shared_lock<std::shared_mutex> lock(mMutex);
			auto it = mCachedResources.find(src);
			if (it != mCachedResources.end()) {
				*output = it->second;
				return TASK_NONE;
			}
		}

		auto task = AsyncLoadFromFactory(threadPool, factory, output, &params.mOverrides, callback);

		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			(*output)->mIterator = mCachedResources.emplace(src, *output).first;
		}
		
		return task;
	}

	TaskId ResourceCache<PipelineResource>::AsyncLoadFromFactory(ThreadPool* threadPool, factory_func_t factory, 
		PipelineResource** output, const ShaderPreprocessorConfig* overrides,
		const TaskBarrierCallback& callback) {

		PipelineResource* resource = new PipelineResource(mManager, &mPipelineViewRegistry);
		AsyncResourceParams asyncParams;
		asyncParams.bUseAsync = true;
		asyncParams.mThreadPool = threadPool;
		asyncParams.mCallback = callback;

		TaskId task = factory(
			mManager->GetParent()->GetDevice(),
			mManager,
			mManager->GetParent()->GetRenderer(),
			resource,
			overrides,
			&asyncParams);

		resource->mFactory = factory;

		*output = resource;

		return task;
	}

	PipelineResource* ResourceCache<PipelineResource>::LoadFromFactory(factory_func_t factory, const ShaderPreprocessorConfig* overrides) {
		PipelineResource* resource = new PipelineResource(mManager, &mPipelineViewRegistry);
		AsyncResourceParams asyncParams;
		asyncParams.bUseAsync = false;

		factory(
			mManager->GetParent()->GetDevice(),
			mManager,
			mManager->GetParent()->GetRenderer(),
			resource,
			overrides,
			&asyncParams);

		resource->mFactory = factory;

		return resource;
	}

	void ResourceCache<PipelineResource>::ActuallyLoad(const std::string& source, PipelineResource* into, 
		const ShaderPreprocessorConfig* overrides) {
		
		auto factoryIt = mPipelineFactories.find(source);

		// See if this corresponds to one of our pipeline factories
		if (factoryIt != mPipelineFactories.end()) {
			
			std::cout << "Loading Internal Pipeline " << source << "..." << std::endl;

			AsyncResourceParams asyncParams;
			asyncParams.bUseAsync = false;
			into->mFactory = factoryIt->second;

			// Spawn from factory
			factoryIt->second(
				mManager->GetParent()->GetDevice(),
				mManager,
				mManager->GetParent()->GetRenderer(),
				into,
				overrides,
				&asyncParams);
		} else {
			// Spawn from json
			throw std::runtime_error("Could not find pipeline factory!");
		}
	}

	TaskId ResourceCache<PipelineResource>::ActuallyLoadAsync(const std::string& source, PipelineResource* into,
		ThreadPool* pool, 
		TaskBarrierCallback callback,
		const ShaderPreprocessorConfig* overrides) {
		
		auto factoryIt = mPipelineFactories.find(source);

		// See if this corresponds to one of our pipeline factories
		if (factoryIt != mPipelineFactories.end()) {
			
			std::cout << "Loading Internal Pipeline " << source << "..." << std::endl;

			AsyncResourceParams asyncParams;
			asyncParams.bUseAsync = true;
			asyncParams.mThreadPool = pool;
			asyncParams.mCallback = callback;
			into->mFactory = factoryIt->second;

			// Async spawn from factory
			return factoryIt->second(
				mManager->GetParent()->GetDevice(),
				mManager,
				mManager->GetParent()->GetRenderer(),
				into,
				overrides,
				&asyncParams);
		} else {
			// Spawn from json
			throw std::runtime_error("Could not find pipeline factory!");
		}
	}

	PipelineResource* ResourceCache<PipelineResource>::LoadFromFactory(factory_func_t factory, 
		const LoadParams<PipelineResource>& params) {
		// Spawn from factory
		auto params_cast = &params;
		auto src = params_cast->mSource;

		{
			std::shared_lock<std::shared_mutex> lock(mMutex);
			auto it = mCachedResources.find(src);
			if (it != mCachedResources.end()) {
				return it->second;
			}
		}

		auto resource = LoadFromFactory(factory, &params.mOverrides);

		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			resource->SetSource(mCachedResources.emplace(src, resource).first);
		}
		
		return resource;
	}

	IResource* ResourceCache<PipelineResource>::Load(const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<PipelineResource>*>(params);
		auto src = params_cast->mSource;
		auto overrides = &params_cast->mOverrides;

		{
			std::shared_lock<std::shared_mutex> lock(mMutex);
			auto it = mCachedResources.find(src);
			if (it != mCachedResources.end()) {
				return it->second;
			}
		}

		PipelineResource* resource = new PipelineResource(mManager, &mPipelineViewRegistry);
		ActuallyLoad(src, resource, overrides);

		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			resource->SetSource(mCachedResources.emplace(src, resource).first);
		}
		
		return resource;
	}

	TaskId ResourceCache<PipelineResource>::AsyncLoadDeferred(const void* params,
		ThreadPool* threadPool,
		IResource** output,
		const TaskBarrierCallback& callback)  {

		auto params_cast = reinterpret_cast<const LoadParams<PipelineResource>*>(params);
		auto src = params_cast->mSource;
		auto overrides = &params_cast->mOverrides;

		{
			std::shared_lock<std::shared_mutex> lock(mMutex);
			auto it = mCachedResources.find(src);
			if (it != mCachedResources.end()) {
				*output = it->second;
				return TASK_NONE;
			}
		}

		PipelineResource* resource = new PipelineResource(mManager, &mPipelineViewRegistry);
		TaskId task = ActuallyLoadAsync(src, resource, threadPool, callback, overrides);

		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			resource->SetSource(mCachedResources.emplace(src, resource).first);
		}

		*output = resource;
		
		return task;
	}

	void ResourceCache<PipelineResource>::Add(IResource* resource, const void* params) {
		throw std::runtime_error("Not supported!");
	}

	void ResourceCache<PipelineResource>::Unload(IResource* resource) {
		auto pipeline = resource->ToPipeline();

		if (pipeline->IsSourced()) {
			std::unique_lock<std::shared_mutex> lock(mMutex);
			mCachedResources.erase(pipeline->mIterator);
		}
	
		delete resource;
	}

	void ResourceCache<PipelineResource>::Clear() {
		for (auto& item : mCachedResources) {
			item.second->ResetRefCount();
			delete item.second;
		}
	}
}