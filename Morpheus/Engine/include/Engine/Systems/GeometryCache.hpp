#pragma once

#include <Engine/Resources/Geometry.hpp>
#include <Engine/Resources/ResourceCache.hpp>
#include <Engine/Systems/System.hpp>
#include <Engine/Graphics.hpp>

namespace Morpheus {
	class GeometryCacheSystem : public ISystem, 
		public IResourceCache<Geometry> {
	private:
		using cache_t = ResourceCache<Handle<Geometry>, 
			Geometry::LoadParameters,
			Geometry::LoadParameters::Hasher>;
		using loader_t = DefaultLoader<Handle<Geometry>, 
			Geometry::LoadParameters,
			Geometry::LoadParameters::Hasher>;
		using gc_t = DefaultGarbageCollector<Handle<Geometry>, 
			Geometry::LoadParameters,
			Geometry::LoadParameters::Hasher>;

		loader_t::cache_load_t GetLoaderFunction();
		loader_t::load_callback_t GetLoadCallback();

		Device mDevice;
		IVertexFormatProvider* mFormatProvider;

		cache_t mCache;
		loader_t mLoader;
		gc_t mGarbageCollector;

	public:
		inline IVertexFormatProvider* GetFormatProvider() const {
			return mFormatProvider;
		}

		inline Device GetDevice() const {
			return mDevice;
		}

		inline cache_t& Cache() { return mCache; }
		inline loader_t& Loader() { return mLoader; }
		inline gc_t& GarbageCollector() { return mGarbageCollector; } 

		inline GeometryCacheSystem(Device device) : 
			mDevice(device), mLoader(GetLoaderFunction(), 
				GetLoadCallback()), mGarbageCollector(mCache) {	
		}

		inline GeometryCacheSystem(RealtimeGraphics& graphics) :
			GeometryCacheSystem(graphics.Device()) {
		}

		Future<Handle<Geometry>> Load(const LoadParams<Geometry>& params, IComputeQueue* queue) override;

		std::unique_ptr<Task> Startup(SystemCollection& systems) override;
		bool IsInitialized() const override;
		void Shutdown() override;
		void NewFrame(Frame* frame) override;
		void OnAddedTo(SystemCollection& collection) override;
	};
}