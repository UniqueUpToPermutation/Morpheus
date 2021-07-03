#pragma once

#include "GraphicsTypes.h"
#include "RenderDevice.h"
#include "DeviceContext.h"

#include <vector>

#include <Engine/GeometryStructures.hpp>
#include <Engine/Resources/Resource.hpp>

namespace DG = Diligent;

namespace Assimp {
	class Importer;
}

struct aiScene;

namespace Morpheus {

	enum class GeometryType {
		STATIC_MESH,
		UNSPECIFIED
	};

	class IVertexFormatProvider {
	public:
		virtual const VertexLayout& GetStaticMeshLayout() const = 0;

		inline const VertexLayout& GetLayout(GeometryType type) const {
			switch (type) {
				case GeometryType::STATIC_MESH:
					return GetStaticMeshLayout();
					break;
				default:
					return GetStaticMeshLayout();
					break;
			}
		}
	};

	template <>
	struct LoadParams<Geometry> {
		// Geometry resource will be loaded with this layout
		VertexLayout mVertexLayout;
		// Geometry resource will be loaded from this file
		std::string mSource;
		// Only need to set this if we are loading from a geometry cache
		GeometryType mType = GeometryType::UNSPECIFIED;

		inline LoadParams() {
		}

		inline LoadParams(const std::string& source, const VertexLayout& layout) : 
			mSource(source), mVertexLayout(layout) {
		}

		inline LoadParams(const char* source, const VertexLayout& layout) : 
			mSource(source), mVertexLayout(layout) {
		}

		inline LoadParams(const std::string& source, GeometryType type) :
			mSource(source), mType(type) {
		}

		inline LoadParams(const char* source, GeometryType type) :
			mSource(source), mType(type) {
		}

		bool operator==(const LoadParams<Geometry>& t) const {
			return mSource == t.mSource;
		}

		struct Hasher {
			inline std::size_t operator()(const LoadParams<Geometry>& k) const
			{
				return std::hash<std::string>()(k.mSource);
			}
		};
	};

	class RawGeometry {
	private:
		std::vector<DG::BufferDesc> mVertexBufferDescs;
		DG::BufferDesc mIndexBufferDesc;

		DG::DrawAttribs mUnindexedDrawAttribs;
		DG::DrawIndexedAttribs mIndexedDrawAttribs;

		std::vector<std::vector<uint8_t>> mVertexBufferDatas;
		std::vector<uint8_t> mIndexBufferData;

		VertexLayout mLayout;
		BoundingBox mAabb;
		bool bHasIndexBuffer;

		TaskBarrier mBarrier;
		std::atomic<bool> bIsLoaded;

		void LoadAssimp(const aiScene* scene, const VertexLayout& vertexLayout);

		template <typename I3T, typename V3T, typename V2T>
		void Unpack(const VertexLayout& layout,
			size_t vertex_count,
			size_t index_count,
			const I3T indices[],
			const V3T positions[],
			const V2T uvs[],
			const V3T normals[],
			const V3T tangents[],
			const V3T bitangents[]);

	public:
		inline bool IsLoaded() const {
			return bIsLoaded;
		}

		inline void SetLoaded(bool value) {
			bIsLoaded = value;
		}

		void CopyTo(RawGeometry* geometry) const;
		void CopyFrom(const RawGeometry& geometry);

		inline int GetChannelCount() const {
			return mVertexBufferDatas.size();
		}

		inline const std::vector<uint8_t>& GetVertexData(int channel = 0) const {
			return mVertexBufferDatas[channel];
		}

		inline const std::vector<uint8_t>& GetIndexData() const {
			return mIndexBufferData;
		}

		inline const DG::BufferDesc& GetVertexDesc(int channel = 0) const {
			return mVertexBufferDescs[channel];
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
			bIsLoaded = false;
		}

		RawGeometry(const RawGeometry& other) = delete;
		RawGeometry(RawGeometry&& other);

		RawGeometry& operator=(const RawGeometry& other) = delete;
		RawGeometry& operator=(RawGeometry&& other);

		void Set(const VertexLayout& layout,
			std::vector<DG::BufferDesc>&& vertexBufferDescs, 
			std::vector<std::vector<uint8_t>>&& vertexBufferDatas,
			const DG::DrawAttribs& unindexedDrawAttribs,
			const BoundingBox& aabb);

		void Set(const VertexLayout& layout,
			std::vector<DG::BufferDesc>&& vertexBufferDescs,
			const DG::BufferDesc& indexBufferDesc,
			std::vector<std::vector<uint8_t>>&& vertexBufferDatas,
			std::vector<uint8_t>&& indexBufferData,
			DG::DrawIndexedAttribs& indexedDrawAttribs,
			const BoundingBox& aabb);

		inline RawGeometry(const VertexLayout& layout,
			std::vector<DG::BufferDesc>&& vertexBufferDescs, 
			std::vector<std::vector<uint8_t>>&& vertexBufferDatas,
			const DG::DrawAttribs& unindexedDrawAttribs,
			const BoundingBox& aabb) {
			Set(layout,
				std::move(vertexBufferDescs), 
				std::move(vertexBufferDatas), 
				unindexedDrawAttribs, aabb);
		}

		inline RawGeometry(const VertexLayout& layout,
			std::vector<DG::BufferDesc>&& vertexBufferDescs,
			const DG::BufferDesc& indexBufferDesc,
			std::vector<uint8_t>&& vertexBufferData,
			std::vector<uint8_t>&& indexBufferData,
			DG::DrawIndexedAttribs& indexedDrawAttribs,
			const BoundingBox& aabb);

		void SpawnOnGPU(DG::IRenderDevice* device, 
			DG::IBuffer** vertexBufferOut, 
			DG::IBuffer** indexBufferOut) const;

		void SpawnOnGPU(DG::IRenderDevice* device,
			Geometry* writeTo) const;

		Task LoadAssimpTask(const LoadParams<Geometry>& params);
		inline void LoadAssimp(const LoadParams<Geometry>& params) {
			LoadAssimpTask(params)();
		}

		void LoadArchive(const uint8_t* rawArchive, const size_t length);
		Task LoadArchiveTask(const std::string& source);
		inline void LoadArchive(const std::string& source) {
			LoadArchiveTask(source)();
		}

		Task LoadTask(const LoadParams<Geometry>& params);
		inline void Load(const LoadParams<Geometry>& params) {
			LoadTask(params)();
		}

		inline void Load(const std::string& source) {
			Load(source);
		}

		Task SaveTask(const std::string& destination);
		inline void Save(const std::string& destination) {
			SaveTask(destination)();
		}
		
		void Clear();
		void AdoptData(RawGeometry&& other);

		inline RawGeometry(const std::string& source) {
			Load(source);
		}

		inline RawGeometry(const std::string& source, const VertexLayout& layout) {
			LoadParams<Geometry> params(source, layout);
			Load(params);
		}

		inline RawGeometry(const LoadParams<Geometry>& params) {
			Load(params);
		}

		TaskBarrier* GetLoadBarrier() {
			return &mBarrier;
		}

		void FromMemory(const VertexLayout& layout,
			size_t vertex_count,
			size_t index_count,
			uint32_t indices[],
			float positions[],
			float uvs[],
			float normals[],
			float tangents[],
			float bitangents[]);

		inline void FromMemory(const VertexLayout& layout,
			size_t vertex_count,
			float positions[],
			float uvs[],
			float normals[],
			float tangents[],
			float bitangents[]) {
			FromMemory(layout, vertex_count, 0, nullptr, 
				positions, uvs, normals, tangents, bitangents);
		}

		struct Prefabs {
			static RawGeometry MaterialBall(const VertexLayout& layout);
			static RawGeometry Box(const VertexLayout& layout);
			static RawGeometry Sphere(const VertexLayout& layout);
			static RawGeometry BlenderMonkey(const VertexLayout& layout);
			static RawGeometry Torus(const VertexLayout& layout);
			static RawGeometry Plane(const VertexLayout& layout);
			static RawGeometry StanfordBunny(const VertexLayout& layout);
			static RawGeometry UtahTeapot(const VertexLayout& layout);
		};
	};

	void ComputeLayoutProperties(
		size_t vertex_count,
		const VertexLayout& layout,
		std::vector<size_t>& offsets,
		std::vector<size_t>& strides,
		std::vector<size_t>& channel_sizes);
}