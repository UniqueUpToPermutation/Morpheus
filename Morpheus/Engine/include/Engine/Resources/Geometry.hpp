#pragma once

#include <Engine/Resources/Resource.hpp>
#include <Engine/Resources/ResourceCache.hpp>
#include <Engine/Graphics.hpp>
#include <Engine/GeometryStructures.hpp>

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

		inline LoadParams(
			const std::string& source, 
			const VertexLayout& layout) : 
			mSource(source), mVertexLayout(layout) {
		}

		inline LoadParams(
			const char* source, 
			const VertexLayout& layout) : 
			mSource(source), mVertexLayout(layout) {
		}

		inline LoadParams(
			const std::string& source, 
			GeometryType type) :
			mSource(source), mType(type) {
		}

		inline LoadParams(
			const char* source, 
			GeometryType type) :
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
		LoadParams<Geometry> mSource;

		// GPU Resident Stuff
		struct RasterizerAspect {
			Handle<DG::IBuffer> mVertexBuffer;
			Handle<DG::IBuffer> mIndexBuffer;

			uint mVertexBufferOffset = 0;
		} mRasterAspect;

		// CPU Resident Stuff (for staging, but can be moved to GPU)
		struct CpuAspect {
			std::vector<DG::BufferDesc> mVertexBufferDescs;
			DG::BufferDesc mIndexBufferDesc;

			std::vector<std::vector<uint8_t>> mVertexBufferDatas;
			std::vector<uint8_t> mIndexBufferData;

			bool bHasIndexBuffer;
		} mCpuAspect;

		// Stuff resident on an external device
		ExternalAspect<ExtObjectType::GEOMETRY> mExtAspect;

		struct SharedAspect {
			DG::DrawIndexedAttribs mIndexedAttribs;
			DG::DrawAttribs mUnindexedAttribs;
			VertexLayout mLayout;
			BoundingBox mBoundingBox;
		} mShared;

		ResourceCache<Geometry, 
				LoadParams<Geometry>, 
				LoadParams<Geometry>::Hasher>::iterator_t mCacheIterator;

		void Set(DG::IRenderDevice* device,
			DG::IBuffer* vertexBuffer, 
			DG::IBuffer* indexBuffer,
			uint vertexBufferOffset, 
			const DG::DrawIndexedAttribs& attribs,
			const VertexLayout& layout,
			const BoundingBox& aabb);

		void Set(DG::IRenderDevice* device,
			DG::IBuffer* vertexBuffer,
			uint vertexBufferOffset,
			const DG::DrawAttribs& attribs,
			const VertexLayout& layout,
			const BoundingBox& aabb);

		static Geometry ReadAssimpRaw(const aiScene* scene,
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

		// -------------------------------------------------------------
		// Geometry Aspects
		// -------------------------------------------------------------

		void CreateExternalAspect(
			IExternalGraphicsDevice* device, 
			const Geometry* source);

		inline void CreateExternalAspect(IExternalGraphicsDevice* device) {
			CreateExternalAspect(device, this);
		}

		void CreateRasterAspect(
			DG::IRenderDevice* device, 
			const Geometry* source);
		
		void CreateDeviceAspect(
			Device device, 
			const Geometry* source);

		inline void CreateRasterAspect(DG::IRenderDevice* device) {
			CreateRasterAspect(device, this);
		}

		inline void CreateDeviceAspect(Device device) {
			CreateDeviceAspect(device);
		}

	public:

		inline static Geometry CopyToDevice(Device device, const Geometry& in) {
			Geometry geo;
			geo.CreateDeviceAspect(device, &in);
			return geo;
		}

		// -------------------------------------------------------------
		// Geometry IO
		// -------------------------------------------------------------

		static UniqueFuture<Geometry> ReadAssimpRawAsync(const LoadParams<Geometry>& params);
		inline static Geometry ReadAssimpRaw(const LoadParams<Geometry>& params) {
			return std::move(ReadAssimpRawAsync(params).Evaluate());
		}

		static UniqueFuture<Geometry> ReadAsync(const LoadParams<Geometry>& params);
		inline static Geometry Read(const LoadParams<Geometry>& params) {
			return std::move(ReadAsync(params).Evaluate());
		}

		inline static Geometry Read(const std::string& source) {
			LoadParams<Geometry> params;
			params.mSource = source;
			params.mVertexLayout = VertexLayout::PositionUVNormalTangentBitangent();
			return Read(params);
		}
		
		void ToDiligent(DG::IRenderDevice* device, 
			DG::IBuffer** vertexBufferOut, 
			DG::IBuffer** indexBufferOut) const;

		static UniqueFuture<Geometry> Load(
			Device device, const LoadParams<Geometry>& params);
		static UniqueFuture<Geometry> Load(
			const LoadParams<Geometry>& params);
		static Future<Handle<Geometry>> LoadHandle(
			Device device, const LoadParams<Geometry>& params);
		static Future<Handle<Geometry>> LoadHandle(
			const LoadParams<Geometry>& params);

		void FromMemory(const VertexLayout& layout,
			size_t vertex_count,
			size_t index_count,
			const uint32_t indices[],
			const float positions[],
			const float uvs[],
			const float normals[],
			const float tangents[],
			const float bitangents[]);

		inline void FromMemory(const VertexLayout& layout,
			size_t vertex_count,
			const float positions[],
			const float uvs[],
			const float normals[],
			const float tangents[],
			const float bitangents[]) {
			FromMemory(layout, vertex_count, 0, nullptr, 
				positions, uvs, normals, tangents, bitangents);
		}

		// -------------------------------------------------------------
		// Geometry Constructors and Destructors
		// -------------------------------------------------------------

		inline Geometry() {
		}

		inline Geometry(
			Device device, 
			const Geometry* geometry) {
			CreateDeviceAspect(device, geometry);
		}

		inline Geometry(
			Device device, 
			const Geometry& geometry) :
			Geometry(device, &geometry) {
		}

		inline Geometry(
			DG::IRenderDevice* device,
			DG::IBuffer* vertexBuffer, 
			DG::IBuffer* indexBuffer,
			uint vertexBufferOffset, 
			const DG::DrawIndexedAttribs& attribs,
			const VertexLayout& layout,
			const BoundingBox& aabb) {
			Set(device, vertexBuffer, indexBuffer, vertexBufferOffset,
				attribs, layout, aabb);
		}

		inline Geometry(
			DG::IRenderDevice* device,
			DG::IBuffer* vertexBuffer,
			uint vertexBufferOffset,
			const DG::DrawAttribs& attribs,
			const VertexLayout& layout,
			const BoundingBox& aabb)  {
			Set(device, vertexBuffer, vertexBufferOffset, attribs,
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
			Read(source);
		}

		inline Geometry(const std::string& source, const VertexLayout& layout) {
			LoadParams<Geometry> params(source, layout);
			Read(params);
		}

		inline Geometry(const LoadParams<Geometry>& params) {
			Read(params);
		}

		// -------------------------------------------------------------
		// Geometry Properties
		// -------------------------------------------------------------

		inline int GetChannelCount() const {
			return mCpuAspect.mVertexBufferDatas.size();
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
			assert(mDevice.IsCPU());
			return mCpuAspect.mVertexBufferDatas[channel];
		}

		inline const std::vector<uint8_t>& GetIndexData() const {
			assert(mDevice.IsCPU());
			return mCpuAspect.mIndexBufferData;
		}

		inline const DG::BufferDesc& GetVertexDesc(int channel = 0) const {
			assert(mDevice.IsCPU());
			return mCpuAspect.mVertexBufferDescs[channel];
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

			static Geometry MaterialBall(Device device, const VertexLayout& layout);
			static Geometry Box(Device device, const VertexLayout& layout);
			static Geometry Sphere(Device device, const VertexLayout& layout);
			static Geometry BlenderMonkey(Device device, const VertexLayout& layout);
			static Geometry Torus(Device device, const VertexLayout& layout);
			static Geometry Plane(Device device, const VertexLayout& layout);
			static Geometry StanfordBunny(Device device, const VertexLayout& layout);
			static Geometry UtahTeapot(Device device, const VertexLayout& layout);
		};

		static void RegisterMetaData();

		Geometry To(Device device, Context context = Context());
		BarrierOut MoveAsync(Device device, Context context = Context()) override;
		UniqueFuture<Geometry> ToAsync(Device device, Context context = Context());
		UniqueFuture<Geometry> GPUToCPUAsync(Device device, Context context) const;

		entt::meta_type GetType() const override;
		entt::meta_any GetSourceMeta() const override;

		inline const LoadParams<Geometry>& GetSource() const {
			return mSource;
		}

		void BinarySerialize(std::ostream& output) const override;
		void BinaryDeserialize(std::istream& input)  override;
		void BinarySerializeSource(
			const std::filesystem::path& workingPath, 
			cereal::PortableBinaryOutputArchive& output) const override;
		void BinaryDeserializeSource(
			const std::filesystem::path& workingPath,
			cereal::PortableBinaryInputArchive& input) override;

		friend class RawGeometry;
	};

	void ComputeLayoutProperties(
		size_t vertex_count,
		const VertexLayout& layout,
		std::vector<size_t>& offsets,
		std::vector<size_t>& strides,
		std::vector<size_t>& channel_sizes);
}