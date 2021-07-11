#pragma once

#include <Engine/Resources/Resource.hpp>
#include <Engine/Resources/ResourceCache.hpp>
#include <Engine/Raytrace/Raytracer.hpp>
#include <Engine/Graphics.hpp>

namespace Assimp {
	class Importer;
}

struct aiScene;

namespace Morpheus {
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

	class Geometry : public IResource {
	private:
		// GPU Resident Stuff
		struct RasterizerAspect {
			Handle<DG::IBuffer> mVertexBuffer;
			Handle<DG::IBuffer> mIndexBuffer;

			uint mVertexBufferOffset = 0;
		} mRasterAspect;

		// CPU Resident Stuff (for staging, but can be moved to GPU)
		struct RawAspect {
			std::vector<DG::BufferDesc> mVertexBufferDescs;
			DG::BufferDesc mIndexBufferDesc;

			std::vector<std::vector<uint8_t>> mVertexBufferDatas;
			std::vector<uint8_t> mIndexBufferData;

			bool bHasIndexBuffer;
		} mRawAspect;

		// Stuff resident on raytracing device
		struct RaytracerAspect {
			std::unique_ptr<Raytrace::IShape> mShape;
		} mRtAspect;

		struct SharedAspect {
			DG::DrawIndexedAttribs mIndexedAttribs;
			DG::DrawAttribs mUnindexedAttribs;
			VertexLayout mLayout;
			BoundingBox mBoundingBox;
		} mShared;

		TaskBarrier mBarrier;
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

		void LoadAssimpRaw(const aiScene* scene,
			const VertexLayout& vertexLayout);

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
		void CreateRasterAspect(DG::IRenderDevice* device, const Geometry* source);
		void CreateRaytraceAspect(Raytrace::IRaytraceDevice* device, const Geometry* source);
		void CreateDeviceAspect(GraphicsDevice device, const Geometry* source);

		inline void CreateRasterAspect(DG::IRenderDevice* device) {
			CreateRasterAspect(device, this);
		}
		inline void CreateRaytraceAspect(Raytrace::IRaytraceDevice* device) {
			CreateRaytraceAspect(device, this);
		}
		inline void CreateDeviceAspect(GraphicsDevice device) {
			CreateDeviceAspect(device);
		}
		inline void To(GraphicsDevice device, Geometry* out) {
			out->CreateDeviceAspect(device, this);
		}
		inline Geometry To(GraphicsDevice device) {
			Geometry geo;
			To(device, &geo);
			return geo;
		}

		// -------------------------------------------------------------
		// Geometry IO
		// -------------------------------------------------------------
		Task LoadAssimpRawTask(const LoadParams<Geometry>& params);
		inline void LoadAssimpRaw(const LoadParams<Geometry>& params) {
			LoadAssimpRawTask(params)();
		}

		Task LoadRawTask(const LoadParams<Geometry>& params);
		inline void LoadRaw(const LoadParams<Geometry>& params) {
			LoadRawTask(params)();
		}

		inline void LoadRaw(const std::string& source) {
			LoadRaw(source);
		}
		
		void SpawnOnGPU(DG::IRenderDevice* device, 
			DG::IBuffer** vertexBufferOut, 
			DG::IBuffer** indexBufferOut) const;

		static ResourceTask<Geometry*> Load(
			GraphicsDevice device, const LoadParams<Geometry>& params);
		static ResourceTask<Handle<Geometry>> LoadHandle(
			GraphicsDevice device, const LoadParams<Geometry>& params);

		static ResourceTask<Geometry*> Load(const LoadParams<Geometry>& params);
		static ResourceTask<Handle<Geometry>> LoadHandle(const LoadParams<Geometry>& params);

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

		// -------------------------------------------------------------
		// Geometry Constructors and Destructors
		// -------------------------------------------------------------
		inline Geometry() {
		}

		Geometry(GraphicsDevice device, const Geometry* geometry) {
			CreateDeviceAspect(device, geometry);
		}

		inline Geometry(GraphicsDevice device, const Geometry& geometry) :
			Geometry(device, &geometry) {
		}

		inline Geometry(DG::IBuffer* vertexBuffer, 
			DG::IBuffer* indexBuffer,
			uint vertexBufferOffset, 
			const DG::DrawIndexedAttribs& attribs,
			const VertexLayout& layout,
			const BoundingBox& aabb) {
			Set(vertexBuffer, indexBuffer, vertexBufferOffset,
				attribs, layout, aabb);
		}

		inline Geometry(DG::IBuffer* vertexBuffer,
			uint vertexBufferOffset,
			const DG::DrawAttribs& attribs,
			const VertexLayout& layout,
			const BoundingBox& aabb)  {
			Set(vertexBuffer, vertexBufferOffset, attribs,
				layout, aabb);
		}

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

		inline Geometry(const VertexLayout& layout,
			std::vector<DG::BufferDesc>&& vertexBufferDescs, 
			std::vector<std::vector<uint8_t>>&& vertexBufferDatas,
			const DG::DrawAttribs& unindexedDrawAttribs,
			const BoundingBox& aabb) {
			Set(layout,
				std::move(vertexBufferDescs), 
				std::move(vertexBufferDatas), 
				unindexedDrawAttribs, aabb);
		}

		inline Geometry(const VertexLayout& layout,
			std::vector<DG::BufferDesc>&& vertexBufferDescs,
			const DG::BufferDesc& indexBufferDesc,
			std::vector<uint8_t>&& vertexBufferData,
			std::vector<uint8_t>&& indexBufferData,
			DG::DrawIndexedAttribs& indexedDrawAttribs,
			const BoundingBox& aabb);

		inline Geometry(const VertexLayout& layout,
			size_t vertex_count,
			size_t index_count,
			uint32_t indices[],
			float positions[],
			float uvs[],
			float normals[],
			float tangents[],
			float bitangents[]) {
			FromMemory(layout, vertex_count, index_count, indices, 
				positions, uvs, normals, tangents, bitangents);
		}

		inline Geometry(const VertexLayout& layout,
			size_t vertex_count,
			float positions[],
			float uvs[],
			float normals[],
			float tangents[],
			float bitangents[]) {
			FromMemory(layout, vertex_count, positions, uvs, 
				normals, tangents, bitangents);	
		}

		void Clear();
		void AdoptData(Geometry&& other);
		void CopyTo(Geometry* geometry) const;
		void CopyFrom(const Geometry& geometry);

		inline Geometry(const Geometry& other) = delete;
		inline Geometry(Geometry&& other) {
			AdoptData(std::move(other));
		}

		Geometry& operator=(const Geometry& other) = delete;
		
		inline Geometry& operator=(Geometry&& other) {
			AdoptData(std::move(other));
			return *this;
		}

		inline Geometry(const std::string& source) {
			LoadRaw(source);
		}

		inline Geometry(const std::string& source, const VertexLayout& layout) {
			LoadParams<Geometry> params(source, layout);
			LoadRaw(params);
		}

		inline Geometry(const LoadParams<Geometry>& params) {
			LoadRaw(params);
		}

		// -------------------------------------------------------------
		// Geometry Properties
		// -------------------------------------------------------------
		inline int GetChannelCount() const {
			return mRawAspect.mVertexBufferDatas.size();
		}

		inline DG::IBuffer* GetVertexBuffer() {
			return mRasterAspect.mVertexBuffer;
		}

		inline DG::IBuffer* GetIndexBuffer() {
			return mRasterAspect.mIndexBuffer;
		}

		inline uint GetVertexBufferOffset() const {
			return mRasterAspect.mVertexBufferOffset;
		}

		inline const std::vector<uint8_t>& GetVertexData(int channel = 0) const {
			assert(!(mFlags & RESOURCE_RAW_ASPECT));
			return mRawAspect.mVertexBufferDatas[channel];
		}

		inline const std::vector<uint8_t>& GetIndexData() const {
			assert(!(mFlags & RESOURCE_RAW_ASPECT));
			return mRawAspect.mIndexBufferData;
		}

		inline const DG::BufferDesc& GetVertexDesc(int channel = 0) const {
			assert(!(mFlags & RESOURCE_RAW_ASPECT));
			return mRawAspect.mVertexBufferDescs[channel];
		}

		inline const VertexLayout& GetLayout() const {
			return mShared.mLayout;
		}

		inline const DG::DrawIndexedAttribs& GetIndexedDrawAttribs() const {
			return mShared.mIndexedAttribs;
		}

		inline const DG::DrawAttribs& GetDrawAttribs() const {
			return mShared.mUnindexedAttribs;
		}

		inline const BoundingBox& GetBoundingBox() const {
			return mShared.mBoundingBox;
		}

		inline TaskBarrier* GetLoadBarrier() {
			return &mBarrier;
		}

		typedef LoadParams<Geometry> LoadParameters;

		struct Prefabs {
			static Geometry MaterialBall(const VertexLayout& layout);
			static Geometry Box(const VertexLayout& layout);
			static Geometry Sphere(const VertexLayout& layout);
			static Geometry BlenderMonkey(const VertexLayout& layout);
			static Geometry Torus(const VertexLayout& layout);
			static Geometry Plane(const VertexLayout& layout);
			static Geometry StanfordBunny(const VertexLayout& layout);
			static Geometry UtahTeapot(const VertexLayout& layout);

			static Geometry MaterialBall(GraphicsDevice device, const VertexLayout& layout);
			static Geometry Box(GraphicsDevice device, const VertexLayout& layout);
			static Geometry Sphere(GraphicsDevice device, const VertexLayout& layout);
			static Geometry BlenderMonkey(GraphicsDevice device, const VertexLayout& layout);
			static Geometry Torus(GraphicsDevice device, const VertexLayout& layout);
			static Geometry Plane(GraphicsDevice device, const VertexLayout& layout);
			static Geometry StanfordBunny(GraphicsDevice device, const VertexLayout& layout);
			static Geometry UtahTeapot(GraphicsDevice device, const VertexLayout& layout);
		};

		friend class RawGeometry;
	};

	void ComputeLayoutProperties(
		size_t vertex_count,
		const VertexLayout& layout,
		std::vector<size_t>& offsets,
		std::vector<size_t>& strides,
		std::vector<size_t>& channel_sizes);
}