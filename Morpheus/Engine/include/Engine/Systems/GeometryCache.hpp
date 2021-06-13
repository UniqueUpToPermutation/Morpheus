#pragma once

#include <Engine/Resources/Geometry.hpp>
#include <Engine/Resources/ResourceCache.hpp>
#include <Engine/Systems/System.hpp>
#include <Engine/Graphics.hpp>

namespace Morpheus {
	class GeometryCacheSystem : public ISystem, 
		public IResourceCache<Geometry> {
	private:
		using cache_t = ResourceCache<Geometry*, 
			Geometry::LoadParameters,
			Geometry::LoadParameters::Hasher>;
		using loader_t = DefaultLoader<Geometry*, 
			Geometry::LoadParameters,
			Geometry::LoadParameters::Hasher>;
		using gc_t = DefaultGarbageCollector<Geometry*, 
			Geometry::LoadParameters,
			Geometry::LoadParameters::Hasher>;

		loader_t::cache_load_t GetLoaderFunction();
		loader_t::load_callback_t GetLoadCallback();

		DG::IRenderDevice* mDevice;
		IVertexFormatProvider* mFormatProvider;

		cache_t mCache;
		loader_t mLoader;
		gc_t mGarbageCollector;

	public:
		inline cache_t& Cache() { return mCache; }
		inline loader_t& Loader() { return mLoader; }
		inline gc_t& GarbageCollector() { return mGarbageCollector; } 

		inline GeometryCacheSystem(DG::IRenderDevice* device) : 
			mDevice(device), mLoader(GetLoaderFunction(), 
				GetLoadCallback()), mGarbageCollector(mCache) {	
		}

		inline GeometryCacheSystem(Graphics& graphics) :
			GeometryCacheSystem(graphics.Device()) {
		}

		Future<Geometry*> Load(const LoadParams<Geometry>& params, ITaskQueue* queue) override;

		Task Startup(SystemCollection& systems) override;
		bool IsInitialized() const override;
		void Shutdown() override;
		void NewFrame(Frame* frame) override;
		void OnAddedTo(SystemCollection& collection) override;
	};
}