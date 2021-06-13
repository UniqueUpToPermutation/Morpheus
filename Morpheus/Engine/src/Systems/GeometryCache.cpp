#include <Engine/Systems/GeometryCache.hpp>

namespace Morpheus {

	GeometryCacheSystem::loader_t::cache_load_t 
	GeometryCacheSystem::GetLoaderFunction() {
		return [device = mDevice](const LoadParams<Geometry>& params) {
			return Geometry::Load(device, params);
		};
	}
	
	GeometryCacheSystem::loader_t::load_callback_t
	GeometryCacheSystem::GetLoadCallback() {
		return [this](const typename cache_t::iterator_t& it) {
			GarbageCollector().OnResourceLoaded(it);
		};
	}

	Future<Geometry*> GeometryCacheSystem::Load(
		const LoadParams<Geometry>& params, ITaskQueue* queue) {
		return mLoader.Load(params, &mCache, queue);
	}

	Task GeometryCacheSystem::Startup(SystemCollection& systems) {

		mFormatProvider = systems.QueryInterface<IVertexFormatProvider>();
		if (!mFormatProvider) {
			throw std::runtime_error(
				"Geometry cache cannot be created without an IVertexFormatProvider!");
		}

		ParameterizedTask<UpdateParams> update([this](const TaskParams& e, const UpdateParams& params) {
			mLoader.Update();
			mGarbageCollector.CollectGarbage(e);
		}, 
		"Update Geometry Cache",
		TaskType::UPDATE);

		systems.GetFrameProcessor().AddUpdateTask(std::move(update));

		return Task();
	}

	bool GeometryCacheSystem::IsInitialized() const {
		return true;
	}

	void GeometryCacheSystem::Shutdown() {
		mGarbageCollector = gc_t(mCache);
		mLoader.Clear();
		mCache.Clear();
	}

	void GeometryCacheSystem::NewFrame(Frame* frame) {
	}

	void GeometryCacheSystem::OnAddedTo(SystemCollection& collection) {
		collection.AddCacheInterface<Geometry>(this);
	}
}