#pragma once

#include <Engine/Resource.hpp>

#include <nlohmann/json.hpp>

namespace Morpheus {
	class StaticMeshResource : public Resource {
	private:
		MaterialResource* mMaterial;
		GeometryResource* mGeometry;
		std::string mSource;

	public:
		entt::id_type GetType() const override;

		StaticMeshResource(ResourceManager* manager, 
			MaterialResource* material,
			GeometryResource* geometry) : 
			Resource(manager),
			mMaterial(material),
			mGeometry(geometry) {
		}

		~StaticMeshResource();

		inline MaterialResource* GetMaterial() {
			return mMaterial;
		}

		inline GeometryResource* GetGeometry() {
			return mGeometry;
		}

		StaticMeshResource* ToStaticMesh() override;

		std::string GetSource() {
			return mSource;
		}

		friend class StaticMeshLoader;
		friend class ResourceCache<StaticMeshResource>;
	};

	template <>
	struct LoadParams<StaticMeshResource> {
		std::string mSource;

		static inline LoadParams<StaticMeshResource> FromString(const std::string& s) {
			LoadParams<StaticMeshResource> p;
			p.mSource = s;
			return p;
		}
	};

	class StaticMeshLoader {
	private:
		ResourceManager* mManager;

	public:
		inline StaticMeshLoader(ResourceManager* manager) :
			mManager(manager) {
		}

		StaticMeshResource* Load(const std::string& source);
		StaticMeshResource* Load(const nlohmann::json& item, const std::string& path);
	};

	template <>
	class ResourceCache<StaticMeshResource> : public IResourceCache {
	private:
		std::unordered_map<std::string, StaticMeshResource*> mResources;
		ResourceManager* mManager;
		StaticMeshLoader mLoader;

	public:
		inline ResourceCache(ResourceManager* manager) :
			mLoader(manager), 
			mManager(manager) {
		}

		Resource* Load(const void* params) override;
		void Add(Resource* resource, const void* params) override;
		void Unload(Resource* resource) override;
		void Clear() override;
	};
}