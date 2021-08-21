#pragma once

#include <Engine/ThreadPool.hpp>
#include <Engine/Resources/Resource.hpp>
#include <shared_mutex>

namespace Morpheus {

	template <typename iteratorType, typename LoadParamsHasher>
	struct MetaHasher {
		inline std::size_t operator()(const iteratorType& k) const {
			return LoadParamsHasher()(k->first);
		}
	};

	template <typename T, 
		typename LoadParameters,
		typename LoadParamsHasher>
	class ResourceCache {
	public:
		using map_t = std::unordered_map<LoadParameters, Future<T>, LoadParamsHasher>;
		using iterator_t = typename map_t::iterator;

	private:
		std::shared_mutex mMutex;
		map_t mResourceMap;

	public:
		inline ResourceCache() {
		}

		inline std::unique_lock<std::shared_mutex> LockUnique() {
			return std::unique_lock<std::shared_mutex>(mMutex);
		}

		inline std::shared_lock<std::shared_mutex> LockShared() {
			return std::shared_lock<std::shared_mutex>(mMutex);
		}

		iterator_t Add(const LoadParameters& params, Future<T>&& future) {
			std::unique_lock<std::shared_mutex> lock(mMutex);
			return mResourceMap.emplace(params, std::move(future));
		}

		iterator_t Add(const LoadParameters& params, Future<T>& future) {
			std::unique_lock<std::shared_mutex> lock(mMutex);
			return mResourceMap.emplace(params, future);
		}

		iterator_t Find(const LoadParameters& params) {
			std::shared_lock<std::shared_mutex> lock(mMutex);
			return mResourceMap.find(params);
		}

		iterator_t AddUnsafe(const LoadParameters& params, Future<T>&& future) {
			return mResourceMap.emplace(params, std::move(future)).first;
		}

		iterator_t AddUnsafe(const LoadParameters& params, const Future<T>& future) {
			return mResourceMap.emplace(params, future).first;
		}

		iterator_t FindUnsafe(const LoadParameters& params) {
			return mResourceMap.find(params);
		}

		iterator_t Begin() {
			return mResourceMap.begin();
		}

		iterator_t End() {
			return mResourceMap.end();
		}

		void Clear() {
			mResourceMap.clear();
		}

		void Remove(const LoadParameters& params) {
			std::unique_lock<std::shared_mutex> lock(mMutex);
			mResourceMap.erase(params);
		}

		void Remove(const iterator_t& it) {
			std::unique_lock<std::shared_mutex> lock(mMutex);
			mResourceMap.erase(it);
		}

		void RemoveUnsafe(const LoadParameters& params) {
			mResourceMap.erase(params);
		}

		void RemoveUnsafe(const iterator_t& it) {
			mResourceMap.erase(it);
		}
	};

	template <typename T, 
		typename LoadParameters,
		typename LoadParamsHasher>
	class DefaultLoader {
	public:
		using cache_load_t = std::function<Future<T>(const LoadParameters&)>;
		using cache_t = ResourceCache<T, LoadParameters, LoadParamsHasher>;
		using load_callback_t = std::function<void(const typename cache_t::iterator_t&)>;

	private:
		cache_load_t mLoad;
		load_callback_t mLoadCallback;

		std::unordered_set<typename cache_t::iterator_t,
			MetaHasher<typename cache_t::iterator_t, LoadParamsHasher>> mLoading;
		std::shared_mutex mLoadingMutex;

	public:
		inline DefaultLoader(const cache_load_t& loader,
			const load_callback_t& loadCallback) : 
			mLoad(loader),
			mLoadCallback(loadCallback) {
		}

		Future<T> Load(const LoadParameters& params, 
			cache_t* cache, IComputeQueue* queue) {
			
			{
				auto lock = cache->LockShared();
				auto it = cache->FindUnsafe(params);
				if (it != cache->End()) {
					return it->second;
				}
			}

			Future<T> future = mLoad(params);
			typename cache_t::iterator_t it;

			{
				auto lock = cache->LockUnique();
				queue->Submit(future);
				it = cache->AddUnsafe(params, future);
			}

			{
				std::unique_lock<std::shared_mutex> lock(mLoadingMutex);
				mLoading.emplace(it);
			}

			return future;
		}

		void Update() {
			std::vector<typename decltype(mLoading)::iterator> loadedIt;
			std::vector<typename cache_t::iterator_t> loaded; 

			{
				std::shared_lock<std::shared_mutex> lock(mLoadingMutex);
				for (auto it = mLoading.begin(); it != mLoading.end(); ++it) {
					if ((*it)->second.IsAvailable()) {
						loadedIt.emplace_back(it);
					}
				}
			}

			if (loadedIt.size() > 0) {
				loaded.reserve(loadedIt.size());
				std::unique_lock<std::shared_mutex> lock(mLoadingMutex);
				for (auto& it : loadedIt) {
					loaded.emplace_back(*it);
					mLoading.erase(it);
				}
			}

			if (mLoadCallback) {
				for (auto it : loaded) {
					mLoadCallback(it);
				}
			}
		}

		void Clear() {
			mLoading.clear();
		}
	};

	template <typename T,
		typename LoadParameters,
		typename LoadParamsHasher>
	class DefaultGarbageCollector {
	public:
		using cache_t = ResourceCache<T, LoadParameters, LoadParamsHasher>;
	
	private:
		std::unordered_set<typename cache_t::iterator_t, 
			MetaHasher<typename cache_t::iterator_t, LoadParamsHasher>> mLoadedResources;
		cache_t* mCache;

	public:
		DefaultGarbageCollector(cache_t& cache) : mCache(&cache) {
		}

		inline void OnResourceLoaded(const typename cache_t::iterator_t& iterator) {
			mLoadedResources.emplace(iterator);
		}

		void CollectGarbage(const TaskParams& e) {
			std::queue<typename cache_t::iterator_t> possibleGarbage;
			std::vector<Future<T>> actualGarbage;

			for (auto tex : mLoadedResources) {
				// We own the only future to this object, and there should be one underlying reference
				if (tex->second.RefCount() == 1 && tex->second.Get()->GetRefCount() == 1) {
					possibleGarbage.emplace(tex);
				}
			}

			// There's stuff we need to clean up
			if (possibleGarbage.size() > 0) {
				auto lock = mCache->LockUnique();

				while (!possibleGarbage.empty()) {
					typename cache_t::iterator_t garbage = possibleGarbage.front();
					possibleGarbage.pop();

					// Make sure it is still garbage
					if (garbage->second.RefCount() == 1 && garbage->second.Get()->GetRefCount() == 1) {
						Future<T> f = std::move(garbage->second);
						actualGarbage.emplace_back(std::move(f));
						mCache->RemoveUnsafe(garbage);
					}
				}
			}

			// There's stuff we need to send to thread 1 to dispose of
			if (actualGarbage.size() > 0) {
				if (e.mThread == THREAD_MAIN) {
					// Dispose everything
					actualGarbage.clear();
				} else {
					FunctionPrototype<> prototype([&garbage = actualGarbage](const TaskParams& e) {
						// Dispose everything
						garbage.clear();
					});
					e.mQueue->Submit(prototype().SetName("Dispose Garbage").OnlyThread(THREAD_MAIN));
				}
			}
		}
	};
}