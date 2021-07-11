#include <Engine/Systems/TextureCache.hpp>

namespace Morpheus {

	TextureCacheSystem::loader_t::cache_load_t 
		TextureCacheSystem::GetLoaderFunction() {
		return [device = mDevice](const LoadParams<Texture>& params) {
			return Texture::LoadPointer(device, params);
		};
	}

	TextureCacheSystem::loader_t::load_callback_t 
		TextureCacheSystem::GetLoadCallback() {
		return [this](const typename cache_t::iterator_t& it) {
			GarbageCollector().OnResourceLoaded(it);
		};
	}

	Future<Texture*> TextureCacheSystem::Load(
		const LoadParams<Texture>& params, ITaskQueue* queue) {
		return mLoader.Load(params, &mCache, queue);
	}

	Task TextureCacheSystem::Startup(SystemCollection& systems) {
		ParameterizedTask<UpdateParams> update([this](const TaskParams& e, const UpdateParams& params) {
			mLoader.Update();
			mGarbageCollector.CollectGarbage(e);
		}, 
		"Update Texture Cache",
		TaskType::UPDATE);

		systems.GetFrameProcessor().AddUpdateTask(std::move(update));
		return Task();
	}

	bool TextureCacheSystem::IsInitialized() const {
		return true;
	}

	void TextureCacheSystem::Shutdown() {
		mGarbageCollector = gc_t(mCache);
		mCache.Clear();
		mCache.Clear();
	}

	void TextureCacheSystem::NewFrame(Frame* frame) {
		
	}

	void TextureCacheSystem::OnAddedTo(SystemCollection& collection) {
		collection.AddCacheInterface<Texture>(this);	
	}
}