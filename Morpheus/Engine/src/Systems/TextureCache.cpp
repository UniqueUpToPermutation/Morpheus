#include <Engine/Systems/TextureCache.hpp>

namespace Morpheus {

	TextureCacheSystem::loader_t::cache_load_t 
		TextureCacheSystem::GetLoaderFunction() {
		return [device = mDevice](const LoadParams<Texture>& params) {
			return Texture::LoadHandle(device, params);
		};
	}

	TextureCacheSystem::loader_t::load_callback_t 
		TextureCacheSystem::GetLoadCallback() {
		return [this](const typename cache_t::iterator_t& it) {
			GarbageCollector().OnResourceLoaded(it);
		};
	}

	Future<Handle<Texture>> TextureCacheSystem::Load(
		const LoadParams<Texture>& params, IComputeQueue* queue) {
		return mLoader.Load(params, &mCache, queue);
	}

	std::unique_ptr<Task> TextureCacheSystem::Startup(SystemCollection& systems) {

		update_proto_t updater([this](const TaskParams& e, Future<UpdateParams> params) {
			mLoader.Update();
			mGarbageCollector.CollectGarbage(e);
		});

		auto& processor = systems.GetFrameProcessor();

		systems.GetFrameProcessor().AddUpdateTask(updater(processor.GetUpdateInput()));
		return nullptr;
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