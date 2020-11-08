#pragma once

#include <Engine/Resource.hpp>
#include <Engine/Engine.hpp>
#include <Engine/Geometry.hpp>
#include <unordered_map>

namespace DG = Diligent;

namespace Assimp {
	class Importer;
}

namespace Morpheus {
	class GeometryResource : public Resource {
	private:
		DG::IBuffer* mVertexBuffer;
		DG::IBuffer* mIndexBuffer;

		uint mVertexBufferOffset;

		PipelineResource* mPipeline;

		DG::DrawIndexedAttribs mIndexedAttribs;
		DG::DrawAttribs mDrawAttribs;

		BoundingBox mBoundingBox;

		std::string mSource;

	public:

		GeometryResource(ResourceManager* manager,
			DG::IBuffer* vertexBuffer, DG::IBuffer* indexBuffer,
			uint vertexBufferOffset, PipelineResource* pipeline, 
			const BoundingBox& aabb);
		~GeometryResource();

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
		Assimp::Importer* mImporter;
		ResourceManager* mManager;

	public:
		GeometryLoader(ResourceManager* manager);
		~GeometryLoader();

		GeometryResource* Load(const std::string& source, PipelineResource* pipeline);
	};

	template <>
	class ResourceCache<GeometryResource> : public IResourceCache {
	private:
		std::unordered_map<std::string, GeometryResource*> mResources;
		ResourceManager* mManager;
		GeometryLoader mLoader;

	public:
		ResourceCache(ResourceManager* manager);
		~ResourceCache();

		Resource* Load(const void* params) override;
		void Add(Resource* resource, const void* params) override;
		void Unload(Resource* resource) override;
		void Clear() override;
	};
}