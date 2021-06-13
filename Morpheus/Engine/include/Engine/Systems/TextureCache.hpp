#pragma once

#include <Engine/Resources/Texture.hpp>
#include <Engine/Resources/ResourceCache.hpp>
#include <Engine/Systems/System.hpp>
#include <Engine/Graphics.hpp>

namespace Morpheus {
	class TextureCacheSystem : public ISystem, 
		public IResourceCache<Texture> {
	private:
		using cache_t = ResourceCache<Texture*, 
			Texture::LoadParameters, 
			Texture::LoadParameters::Hasher>;
		using loader_t = DefaultLoader<Texture*, 
			Texture::LoadParameters, 
			Texture::LoadParameters::Hasher>;
		using gc_t = DefaultGarbageCollector<Texture*, 
			Texture::LoadParameters, 
			Texture::LoadParameters::Hasher>;

		loader_t::cache_load_t GetLoaderFunction();
		loader_t::load_callback_t GetLoadCallback();

		DG::IRenderDevice* mDevice;

		cache_t mCache;
		loader_t mLoader;
		gc_t mGarbageCollector;

	public:
		inline cache_t& Cache() { return mCache; }
		inline loader_t& Loader() { return mLoader; }
		inline gc_t& GarbageCollector() { return mGarbageCollector; }

		inline TextureCacheSystem(DG::IRenderDevice* device) : 
			mDevice(device), mLoader(GetLoaderFunction(), 
				GetLoadCallback()), mGarbageCollector(mCache) {	
		}

		inline TextureCacheSystem(Graphics& graphics) : 
			TextureCacheSystem(graphics.Device()) {	
		}

		Future<Texture*> Load(const LoadParams<Texture>& params, ITaskQueue* queue) override;

		Task Startup(SystemCollection& systems) override;
		bool IsInitialized() const override;
		void Shutdown() override;
		void NewFrame(Frame* frame) override;
		void OnAddedTo(SystemCollection& collection) override;
	};
}