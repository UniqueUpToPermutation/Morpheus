#pragma once
#include <Engine/Entity.hpp>
#include <Engine/RefCountComponent.hpp>

namespace Morpheus {
	class TextureResource;
	class PipelineResource;
	class MaterialResource;
	class GeometryResource;
	class ResourceManager;

	typedef RefCountComponent<TextureResource> TextureComponent;
	typedef RefCountComponent<PipelineResource> PipelineComponent;
	typedef RefCountComponent<MaterialResource> MaterialComponent;
	typedef RefCountComponent<GeometryResource> GeometryComponent;

	using resource_type = 
		entt::identifier<
			TextureResource,
			PipelineResource,
			MaterialResource,
			GeometryResource>;

	class IResource;

	class IResourceCache {
	public:
		virtual IResource* Load(const void* params) = 0;
		virtual IResource* DeferredLoad(const void* params) = 0;
		virtual void ProcessDeferred() = 0;
		virtual void Add(IResource* resource, const void* params) = 0;
		virtual void Unload(IResource* resource) = 0;
		virtual void Clear() = 0;
		virtual ~IResourceCache() {
		}
	};

	template <typename T>
	class ResourceCache;

	template <typename T>
	struct LoadParams;

	template <typename T>
	struct ResourceConvert;

	class IResource {
	private:
		unsigned int mRefCount = 0;
		ResourceManager* mManager;

	public:
		IResource(ResourceManager* manager) : 
			mManager(manager) {
		}

		virtual ~IResource() {
			assert(mRefCount == 0);
		}

		inline void AddRef() {
			++mRefCount;
		}

		void Release();
		
		inline void ResetRefCount() {
			mRefCount = 0;
		}

		inline int GetRefCount() const {
			return mRefCount;
		}

		virtual entt::id_type GetType() const = 0;

		virtual PipelineResource* ToPipeline();
		virtual GeometryResource* ToGeometry();
		virtual MaterialResource* ToMaterial();
		virtual TextureResource* ToTexture();

		inline ResourceManager* GetManager() {
			return mManager;
		}

		template <typename T>
		inline T* To() {
			return ResourceConvert<T>::Convert(this);
		}
	};

	template <>
	struct ResourceConvert<PipelineResource> {
		static inline PipelineResource* Convert(IResource* resource) {
			return resource->ToPipeline();
		}
	};

	template <>
	struct ResourceConvert<GeometryResource> {
		static inline GeometryResource* Convert(IResource* resource) {
			return resource->ToGeometry();
		}
	};

	template <>
	struct ResourceConvert<MaterialResource> {
		static inline MaterialResource* Convert(IResource* resource) {
			return resource->ToMaterial();
		}
	};

	template <>
	struct ResourceConvert<TextureResource> {
		static inline TextureResource* Convert(IResource* resource) {
			return resource->ToTexture();
		}
	};
}