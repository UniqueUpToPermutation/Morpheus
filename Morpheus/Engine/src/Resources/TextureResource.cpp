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

	Task ResourceCache<TextureResource>::LoadTask(const void* params,
		IResource** output) {

		auto params_cast = reinterpret_cast<const LoadParams<TextureResource>*>(params);
		
		{
			std::shared_lock<std::shared_mutex> lock(mMutex);
			auto it = mResourceMap.find(params_cast->mSource);

			if (it != mResourceMap.end()) {
				*output = it->second;
				return Task();
			}
		} 

		auto result = new TextureResource(mManager);

		struct Data {
			std::unique_ptr<RawTexture> mTexture;
		};

		Task task([this, result, params = *params_cast, data = Data()](const TaskParams& e) mutable {
			if (e.mTask->SubTask()) {
				data.mTexture = std::make_unique<RawTexture>();
				data.mTexture->Load(params);

				if (e.mTask->RequestThreadSwitch(e, ASSIGN_THREAD_MAIN))
					return TaskResult::REQUEST_THREAD_SWITCH;
			}

			if (e.mTask->SubTask()) {
				result->mTexture = data.mTexture->SpawnOnGPU(mManager->GetParent()->GetDevice());
				result->SetLoaded(true);
			}

			return TaskResult::FINISHED;
		},
		std::string("Upload Texture ") + params_cast->mSource,
		TaskType::FILE_IO);

		result->GetLoadBarrier()->mIn.Lock().Connect(&task->Out());

		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			result->SetSource(mResourceMap.emplace(params_cast->mSource, result).first);
		}

		*output = result;

		return task;
	}

	void ResourceCache<TextureResource>::Add(TextureResource* tex, const std::string& source) {

		if (!tex->IsLoaded()) {
			throw std::runtime_error("Texture must be loaded before it can be added to resource cache!");
		}

		assert(tex->IsLoaded());
		assert(tex->GetLoadBarrier()->mOut.Lock().IsFinished());

		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			tex->SetSource(mResourceMap.emplace(source, tex).first);
			mResourceSet.emplace(tex);
		}
	}

	void ResourceCache<TextureResource>::Add(TextureResource* tex) {

		if (!tex->IsLoaded()) {
			throw std::runtime_error("Texture must be loaded before it can be added to resource cache!");
		}

		assert(tex->IsLoaded());
		assert(tex->GetLoadBarrier()->mOut.Lock().IsFinished());

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