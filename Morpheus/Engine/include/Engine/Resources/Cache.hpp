#pragma once

#include <Engine/Resources/Resource.hpp>

#include <shared_mutex>

namespace Morpheus {

	class ResourceCache {
	private:
		std::shared_mutex mMutex;
		std::unordered_map<UniversalIdentifier,
			Handle<IResource>,
			UniversalIdentifier::Hasher> mCache;

	public:
		bool Contains(const UniversalIdentifier& id) const;

		void Add(Handle<IResource> resource);
		void Add(Handle<Frame> resource);

		template <typename T>
		Handle<T> FindAs(const UniversalIdentifier& id) {
			{
				std::shared_lock<std::shared_mutex> lock(mMutex);
				auto it = mCache.find(id);

				if (it != mCache.end()) {
					if (entt::resolve<T>() != it->second->GetType()) {
						throw std::runtime_error("Requested type does not match object in cache!");
					}

					return it->second.TryCast<T>();
				}
			}
			return nullptr;
		}

		Handle<IResource> FindOrEmplace(const UniversalIdentifier& id,
			Handle<IResource> handle,
			bool* was_emplaced = nullptr) {
			{
				std::unique_lock<std::shared_mutex> lock(mMutex);
				auto it = mCache.find(id);
				if (it != mCache.end()) {
					if (was_emplaced)
						*was_emplaced = false;
					return it->second;
				} else {
					mCache.emplace_hint(it, id, handle);
					if (was_emplaced)
						*was_emplaced = true;
					return handle;
				}
			}
		}

		template <typename T>
		Handle<IResource> FindOrEmplace(const UniversalIdentifier& id,
			T&& resource,
			bool* was_emplaced = nullptr) {
			
			return FindOrEmplace(id, 
				Handle<T>(std::move(resource)).template DownCast<IResource>(), 
				was_emplaced);
		}

		Handle<IResource> Find(const UniversalIdentifier& id) {
			{
				std::shared_lock<std::shared_mutex> lock(mMutex);
				auto it = mCache.find(id);
				if (it != mCache.end()) {
					return it->second;
				} else {
					return nullptr;
				}
			}
		}

		template <typename T>
		Handle<T> FindOrCreateAs(const UniversalIdentifier& id) {
			auto h = FindAs<T>(id);

			if (h) return h;

			{
				std::unique_lock<std::shared_mutex> lock(mMutex);
				Handle<T> h(T());
				mCache[id] = h;
				return h;
			}
		}

		void Release(Handle<IResource> obj) {
			{
				std::unique_lock<std::shared_mutex> lock(mMutex);
				auto it = mCache.find(obj->GetUniversalId());

				if (it != mCache.end()) {
					mCache.erase(it);
				}
			}
		}
	};
}