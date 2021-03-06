#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/GeometryResource.hpp>
#include <Engine/Resources/TextureResource.hpp>
#include <Engine/Resources/MaterialResource.hpp>
#include <Engine/Resources/ShaderResource.hpp>

namespace Morpheus {
	void ResourceManager::CollectGarbage() {
		while (true) {
			IResource* resource = nullptr;
			
			{
				// Get item
				std::unique_lock<std::shared_mutex> lock(mDisposalListMutex);
				if (mDisposalList.size() == 0)
					break;

				resource = mDisposalList.front();
				mDisposalList.pop();
			}
			
			// Unload the resource
			auto type = resource->GetType();
			auto it = mResourceCaches.find(type);
			if (it != mResourceCaches.end()) {
				it->second->Unload(resource);
			}
			else {
				throw std::runtime_error("Unrecognized resource type!");
			}
		}
	}

	ResourceManager::ResourceManager(Engine* parent, ThreadPool* pool) : mParent(parent), mThreadPool(pool) {
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

		auto shaderCache = new ResourceCache<ShaderResource>(this);

		mResourceCaches[resource_type::type<ShaderResource>] = 
			shaderCache;
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
		geoParams.mVertexLayout = (*materialResourceOut)->GetPipeline()->GetVertexLayout();

		*geometryResourceOut = Load<GeometryResource>(geoParams);
	}
}