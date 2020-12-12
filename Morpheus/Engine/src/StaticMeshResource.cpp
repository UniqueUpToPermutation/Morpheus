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
		IResource(manager),
		mMaterial(nullptr),
		mGeometry(nullptr) {
	}

	StaticMeshResource::StaticMeshResource(ResourceManager* manager, 
		MaterialResource* material,
		GeometryResource* geometry) :
		IResource(manager) {
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

	IResource* ResourceCache<StaticMeshResource>::Load(const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<StaticMeshResource>*>(params);
		
		auto it = mResourceMap.find(params_cast->mSource);

		if (it != mResourceMap.end()) {
			return it->second;
		} else {
			StaticMeshResource* resource = new StaticMeshResource(mManager);
			mLoader.Load(params_cast->mSource, resource);
			mResourceMap[params_cast->mSource] = resource;
			return resource;
		}
	}

	IResource* ResourceCache<StaticMeshResource>::DeferredLoad(const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<StaticMeshResource>*>(params);
		
		auto it = mResourceMap.find(params_cast->mSource);

		if (it != mResourceMap.end()) {
			return it->second;
		} else {
			StaticMeshResource* resource = new StaticMeshResource(mManager);
			mDeferredResources.emplace_back(std::make_pair(resource, *params_cast));
			mResourceMap[params_cast->mSource] = resource;
			return resource;
		}
	}

	void ResourceCache<StaticMeshResource>::ProcessDeferred() {
		for (auto resource : mDeferredResources) {
			mLoader.Load(resource.second.mSource, resource.first);
		}

		mDeferredResources.clear();
	}

	void ResourceCache<StaticMeshResource>::Add(IResource* resource, const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<StaticMeshResource>*>(params);
		
		auto it = mResourceMap.find(params_cast->mSource);

		auto staticMeshResource = resource->ToStaticMesh();

		if (it != mResourceMap.end()) {
			if (it->second != staticMeshResource)
				Unload(it->second);
			else
				return;
		} 

		mResourceMap[params_cast->mSource] = staticMeshResource;
	}

	void ResourceCache<StaticMeshResource>::Unload(IResource* resource) {
		auto mesh = resource->ToStaticMesh();

		auto it = mResourceMap.find(mesh->GetSource());
		if (it != mResourceMap.end()) {
			if (it->second == mesh) {
				mResourceMap.erase(it);
			}
		}

		delete resource;
	}

	void ResourceCache<StaticMeshResource>::Clear() {
		for (auto& item : mResourceMap) {
			item.second->ResetRefCount();
			delete item.second;
		}

		mResourceMap.clear();
	}
}