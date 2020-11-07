#pragma once

#include <Engine/Resource.hpp>
#include <Engine/Engine.hpp>
#include <unordered_map>

namespace DG = Diligent;

namespace Morpheus {
	class GeometryResource : public Resource {
	private:
		DG::IBuffer* mVertexBuffer;
		DG::IBuffer* mIndexBuffer;

		uint mVertexBufferOffset;

		PipelineResource* mPipeline;

		DG::DrawIndexedAttribs mIndexedAttribs;
		DG::DrawAttribs mDrawAttribs;

	public:

		GeometryResource();
		~GeometryResource();

		inline DG::IBuffer* GetVertexBuffer() {
			return mVertexBuffer;
		}

		inline DG::IBuffer* GetIndexBuffer() {
			return mIndexBuffer;
		}

		inline uint GetVertexBufferOffset() {
			return mVertexBufferOffset;
		}

		inline PipelineResource* GetPipeline() {
			return mPipeline;
		}

		inline DG::DrawIndexedAttribs GetIndexedDrawAttribs() {
			return mIndexedAttribs;
		}

		inline DG::DrawAttribs GetDrawAttribs() {
			return mDrawAttribs;
		}
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

	template <>
	class ResourceCache<GeometryResource> : public IResourceCache {
	private:
		std::unordered_map<std::string, GeometryResource*> mResources;

	public:
		Resource* Load(const void* params) override;
		void Add(Resource* resource, const void* params) override;
		void Unload(Resource* resource) override;
	};
}