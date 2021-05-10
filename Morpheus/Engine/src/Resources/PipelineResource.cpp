#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Engine.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/ShaderLoader.hpp>
#include <Engine/Resources/ShaderResource.hpp>
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

	Task ResourceCache<PipelineResource>::ActuallyLoadFromFactory(factory_func_t factory, 
		PipelineResource** output, 
		const LoadParams<PipelineResource>& params) {

		PipelineResource* resource = new PipelineResource(mManager, &mPipelineViewRegistry);
		
		Task task = factory(
			mManager->GetParent()->GetDevice(),
			mManager,
			mManager->GetParent()->GetRenderer(),
			resource,
			&params.mOverrides);

		resource->GetLoadBarrier()->mIn.Lock().Connect(&task);
		resource->mFactory = factory;

		*output = resource;

		return task;
	}

	Task ResourceCache<PipelineResource>::ActuallyLoad(const std::string& source, 
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides) {
		
		auto factoryIt = mPipelineFactories.find(source);

		// See if this corresponds to one of our pipeline factories
		if (factoryIt != mPipelineFactories.end()) {
			
			std::cout << "Loading Internal Pipeline " << source << "..." << std::endl;
			into->mFactory = factoryIt->second;

			Task task = factoryIt->second(mManager->GetParent()->GetDevice(),
				mManager,
				mManager->GetParent()->GetRenderer(),
				into,
				overrides);

			into->GetLoadBarrier()->mIn.Lock().Connect(&task);
			return task;

		} else {
			// Spawn from json
			throw std::runtime_error("Could not find pipeline factory!");
		}
	}

	Task ResourceCache<PipelineResource>::LoadFromFactoryTask(
		factory_func_t factory, 
		PipelineResource** output,
		const LoadParams<PipelineResource>& params) {
		// Spawn from factory
		auto params_cast = &params;
		auto src = params_cast->mSource;

		{
			std::shared_lock<std::shared_mutex> lock(mMutex);
			auto it = mCachedResources.find(src);
			if (it != mCachedResources.end()) {
				*output = it->second;
				return Task();
			}
		}

		auto task = ActuallyLoadFromFactory(factory, output, params);

		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			(*output)->SetSource(mCachedResources.emplace(src, *output).first);
		}
		
		return task;
	}

	Task ResourceCache<PipelineResource>::LoadTask(const void* params,
		IResource** output)  {

		auto params_cast = reinterpret_cast<const LoadParams<PipelineResource>*>(params);
		auto src = params_cast->mSource;
		auto overrides = &params_cast->mOverrides;

		{
			std::shared_lock<std::shared_mutex> lock(mMutex);
			auto it = mCachedResources.find(src);
			if (it != mCachedResources.end()) {
				*output = it->second;
				return Task();
			}
		}

		PipelineResource* resource = new PipelineResource(mManager, &mPipelineViewRegistry);
		Task task = ActuallyLoad(src, resource, overrides);

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