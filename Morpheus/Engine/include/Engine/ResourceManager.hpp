#pragma once

#include <vector>
#include <unordered_map>

#include <Engine/Resource.hpp>
#include <Engine/ShaderLoader.hpp>

namespace Morpheus {

	class Engine;

	class ResourceManager {
	private:
		std::unordered_map<entt::id_type, IResourceCache*> mResourceCaches;
		std::vector<Resource*> mDisposalList;
		ShaderPreprocessorConfig mShaderPreprocessorConfig;

		Engine* mParent;

	public:

		ResourceManager(Engine* parent);
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
		inline T* DeferredLoad(const LoadParams<T>& params) {
			auto resource_id = resource_type::type<T>;
			auto it = mResourceCaches.find(resource_id);

			if (it == mResourceCaches.end()) {
				throw std::runtime_error("Could not find resource cache for resource type!");
			}

			auto result = it->second->DeferredLoad(&params);
			result->AddRef(); // Increment references

			return result->template To<T>();
		}

		template <typename T>
		inline T* DeferredLoad(const std::string& source) {
			auto params = LoadParams<T>::FromString(source);
			return DeferredLoad<T>(params);
		}

		inline void ProcessDeferred() {
			for (auto cache : mResourceCaches) {
				cache.second->ProcessDeferred();
			}
		}

		inline void RequestUnload(Resource* resource) {
			mDisposalList.emplace_back(resource);
		}

		void CollectGarbage();

		inline Engine* GetParent() {
			return mParent;
		}
	};
}