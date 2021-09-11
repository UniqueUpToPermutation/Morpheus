#include <Engine/Resources/Cache.hpp>
#include <Engine/Frame.hpp>

namespace Morpheus {

	bool ResourceCache::Contains(const UniversalIdentifier& path) const {
		return mCache.find(path) != mCache.end();
	}

	void ResourceCache::Add(Handle<IResource> resource) {
		mCache[resource->GetUniversalId()] = resource;
	}

	void ResourceCache::Add(Handle<Frame> resource) {
		mCache[resource->GetUniversalId()] = resource.DownCast<IResource>();

		auto frame = dynamic_cast<Frame*>(resource.Ptr());

		if (frame) {
			const auto& resources = frame->GetEntityToResources();

			for (auto it = frame->GetNamesBegin(); it != frame->GetNamesEnd(); ++it) {
				auto resourceIt = resources.find(it->second);
				if (resourceIt != resources.end()) {
					mCache[resourceIt->second->GetUniversalId()] = resourceIt->second;
				}
			}
		}
	}
}