#pragma once

#include <Engine/Resource.hpp>

#include <nlohmann/json.hpp>

namespace Morpheus {
	class StaticMeshResource : public IResource {
	private:
		MaterialResource* mMaterial;
		GeometryResource* mGeometry;
		std::string mSource;

		void Init(MaterialResource* material,
			GeometryResource* geometry);

	public:
		entt::id_type GetType() const override;

		StaticMeshResource(ResourceManager* manager);
		StaticMeshResource(ResourceManager* manager, 
			MaterialResource* material,
			GeometryResource* geometry);

		~StaticMeshResource();

		inline bool IsReady() const {
			return mMaterial != nullptr && mGeometry != nullptr;
		}

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

		void Load(const std::string& source, StaticMeshResource* loadinto);
		void Load(const nlohmann::json& item, const std::string& path, StaticMeshResource* loadinto);
	};

	template <>
	class ResourceCache<StaticMeshResource> : public IResourceCache {
	private:
		std::unordered_map<std::string, StaticMeshResource*> mResourceMap;
		ResourceManager* mManager;
		StaticMeshLoader mLoader;
		std::vector<std::pair<StaticMeshResource*, LoadParams<StaticMeshResource>>> mDeferredResources;

	public:
		inline ResourceCache(ResourceManager* manager) :
			mLoader(manager), 
			mManager(manager) {
		}

		IResource* Load(const void* params) override;
		IResource* DeferredLoad(const void* params) override;
		void ProcessDeferred() override;
		void Add(IResource* resource, const void* params) override;
		void Unload(IResource* resource) override;
		void Clear() override;
	};
}