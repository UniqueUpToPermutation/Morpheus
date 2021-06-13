#pragma once

#include <Engine/Resources/Resource.hpp>
#include <Engine/Resources/RawGeometry.hpp>
#include <Engine/Resources/ResourceCache.hpp>

namespace Morpheus {
	class Geometry : public IResource {
	private:
		DG::IBuffer* mVertexBuffer;
		DG::IBuffer* mIndexBuffer;
		ResourceManagement mManagementScheme;

		uint mVertexBufferOffset;

		VertexLayout mLayout;

		DG::DrawIndexedAttribs mIndexedAttribs;
		DG::DrawAttribs mUnindexedAttribs;

		BoundingBox mBoundingBox;

		ResourceCache<Geometry, 
			LoadParams<Geometry>, 
			LoadParams<Geometry>::Hasher>::iterator_t mCacheIterator;

		void Set(DG::IBuffer* vertexBuffer, 
			DG::IBuffer* indexBuffer,
			uint vertexBufferOffset, 
			const DG::DrawIndexedAttribs& attribs,
			const VertexLayout& layout,
			const BoundingBox& aabb);

		void Set(DG::IBuffer* vertexBuffer,
			uint vertexBufferOffset,
			const DG::DrawAttribs& attribs,
			const VertexLayout& layout,
			const BoundingBox& aabb);

		void LoadAssimp(const aiScene* scene,
			const VertexLayout& vertexLayout);

	public:

		inline Geometry() : mVertexBuffer(nullptr), mIndexBuffer(nullptr) {
		}

		inline Geometry(DG::IBuffer* vertexBuffer, 
			DG::IBuffer* indexBuffer,
			uint vertexBufferOffset, 
			const DG::DrawIndexedAttribs& attribs,
			const VertexLayout& layout,
			const BoundingBox& aabb) : 
			mManagementScheme(ResourceManagement::INTERNAL_UNMANAGED) {
			Set(vertexBuffer, indexBuffer, vertexBufferOffset,
				attribs, layout, aabb);
		}

		inline Geometry(DG::IBuffer* vertexBuffer,
			uint vertexBufferOffset,
			const DG::DrawAttribs& attribs,
			const VertexLayout& layout,
			const BoundingBox& aabb) : 
			mManagementScheme(ResourceManagement::INTERNAL_UNMANAGED) {
			Set(vertexBuffer, vertexBufferOffset, attribs,
				layout, aabb);
		}

		ResourceManagement GetManagementScheme() const {
			return mManagementScheme;
		}

		inline bool IsManaged() const {
			return mManagementScheme == ResourceManagement::FROM_DISK_MANAGED ||
				mManagementScheme == ResourceManagement::INTERNAL_MANAGED;
		}

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

		inline const VertexLayout& GetLayout() const {
			return mLayout;
		}

		inline DG::DrawIndexedAttribs GetIndexedDrawAttribs() const {
			return mIndexedAttribs;
		}

		inline DG::DrawAttribs GetDrawAttribs() const {
			return mUnindexedAttribs;
		}

		inline BoundingBox GetBoundingBox() const {
			return mBoundingBox;
		}

		static ResourceTask<Geometry*> Load(
			DG::IRenderDevice* device, const LoadParams<Geometry>& params);
		static ResourceTask<Handle<Geometry>> LoadHandle(
			DG::IRenderDevice* device, const LoadParams<Geometry>& params);

		typedef LoadParams<Geometry> LoadParameters;

		friend class RawGeometry;
	};
}