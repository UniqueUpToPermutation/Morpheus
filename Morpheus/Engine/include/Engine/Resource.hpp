#pragma once

#include <entt/entt.hpp>

namespace Morpheus {
	class TextureResource;
	class PipelineResource;
	class ShaderResource;
	class MaterialResource;
	class GeometryResource;
	class ResourceManager;

	using resource_type = 
		entt::identifier<
			TextureResource,
			PipelineResource,
			ShaderResource,
			MaterialResource,
			GeometryResource>;

	class Resource;

	class IResourceCache {
	public:
		virtual Resource* Load(const void* params) = 0;
		virtual void Add(Resource* resource, const void* params) = 0;
		virtual void Unload(Resource* resource) = 0;
		virtual ~IResourceCache() {
		}
	};

	template <typename T>
	class ResourceCache;

	template <typename T>
	struct LoadParams;

	class Resource {
	private:
		unsigned int mRefCount = 0;
		ResourceManager* mManager;

	public:
		Resource(ResourceManager* manager) : 
			mManager(manager) {
		}

		virtual ~Resource() {
			assert(mRefCount == 0);
		}

		inline void AddRef() {
			++mRefCount;
		}

		void Release();
		
		inline void ResetRefCount() {
			mRefCount = 0;
		}

		virtual entt::id_type GetType() const = 0;
	};
}