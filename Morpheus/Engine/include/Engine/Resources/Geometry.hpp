#pragma once

#include <Engine/Resources/Resource.hpp>
#include <Engine/Graphics.hpp>
#include <Engine/GeometryStructures.hpp>

namespace Morpheus {
	template <>
	struct LoadParams<Geometry> {
		// Geometry resource will be loaded with this layout
		VertexLayout mVertexLayout;
		// Geometry resource will be loaded from this file
		std::filesystem::path mPath;
		// Only need to set this if we are loading from a geometry cache
		GeometryType mType = GeometryType::UNSPECIFIED;

		ArchiveLoadType mArchiveLoad;

		inline LoadParams() {
		}

		inline LoadParams(
			const std::filesystem::path& path, 
			const VertexLayout& layout) : 
			mPath(path), mVertexLayout(layout) {
		}

		inline LoadParams(
			const std::filesystem::path& path, 
			GeometryType type) :
			mPath(path), mType(type) {
		}

		inline bool operator==(const LoadParams<Geometry>& t) const {
			return mPath == t.mPath;
		}

		inline const std::filesystem::path& GetPath() const {
			return mPath;
		}

		template <class Archive>
		void save(Archive& archive) const {
			std::save<Archive>(archive, mPath);
			archive(mType);

			if (mType == GeometryType::UNSPECIFIED)
				archive(mVertexLayout);
		}

		template <class Archive>
		void load(Archive& archive) {
			std::load<Archive>(archive, mPath);
			archive(mType);

			if (mType == GeometryType::UNSPECIFIED)
				archive(mVertexLayout);
		}
	};

	template <typename IndexType = uint32_t,
		typename Vec2Type = DG::float2,
		typename Vec3Type = DG::float3,
		typename Vec4Type = DG::float4>
	struct GeometryData {
		std::vector<IndexType> mIndices;
		std::vector<Vec3Type> mPositions;
		std::vector<std::vector<Vec2Type>> mUVs;
		std::vector<Vec3Type> mNormals;
		std::vector<Vec3Type> mTangents;
		std::vector<Vec3Type> mBitangents;
		std::vector<std::vector<Vec4Type>> mColors;

		template <typename Archive>
		void serialize(Archive& arr) {
			arr(mIndices);
			arr(mPositions);
			arr(mUVs);
			arr(mNormals);
			arr(mTangents);
			arr(mBitangents);
			arr(mColors);
		}
	};

	template <typename IndexType = uint32_t,
		typename Vec2Type = DG::float2,
		typename Vec3Type = DG::float3,
		typename Vec4Type = DG::float4>
	struct GeometryDataSource {
		size_t mVertexCount;
		size_t mIndexCount;

		const IndexType* mIndices;
		const Vec3Type* mPositions;
		std::vector<const Vec2Type*> mUVs;
		const Vec3Type* mNormals;
		const Vec3Type* mTangents;
		const Vec3Type* mBitangents;
		std::vector<const Vec4Type*> mColors;

		GeometryDataSource(
			const GeometryData<IndexType, Vec2Type, Vec3Type, Vec4Type>& data) :
			mVertexCount(data.mPositions.size()),
			mIndexCount(data.mIndices.size()),
			mPositions(data.mPositions.size() > 0 ? &data.mPositions[0] : nullptr),
			mNormals(data.mNormals.size() > 0 ? &data.mNormals[0] : nullptr),
			mTangents(data.mTangents.size() > 0 ? &data.mTangents[0] : nullptr),
			mBitangents(data.mBitangents.size() > 0 ? &data.mBitangents[0] : nullptr) {

			for (auto& uv : data.mUVs) 
				mUVs.emplace_back(&uv[0]);
			
			for (auto& colors : data.mColors) 
				mColors.emplace_back(&colors[0]);
		}

		GeometryDataSource(
			size_t vertexCount,
			size_t indexCount,
			const IndexType indices[],
			const Vec3Type positions[],
			const std::vector<const Vec2Type*>& uvs = {},
			const Vec3Type normals[] = nullptr,
			const Vec3Type tangents[] = nullptr,
			const Vec3Type bitangents[] = nullptr,
			const std::vector<const Vec4Type*>& colors = {}) : 
			mVertexCount(vertexCount),
			mIndexCount(indexCount),
			mIndices(indices),
			mPositions(positions),
			mUVs(uvs),
			mNormals(normals),
			mTangents(tangents),
			mBitangents(bitangents),
			mColors(colors) {
		}

		GeometryDataSource(
			size_t vertexCount,
			size_t indexCount,
			const IndexType indices[],
			const Vec3Type positions[],
			const Vec2Type uvs[] = nullptr,
			const Vec3Type normals[] = nullptr,
			const Vec3Type tangents[] = nullptr,
			const Vec3Type bitangents[] = nullptr,
			const Vec4Type colors[] = nullptr) :
			GeometryDataSource(vertexCount,
				indexCount,
				indices,
				positions,
				std::vector<const Vec2Type*>{uvs},
				normals,
				tangents,
				bitangents,
				std::vector<const Vec4Type*>{colors}) {
		}
		
		GeometryDataSource(size_t vertexCount,
			const Vec3Type positions[],
			const std::vector<const Vec2Type*>& uvs = {},
			const Vec3Type normals[] = nullptr,
			const Vec3Type tangents[] = nullptr,
			const Vec3Type bitangents[] = nullptr,
			const std::vector<const Vec4Type*>& colors = {}) :
			GeometryDataSource(vertexCount,
				0,
				nullptr,
				positions,
				uvs,
				normals,
				tangents,
				bitangents,
				colors) {
		}

		GeometryDataSource(size_t vertexCount,
			const Vec3Type positions[],
			const Vec2Type uvs[] = nullptr,
			const Vec3Type normals[] = nullptr,
			const Vec3Type tangents[] = nullptr,
			const Vec3Type bitangents[] = nullptr,
			const Vec4Type colors[] = nullptr) : 
			GeometryDataSource(vertexCount,
				0,
				nullptr,
				positions,
				{uvs},
				normals,
				tangents,
				bitangents,
				{colors}) {
		}

		bool HasIndices() const {
			return mIndices != nullptr;
		}

		bool HasPositions() const {
			return mPositions != nullptr;
		}

		bool HasNormals() const {
			return mNormals != nullptr;
		}

		bool HasTangents() const {
			return mTangents != nullptr;
		}

		bool HasBitangents() const {
			return mBitangents != nullptr;
		}
	};

	typedef GeometryDataSource<> GeometryDataSourceVectorFloat;
	typedef GeometryDataSource<uint32_t, float, float, float> GeometryDataSourceFloat;
	typedef GeometryData<uint32_t, float, float, float> GeometryDataFloat;

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

		template <typename I3T, typename V2T, typename V3T, typename V4T>
		void Pack(const VertexLayout& layout,
			const GeometryDataSource<I3T, V2T, V3T, V4T>& data);

		GeometryDataFloat Unpack() const;

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

		static UniqueFuture<Geometry> ReadAssimpAsync(
			const LoadParams<Geometry>& params);
		inline static Geometry ReadAssimpRaw(const LoadParams<Geometry>& params) {
			return std::move(ReadAssimpAsync(params).Evaluate());
		}

		static UniqueFuture<Geometry> ReadAsync(const LoadParams<Geometry>& params);
		inline static Geometry Read(const LoadParams<Geometry>& params) {
			return std::move(ReadAsync(params).Evaluate());
		}

		inline static Geometry Read(const std::filesystem::path& path) {
			LoadParams<Geometry> params;
			params.mPath = path;
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
			const GeometryDataSourceFloat& data);
		void FromMemory(const VertexLayout& layout,
			const GeometryDataSourceVectorFloat& data);

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

		inline Geometry(const LoadParams<Geometry>& params) {
			mDevice = Device::Disk();
			mSource = params;
		}

		inline Geometry(Device device, const LoadParams<Geometry>& params) : 
			Geometry(params) {
			Move(device);
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
			const DG::DrawIndexedAttribs& indexedDrawAttribs,
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
			const GeometryDataSourceFloat& data) {
			FromMemory(layout, data);	
		}

		inline Geometry(const VertexLayout& layout,
			const GeometryDataSourceVectorFloat& data) {
			FromMemory(layout, data);	
		}

		void Clear();
		void AdoptData(Geometry&& other);
		void CopyTo(Geometry* geometry) const;
		void CopyFrom(const Geometry& geometry);

		Geometry(const Geometry& other) = delete;
		Geometry(Geometry&& other) = default;
		Geometry& operator=(const Geometry& other) = delete;
		Geometry& operator=(Geometry&& other) = default;

		inline Geometry(const std::string& source) {
			Read(source);
		}

		inline Geometry(const std::string& source, const VertexLayout& layout) {
			LoadParams<Geometry> params(source, layout);
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
		UniqueFuture<Geometry> GPUToCPUAsync(Device device, Context context);

		entt::meta_type GetType() const override;
		entt::meta_any GetSourceMeta() const override;
		std::filesystem::path GetPath() const override;

		inline const LoadParams<Geometry>& GetSource() const {
			return mSource;
		}

		Handle<IResource> MoveIntoHandle() override;

		void BinarySerialize(std::ostream& output) const override;
		void BinaryDeserialize(std::istream& input)  override;
		void BinarySerializeReference(
			const std::filesystem::path& workingPath, 
			cereal::PortableBinaryOutputArchive& output) const override;
		void BinaryDeserializeReference(
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