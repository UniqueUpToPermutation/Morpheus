#pragma once

#include <vector>
#include <unordered_map>

#include <Engine/Resources/Resource.hpp>
#include <Engine/Resources/ShaderLoader.hpp>
#include <Engine/Resources/EmbeddedFileLoader.hpp>

namespace Morpheus {

	class Engine;
	
	class ResourceManager {
	private:
		std::unordered_map<entt::id_type, IResourceCache*> mResourceCaches;
		std::vector<IResource*> mDisposalList;
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
		inline T* Load(const LoadParams<T>& params) {
			auto resource_id = resource_type::type<T>;
			auto it = mResourceCaches.find(resource_id);

			if (it == mResourceCaches.end()) {
				throw std::runtime_error("Could not find resource cache for resource type!");
			}

			auto result = it->second->Load(&params);
			result->AddRef(); // Increment references

			return result->template To<T>();
		}

		template <typename T>
		inline T* Load(const std::string& source) {
			auto params = LoadParams<T>::FromString(source);
			return Load<T>(params);
		}

		template <typename T>
		inline T* AsyncLoad(const LoadParams<T>& params, 
			const TaskBarrierCallback& callback = nullptr) {
			auto resource_id = resource_type::type<T>;
			auto it = mResourceCaches.find(resource_id);

			if (it == mResourceCaches.end()) {
				throw std::runtime_error("Could not find resource cache for resource type!");
			}

			auto result = it->second->AsyncLoad(&params, mThreadPool, callback);
			result->AddRef(); // Increment references

			return result->template To<T>();
		}

		template <typename T>
		inline T* AsyncLoad(const std::string& source, 
			const TaskBarrierCallback& callback = nullptr) {
			auto params = LoadParams<T>::FromString(source);
			return AsyncLoad<T>(params, callback);
		}

		template <typename T>
		inline TaskId AsyncLoadDeferred(const LoadParams<T>& params,
			T** output,
			const TaskBarrierCallback& callback = nullptr) {
			auto resource_id = resource_type::type<T>;
			auto it = mResourceCaches.find(resource_id);

			if (it == mResourceCaches.end()) {
				throw std::runtime_error("Could not find resource cache for resource type!");
			}

			IResource* resource = nullptr;
			auto task = it->second->AsyncLoadDeferred(&params, mThreadPool, &resource, callback);
			resource->AddRef(); // Increment References

			*output = resource->template To<T>();

			return task;
		}

		template <typename T>
		inline TaskId AsyncLoad(const std::string& source,
			T** output,
			const TaskBarrierCallback& callback = nullptr) {
			auto params = LoadParams<T>::FromString(source);
			return AsyncLoad<T>(params, output, callback);
		}

		inline void RequestUnload(IResource* resource) {
			mDisposalList.emplace_back(resource);
		}

		void CollectGarbage();

		inline Engine* GetParent() {
			return mParent;
		}

		inline EmbeddedFileLoader* GetEmbededFileLoader() {
			return &mEmbeddedFileLoader;
		}

		void LoadMesh(const std::string& geometrySource,
			const std::string& materialSource,
			GeometryResource** geometryResourceOut,
			MaterialResource** materialResourceOut);
	};
}