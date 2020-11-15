#include <Engine/StaticMeshResource.hpp>
#include <Engine/MaterialResource.hpp>
#include <Engine/GeometryResource.hpp>

#include <fstream>

namespace Morpheus {
	StaticMeshResource::~StaticMeshResource() {
		mMaterial->Release();
		mGeometry->Release();
	}

	StaticMeshResource::StaticMeshResource(ResourceManager* manager) :
		Resource(manager),
		mMaterial(nullptr),
		mGeometry(nullptr) {
	}

	StaticMeshResource::StaticMeshResource(ResourceManager* manager, 
		MaterialResource* material,
		GeometryResource* geometry) :
		Resource(manager) {
		Init(material, geometry);
	}

	void StaticMeshResource::Init(MaterialResource* material, GeometryResource* geometry) {
		mMaterial = material;
		mGeometry = geometry;
	}

	entt::id_type StaticMeshResource::GetType() const {
		return resource_type::type<StaticMeshResource>;
	}

	void StaticMeshLoader::Load(const std::string& source, StaticMeshResource* loadinto) {
		std::cout << "Loading " << source << "..." << std::endl;

		std::ifstream stream;
		stream.exceptions(std::ios::failbit | std::ios::badbit);
		stream.open(source);

		nlohmann::json json;
		stream >> json;

		stream.close();

		std::string path = ".";
		auto path_cutoff = source.rfind('/');
		if (path_cutoff != std::string::npos) {
			path = source.substr(0, path_cutoff);
		}

		Load(json, path, loadinto);
		loadinto->mSource = source;
	}

	void StaticMeshLoader::Load(const nlohmann::json& item, const std::string& path, StaticMeshResource* staticMesh) {
		std::string materialSrc;
		item["Material"].get_to(materialSrc);

		MaterialResource* material = mManager->Load<MaterialResource>(materialSrc);

		LoadParams<GeometryResource> params;
		params.mPipelineResource = material->GetPipeline();
		item["Geometry"].get_to(params.mSource);

		GeometryResource* geometry = mManager->Load<GeometryResource>(params);

		staticMesh->Init(material, geometry);
	}

	StaticMeshResource* StaticMeshResource::ToStaticMesh() {
		return this;
	}

	Resource* ResourceCache<StaticMeshResource>::Load(const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<StaticMeshResource>*>(params);
		
		auto it = mResources.find(params_cast->mSource);

		if (it != mResources.end()) {
			return it->second;
		} else {
			StaticMeshResource* resource = new StaticMeshResource(mManager);
			mLoader.Load(params_cast->mSource, resource);
			mResources[params_cast->mSource] = resource;
			return resource;
		}
	}

	Resource* ResourceCache<StaticMeshResource>::DeferredLoad(const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<StaticMeshResource>*>(params);
		
		auto it = mResources.find(params_cast->mSource);

		if (it != mResources.end()) {
			return it->second;
		} else {
			StaticMeshResource* resource = new StaticMeshResource(mManager);
			mDeferredResources.emplace_back(std::make_pair(resource, *params_cast));
			mResources[params_cast->mSource] = resource;
			return resource;
		}
	}

	void ResourceCache<StaticMeshResource>::ProcessDeferred() {
		for (auto resource : mDeferredResources) {
			mLoader.Load(resource.second.mSource, resource.first);
		}

		mDeferredResources.clear();
	}

	void ResourceCache<StaticMeshResource>::Add(Resource* resource, const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<StaticMeshResource>*>(params);
		
		auto it = mResources.find(params_cast->mSource);

		auto staticMeshResource = resource->ToStaticMesh();

		if (it != mResources.end()) {
			if (it->second != staticMeshResource)
				Unload(it->second);
			else
				return;
		} 

		mResources[params_cast->mSource] = staticMeshResource;
	}

	void ResourceCache<StaticMeshResource>::Unload(Resource* resource) {
		auto mesh = resource->ToStaticMesh();

		auto it = mResources.find(mesh->GetSource());
		if (it != mResources.end()) {
			if (it->second == mesh) {
				mResources.erase(it);
			}
		}

		delete resource;
	}

	void ResourceCache<StaticMeshResource>::Clear() {
		for (auto& item : mResources) {
			item.second->ResetRefCount();
			delete item.second;
		}

		mResources.clear();
	}
}