#pragma once

#include <Engine/Resources/Resource.hpp>
#include <Engine/Engine.hpp>
#include <Engine/Geometry.hpp>
#include <unordered_map>

namespace DG = Diligent;

namespace Assimp {
	class Importer;
}

namespace Morpheus {
	class GeometryResource : public IResource {
	private:
		DG::IBuffer* mVertexBuffer;
		DG::IBuffer* mIndexBuffer;

		uint mVertexBufferOffset;

		PipelineResource* mPipeline;

		DG::DrawIndexedAttribs mIndexedAttribs;
		DG::DrawAttribs mDrawAttribs;

		BoundingBox mBoundingBox;

		std::string mSource;

		void Init(DG::IBuffer* vertexBuffer, DG::IBuffer* indexBuffer,
			uint vertexBufferOffset, PipelineResource* pipeline, 
			const BoundingBox& aabb);

	public:

		GeometryResource(ResourceManager* manager);

		GeometryResource(ResourceManager* manager,
			DG::IBuffer* vertexBuffer, DG::IBuffer* indexBuffer,
			uint vertexBufferOffset, PipelineResource* pipeline, 
			const BoundingBox& aabb);
		~GeometryResource();

		inline bool IsReady() const {
			return mVertexBuffer != nullptr;
		}

		inline DG::IBuffer* GetVertexBuffer() {
			return mVertexBuffer;
		}

		inline DG::IBuffer* GetIndexBuffer() {
			return mIndexBuffer;
		}

		inline uint GetVertexBufferOffset() const {
			return mVertexBufferOffset;
		}

		inline PipelineResource* GetPipeline() {
			return mPipeline;
		}

		inline DG::DrawIndexedAttribs GetIndexedDrawAttribs() const {
			return mIndexedAttribs;
		}

		inline DG::DrawAttribs GetDrawAttribs() const {
			return mDrawAttribs;
		}

		inline BoundingBox GetBoundingBox() const {
			return mBoundingBox;
		}

		entt::id_type GetType() const override {
			return resource_type::type<GeometryResource>;
		}

		inline std::string GetSource() const {
			return mSource;
		}

		GeometryResource* ToGeometry() override;

		friend class GeometryLoader;
		friend class ResourceCache<GeometryResource>;
	};

	template <>
	struct LoadParams<GeometryResource> {
		// If pipeline resource is nullptr, then default to loading from pipeline source
		PipelineResource* mPipelineResource;
		std::string mPipelineSource;
		std::string mSource;

		static LoadParams<GeometryResource> FromString(const std::string& str) {
			throw std::runtime_error("Cannot create geometry resource load params from string!");
		}
	};

	class GeometryLoader {
	private:
		ResourceManager* mManager;

	public:
		GeometryLoader(ResourceManager* manager);
		~GeometryLoader();

		void Load(const std::string& source, PipelineResource* pipeline, 
			GeometryResource* loadinto);
	};

	template <>
	class ResourceCache<GeometryResource> : public IResourceCache {
	private:
		std::unordered_map<std::string, GeometryResource*> mResourceMap;
		ResourceManager* mManager;
		GeometryLoader mLoader;
		std::vector<std::pair<GeometryResource*, LoadParams<GeometryResource>>> mDeferredResources;

	public:
		ResourceCache(ResourceManager* manager);
		~ResourceCache();

		IResource* Load(const void* params) override;
		IResource* DeferredLoad(const void* params) override;
		void ProcessDeferred() override;
		void Add(IResource* resource, const void* params) override;
		void Unload(IResource* resource) override;
		void Clear() override;
	};
}