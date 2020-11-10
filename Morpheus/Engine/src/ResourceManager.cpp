#include <Engine/ResourceManager.hpp>
#include <Engine/PipelineResource.hpp>
#include <Engine/GeometryResource.hpp>
#include <Engine/TextureResource.hpp>
#include <Engine/MaterialResource.hpp>
#include <Engine/StaticMeshResource.hpp>

namespace Morpheus {
	void ResourceManager::CollectGarbage() {
		for (auto item : mDisposalList) {
			auto type = item->GetType();
			auto it = mResourceCaches.find(type);
			if (it != mResourceCaches.end()) {
				it->second->Unload(item);
			}
			else {
				throw std::runtime_error("Unrecognized resource type!");
			}
		}

		mDisposalList.clear();
	}

	ResourceManager::ResourceManager(Engine* parent) : mParent(parent) {
		auto pipelineCache = new ResourceCache<PipelineResource>(this);

		mResourceCaches[resource_type::type<PipelineResource>] = 
			pipelineCache;

		auto geoCache = new ResourceCache<GeometryResource>(this);

		mResourceCaches[resource_type::type<GeometryResource>] = 
			geoCache;

		auto textureCache = new ResourceCache<TextureResource>(this);

		mResourceCaches[resource_type::type<TextureResource>] = 
			textureCache;

		auto materialCache = new ResourceCache<MaterialResource>(this);

		mResourceCaches[resource_type::type<MaterialResource>] = 
			materialCache;

		auto staticMeshCache = new ResourceCache<StaticMeshResource>(this);

		mResourceCaches[resource_type::type<StaticMeshResource>] = 
			staticMeshCache;
	}

	ResourceManager::~ResourceManager() {
		for (auto it : mResourceCaches) {
			delete it.second;
		}

		mResourceCaches.clear();
	}
}