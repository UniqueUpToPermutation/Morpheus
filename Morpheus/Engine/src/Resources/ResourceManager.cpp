#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/GeometryResource.hpp>
#include <Engine/Resources/TextureResource.hpp>
#include <Engine/Resources/MaterialResource.hpp>

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
	}

	ResourceManager::~ResourceManager() {
		CollectGarbage();

		for (auto it : mResourceCaches) {
			delete it.second;
		}

		mResourceCaches.clear();
	}

	void ResourceManager::LoadMesh(const std::string& geometrySource,
		const std::string& materialSource,
		GeometryResource** geometryResourceOut,
		MaterialResource** materialResourceOut) {

		*materialResourceOut = Load<MaterialResource>(materialSource);

		LoadParams<GeometryResource> geoParams;
		geoParams.mSource = geometrySource;
		geoParams.mPipelineResource = (*materialResourceOut)->GetPipeline();

		*geometryResourceOut = Load<GeometryResource>(geoParams);
	}
}