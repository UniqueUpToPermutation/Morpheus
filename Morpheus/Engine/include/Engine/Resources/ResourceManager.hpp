#pragma once

#include <vector>
#include <unordered_map>
#include <shared_mutex>

#include <Engine/Resources/Resource.hpp>
#include <Engine/Resources/ShaderLoader.hpp>
#include <Engine/Resources/EmbeddedFileLoader.hpp>

namespace Morpheus {

	class Engine;
	
	class ResourceManager {
	private:
		std::unordered_map<entt::id_type, IResourceCache*> mResourceCaches;
		std::shared_mutex mDisposalListMutex;
		std::queue<IResource*> mDisposalList;
		ShaderPreprocessorConfig mShaderPreprocessorConfig;
		EmbeddedFileLoader mEmbeddedFileLoader;

		Engine* mParent;
		ThreadPool* mThreadPool;

	public:

		ResourceManager(Engine* parent, ThreadPool* threadPool);
		~ResourceManager();

		template <typename T>
		inline ResourceCache<T>* GetCache() {
			auto it = mResourceCaches.find(resource_type::type<T>);

			if (it != mResourceCaches.end()) {
				return dynamic_cast<ResourceCache<T>*>(it->second);
			}

			return nullptr;
		}

		inline ShaderPreprocessorConfig* GetShaderPreprocessorConfig() {
			return &mShaderPreprocessorConfig;
		}

		template <typename T>
		inline void Add(T* resource, const LoadParams<T>& params) {
			auto resource_id = resource_type::type<T>;
			auto it = mResourceCaches.find(resource_id);

			if (it == mResourceCaches.end()) {
				throw std::runtime_error("Could not find resource cache for resource type!");
			}

			it->second->Add(resource, &params);
		}

		template <typename T>
		inline void Add(T* resource, const std::string& str) {
			Add(resource, LoadParams<T>::FromString(str));
		}

		template <typename T>
		inline Task LoadTask(const LoadParams<T>& params,
			T** output) {
			auto resource_id = resource_type::type<T>;
			auto it = mResourceCaches.find(resource_id);

			if (it == mResourceCaches.end()) {
				throw std::runtime_error("Could not find resource cache for resource type!");
			}

			IResource* resource = nullptr;
			auto task = it->second->LoadTask(&params, &resource);
			resource->AddRef(); // Increment References

			*output = resource->template To<T>();

			return task;
		}

		template <typename T>
		inline Task LoadTask(const std::string& source,
			T** output) {
			auto params = LoadParams<T>::FromString(source);
			return LoadTask<T>(params, output);		
		}

		template <typename T>
		inline T* Load(const LoadParams<T>& params) {
			T* output = nullptr;
			LoadTask(params, &output)();
			return output;
		}

		template <typename T>
		inline T* Load(const std::string& source) {
			T* output = nullptr;
			LoadTask(source, &output)();
			return output;
		}

		inline void RequestUnload(IResource* resource) {
			std::unique_lock lock(mDisposalListMutex);
			mDisposalList.emplace(resource);
		}

		void CollectGarbage();

		inline Engine* GetParent() {
			return mParent;
		}

		inline EmbeddedFileLoader* GetEmbededFileLoader() {
			return &mEmbeddedFileLoader;
		}
	};
}