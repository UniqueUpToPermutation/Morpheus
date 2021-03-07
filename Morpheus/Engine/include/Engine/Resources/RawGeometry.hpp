#pragma once

#include "GraphicsTypes.h"
#include "RenderDevice.h"
#include "DeviceContext.h"

#include <vector>

#include <Engine/Geometry.hpp>
#include <Engine/Resources/Resource.hpp>

namespace DG = Diligent;

namespace Assimp {
	class Importer;
}

struct aiScene;

namespace Morpheus {
	class GeometryResource;

	template <>
	struct LoadParams<GeometryResource> {
		// Geometry resource will be loaded with this layout
		VertexLayout mVertexLayout;
		// Geometry resource will be loaded from this file
		std::string mSource;
		// If this is not null, the pipeline's vertex layout will be used instead of mVertexLayout
		PipelineResource* mPipeline = nullptr;
		// If this is not null, the material's vertex layout will be used instead of mVertexLayout
		MaterialResource* mMaterial = nullptr;

		static LoadParams<GeometryResource> FromString(const std::string& str) {
			LoadParams<GeometryResource> params;
			params.mSource = str;
			return params;
		}
	};

	class RawGeometry {
	private:
		DG::BufferDesc mVertexBufferDesc;
		DG::BufferDesc mIndexBufferDesc;

		DG::DrawAttribs mUnindexedDrawAttribs;
		DG::DrawIndexedAttribs mIndexedDrawAttribs;

		std::vector<uint8_t> mVertexBufferData;
		std::vector<uint8_t> mIndexBufferData;

		VertexLayout mLayout;
		BoundingBox mAabb;
		bool bHasIndexBuffer;

		TaskBarrier mLoadBarrier;

		void LoadAssimp(const aiScene* scene, const VertexLayout& vertexLayout);

	public:
		void CopyTo(RawGeometry* geometry) const;
		void CopyFrom(const RawGeometry& geometry);

		inline const std::vector<uint8_t>& GetVertexData() const {
			return mVertexBufferData;
		}

		inline const std::vector<uint8_t>& GetIndexData() const {
			return mIndexBufferData;
		}

		inline const DG::BufferDesc& GetVertexDesc() const {
			return mVertexBufferDesc;
		}

		inline const DG::BufferDesc& GetIndexDesc() const {
			return mIndexBufferDesc;
		}

		inline const DG::DrawAttribs& GetDrawAttribs() const {
			return mUnindexedDrawAttribs;
		}

		inline const DG::DrawIndexedAttribs& GetIndexedDrawAttribs() const {
			return mIndexedDrawAttribs;
		}

		inline const VertexLayout& GetLayout() const {
			return mLayout;
		}

		inline const BoundingBox& GetBoundingBox() const {
			return mAabb;
		}

		inline const bool HasIndexBuffer() const {
			return bHasIndexBuffer;
		}

		inline RawGeometry() {
		}
		RawGeometry(const RawGeometry& other) = delete;
		RawGeometry(RawGeometry&& other);

		void Set(const VertexLayout& layout,
			const DG::BufferDesc& vertexBufferDesc, 
			std::vector<uint8_t>&& vertexBufferData,
			const DG::DrawAttribs& unindexedDrawAttribs,
			const BoundingBox& aabb);

		void Set(const VertexLayout& layout,
			const DG::BufferDesc& vertexBufferDesc,
			const DG::BufferDesc& indexBufferDesc,
			std::vector<uint8_t>&& vertexBufferData,
			std::vector<uint8_t>&& indexBufferData,
			DG::DrawIndexedAttribs& indexedDrawAttribs,
			const BoundingBox& aabb);

		inline RawGeometry(const VertexLayout& layout,
		const DG::BufferDesc& vertexBufferDesc, 
			std::vector<uint8_t>&& vertexBufferData,
			const DG::DrawAttribs& unindexedDrawAttribs,
			const BoundingBox& aabb) {
			Set(layout,
				vertexBufferDesc, std::move(vertexBufferData), 
				unindexedDrawAttribs, aabb);
		}

		inline RawGeometry(const VertexLayout& layout,
			const DG::BufferDesc& vertexBufferDesc,
			const DG::BufferDesc& indexBufferDesc,
			std::vector<uint8_t>&& vertexBufferData,
			std::vector<uint8_t>&& indexBufferData,
			DG::DrawIndexedAttribs& indexedDrawAttribs,
			const BoundingBox& aabb);

		void SpawnOnGPU(DG::IRenderDevice* device, 
			DG::IBuffer** vertexBufferOut, 
			DG::IBuffer** indexBufferOut);

		void SpawnOnGPU(DG::IRenderDevice* device,
			GeometryResource* writeTo);

		TaskId LoadArchiveAsyncDeferred(const std::string& path, ThreadPool* pool);
		TaskId LoadAssimpAsyncDeferred(ThreadPool* pool, const LoadParams<GeometryResource>& params);

		void LoadAssimp(const LoadParams<GeometryResource>& params);
		void LoadAssimpAsync(ThreadPool* pool, const LoadParams<GeometryResource>& params);

		void LoadArchive(const uint8_t* rawArchive, const size_t length);
		void LoadArchive(const std::string& source);
		void LoadArchiveAsync(const std::string& source, ThreadPool* pool);

		void Load(const LoadParams<GeometryResource>& params);
		void LoadAsync(const LoadParams<GeometryResource>& params, ThreadPool* pool);
		TaskId LoadAsyncDeferred(const LoadParams<GeometryResource>& params, ThreadPool* pool);

		inline void Load(const std::string& source) {
			Load(LoadParams<GeometryResource>::FromString(source));
		}

		inline void LoadAsync(const std::string& source, ThreadPool* pool) {
			LoadAsync(LoadParams<GeometryResource>::FromString(source), pool);
		}

		inline TaskId LoadAsyncDeferred(const std::string& source, ThreadPool* pool) {
			return LoadAsyncDeferred(LoadParams<GeometryResource>::FromString(source), pool);
		}

		void Save(const std::string& destination);
		void Clear();

		inline RawGeometry(const std::string& source) {
			Load(source);
		}

		inline RawGeometry(const std::string& source, const VertexLayout& layout) {
			LoadParams<GeometryResource> params;
			params.mSource = source;
			params.mVertexLayout = layout;
			Load(params);
		}

		inline RawGeometry(const LoadParams<GeometryResource>& params) {
			Load(params);
		}

		TaskBarrier* GetLoadBarrier() {
			return &mLoadBarrier;
		}
	};
}