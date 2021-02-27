#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/TextureResource.hpp>
#include <Engine/Engine.hpp>

#include <shared_mutex>
#include <type_traits>

namespace Morpheus {

	TextureResource* TextureResource::ToTexture() {
		return this;
	}

	TextureResource::~TextureResource() {
		if (mTexture)
			mTexture->Release();	
	}

	ResourceCache<TextureResource>::ResourceCache(ResourceManager* manager) :
		mManager(manager) {
	}

	ResourceCache<TextureResource>::~ResourceCache() {
		Clear();
	}

	TaskId ResourceCache<TextureResource>::ActuallyLoad(const void* params,
		const AsyncResourceParams& asyncParams,
		IResource** output) {

		auto params_cast = reinterpret_cast<const LoadParams<TextureResource>*>(params);
		
		{
			std::shared_lock<std::shared_mutex> lock(mMutex);
			auto it = mResourceMap.find(params_cast->mSource);

			if (it != mResourceMap.end()) {
				*output = it->second;
				return TASK_NONE;
			}
		} 

		auto result = new TextureResource(mManager);
		TaskId taskId;

		if (asyncParams.bUseAsync) {
			auto raw = std::make_shared<RawTexture>();

			taskId = raw->LoadAsyncDeferred(*params_cast, asyncParams.mThreadPool);

			auto device = mManager->GetParent()->GetDevice();
			auto queue = asyncParams.mThreadPool->GetQueue();
			auto rawBarrier = raw->GetLoadBarrier();
			auto gpuSpawnBarrier = result->GetLoadBarrier();

			gpuSpawnBarrier->SetCallback(asyncParams.mCallback);

			TaskId rawToGpu = queue.MakeTask([raw, device, result](const TaskParams& params) {
				DG::ITexture* tex = raw->SpawnOnGPU(device);
				result->mTexture = tex;
			}, gpuSpawnBarrier);

			queue.Dependencies(rawToGpu).After(rawBarrier);

		} else {
			RawTexture raw(*params_cast);
			DG::ITexture* tex = raw.SpawnOnGPU(mManager->GetParent()->GetDevice());
			result->mTexture = tex;
			taskId = TASK_NONE;
		}
		
		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			result->SetSource(mResourceMap.emplace(params_cast->mSource, result).first);
		}

		*output = result;
		return taskId;
	}

	IResource* ResourceCache<TextureResource>::Load(const void* params) {
		AsyncResourceParams asyncParams;
		asyncParams.bUseAsync = false;

		IResource* result = nullptr;
		ActuallyLoad(params, asyncParams, &result);
		
		return result;
	}

	TaskId ResourceCache<TextureResource>::AsyncLoadDeferred(const void* params, 
		ThreadPool* pool,
		IResource** resource,
		const TaskBarrierCallback& callback) {

		AsyncResourceParams asyncParams;
		asyncParams.bUseAsync = true;
		asyncParams.mThreadPool = pool;
		asyncParams.mCallback = callback;

		return ActuallyLoad(params, asyncParams, resource);
	}

	void ResourceCache<TextureResource>::Add(TextureResource* tex, const std::string& source) {
		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			tex->SetSource(mResourceMap.emplace(source, tex).first);
			mResourceSet.emplace(tex);
		}
	}

	void ResourceCache<TextureResource>::Add(TextureResource* tex) {
		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			mResourceSet.emplace(tex);
		}
	}

	void ResourceCache<TextureResource>::Add(IResource* resource, const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<TextureResource>*>(params);
		auto tex = resource->ToTexture();

		Add(tex, params_cast->mSource);
	}

	void ResourceCache<TextureResource>::Unload(IResource* resource) {
		auto tex = resource->ToTexture();

		std::unique_lock<std::shared_mutex> lock(mMutex);

		if (tex->bSourced) {
			auto it = tex->mSource;
			if (it != mResourceMap.end()) {
				if (it->second == tex) {
					mResourceMap.erase(it);
				}
			}
		}

		mResourceSet.erase(tex);

		delete resource;
	}

	void ResourceCache<TextureResource>::Clear() {
		std::unique_lock<std::shared_mutex> lock(mMutex);
		for (auto& item : mResourceSet) {
			item->ResetRefCount();
			delete item;
		}

		mResourceSet.clear();
		mResourceMap.clear();
	}

	void SavePng(DG::ITexture* texture, const std::string& path, 
		DG::IDeviceContext* context, DG::IRenderDevice* device, bool bSaveMips) {
		RawTexture rawTex(texture, device, context);
		rawTex.SavePng(path, bSaveMips);
	}

	void SaveGli(DG::ITexture* texture, const std::string& path, 
		DG::IDeviceContext* context, DG::IRenderDevice* device) {
		RawTexture rawTex(texture, device, context);
		rawTex.SaveGli(path);
	}

	void TextureResource::SavePng(const std::string& path, bool bSaveMips) {
		Morpheus::SavePng(mTexture, path, 
			GetManager()->GetParent()->GetImmediateContext(),
			GetManager()->GetParent()->GetDevice(), bSaveMips);
	}

	void TextureResource::SaveGli(const std::string& path) {
		Morpheus::SaveGli(mTexture, path,
			GetManager()->GetParent()->GetImmediateContext(),
			GetManager()->GetParent()->GetDevice());
	}

	TextureResource* ResourceCache<TextureResource>::MakeResource(
		DG::ITexture* texture, const std::string& source) {

		std::cout << "Creating texture resource " << source << "..." << std::endl;

		TextureResource* res = new TextureResource(mManager, texture);
		res->AddRef();
		Add(res, source);
		return res;
	}

	TextureResource* ResourceCache<TextureResource>::MakeResource(DG::ITexture* texture) {
		TextureResource* res = new TextureResource(mManager, texture);
		res->AddRef();
		Add(res);
		return res;
	}
}