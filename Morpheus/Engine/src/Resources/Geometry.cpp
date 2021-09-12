#include <Engine/Resources/Geometry.hpp>
#include <Engine/Resources/ResourceData.hpp>
#include <Engine/Resources/Buffer.hpp>

#include <Engine/Resources/FrameIO.hpp>

#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>

using namespace Assimp;
using namespace std;
using namespace entt;

namespace Morpheus {

	void Geometry::CreateExternalAspect(IExternalGraphicsDevice* device, 
		const Geometry* source) {
		mExtAspect = ExternalAspect<ExtObjectType::GEOMETRY>(device, 
			device->CreateGeometry(*source));
	}

	Geometry Geometry::Prefabs::MaterialBall(Device device, const VertexLayout& layout) {
		return Geometry(device, Geometry::Prefabs::MaterialBall(layout));
	}

	Geometry Geometry::Prefabs::Box(Device device, const VertexLayout& layout) {
		return Geometry(device, Geometry::Prefabs::Box(layout));
	}

	Geometry Geometry::Prefabs::Sphere(Device device, const VertexLayout& layout) {
		return Geometry(device, Geometry::Prefabs::Sphere(layout));
	}

	Geometry Geometry::Prefabs::BlenderMonkey(Device device, const VertexLayout& layout) {
		return Geometry(device, Geometry::Prefabs::BlenderMonkey(layout));
	}

	Geometry Geometry::Prefabs::Torus(Device device, const VertexLayout& layout) {
		return Geometry(device, Geometry::Prefabs::Torus(layout));
	}

	Geometry Geometry::Prefabs::Plane(Device device, const VertexLayout& layout) {
		return Geometry(device, Geometry::Prefabs::Plane(layout));
	}

	Geometry Geometry::Prefabs::StanfordBunny(Device device, const VertexLayout& layout) {
		return Geometry(device, Geometry::Prefabs::StanfordBunny(layout));
	}

	Geometry Geometry::Prefabs::UtahTeapot(Device device, const VertexLayout& layout) {
		return Geometry(device, Geometry::Prefabs::UtahTeapot(layout));
	}

	void Geometry::Set(
		DG::IRenderDevice* device,
		DG::IBuffer* vertexBuffer, 
		DG::IBuffer* indexBuffer,
		uint vertexBufferOffset, 
		const DG::DrawIndexedAttribs& attribs,
		const VertexLayout& layout,
		const BoundingBox& aabb) {
		mDevice = device;

		mRasterAspect.mVertexBuffer = vertexBuffer;
		mRasterAspect.mIndexBuffer = indexBuffer;
		mRasterAspect.mVertexBufferOffset = vertexBufferOffset;
		mShared.mLayout = layout,
		mShared.mBoundingBox = aabb;
		mShared.mIndexedAttribs = attribs;
	}

	void Geometry::Set(
		DG::IRenderDevice* device,
		DG::IBuffer* vertexBuffer,
		uint vertexBufferOffset,
		const DG::DrawAttribs& attribs,
		const VertexLayout& layout,
		const BoundingBox& aabb) {
		mDevice = device;

		mRasterAspect.mVertexBuffer = vertexBuffer;
		mRasterAspect.mIndexBuffer = nullptr;
		mRasterAspect.mVertexBufferOffset = vertexBufferOffset;
		mShared.mLayout = layout;
		mShared.mBoundingBox = aabb;
		mShared.mUnindexedAttribs = attribs;
	}

	void Geometry::CreateRasterAspect(DG::IRenderDevice* device, const Geometry* geometry) {
		
		DG::IBuffer* vertexBuffer = nullptr;
		DG::IBuffer* indexBuffer = nullptr;

		geometry->ToDiligent(device, &vertexBuffer, &indexBuffer);

		if (indexBuffer) {
			Set(device, vertexBuffer, indexBuffer, 0,
				geometry->GetIndexedDrawAttribs(), 
				mShared.mLayout, geometry->GetBoundingBox());
		} else {
			Set(device, vertexBuffer, 0, geometry->GetDrawAttribs(), 
				mShared.mLayout,  geometry->GetBoundingBox());
		}
	}

	void Geometry::CreateDeviceAspect(Device device, const Geometry* source) {
		if (!source->mDevice.IsCPU())
			throw std::runtime_error("Input geometry must be CPU!");

		if (device.IsGPU())
			CreateRasterAspect(device, source);
		else if (device.IsExternal())
			CreateExternalAspect(device, source);
		else if (device.IsCPU()) 
			CopyFrom(*source);
		else if (device.IsDisk())
			throw std::runtime_error("Cannot create a device aspect on disk!");
		else
			throw std::runtime_error("Device cannot be null!");
	}

	UniqueFuture<Geometry> Geometry::Load(
		Device device, const LoadParams<Geometry>& params) {
		return Geometry(params).ToAsync(device);
	}

	UniqueFuture<Geometry> Geometry::Load(
		const LoadParams<Geometry>& params) {
		return Geometry(params).ToAsync(Device::CPU());
	}

	Future<Handle<Geometry>> Geometry::LoadHandle(
		Device device, const LoadParams<Geometry>& params) {
		Promise<Handle<Geometry>> result;
		PipeToHandle(Load(device, params), result);
		return result;
	}

	Future<Handle<Geometry>> Geometry::LoadHandle(
		const LoadParams<Geometry>& params) {
		Promise<Handle<Geometry>> result;
		PipeToHandle(Load(params), result);
		return result;
	}

	void Geometry::CopyTo(Geometry* geometry) const {
		geometry->CopyFrom(*this);
	}

	void Geometry::CopyFrom(const Geometry& geometry) {
		if (!geometry.mDevice.IsCPU())
			throw std::runtime_error("Cannot copy non-CPU geometry with CopyFrom!");

		mCpuAspect = geometry.mCpuAspect;
		mDevice = geometry.mDevice;
		mShared = geometry.mShared;
		mSource = geometry.mSource;
	}

	void Geometry::Set(const VertexLayout& layout,
		std::vector<DG::BufferDesc>&& vertexBufferDescs, 
		std::vector<std::vector<uint8_t>>&& vertexBufferDatas,
		const DG::DrawAttribs& unindexedDrawAttribs,
		const BoundingBox& aabb) {

		mDevice = Device::CPU();
		
		mCpuAspect.mVertexBufferDescs = std::move(vertexBufferDescs);
		mCpuAspect.mVertexBufferDatas = std::move(vertexBufferDatas);
		mCpuAspect.bHasIndexBuffer = false;

		mShared.mLayout = layout;
		mShared.mUnindexedAttribs = unindexedDrawAttribs;
		mShared.mBoundingBox = aabb;
	}

	void Geometry::Set(const VertexLayout& layout,
		std::vector<DG::BufferDesc>&& vertexBufferDescs,
		const DG::BufferDesc& indexBufferDesc,
		std::vector<std::vector<uint8_t>>&& vertexBufferDatas,
		std::vector<uint8_t>&& indexBufferData,
		const DG::DrawIndexedAttribs& indexedDrawAttribs,
		const BoundingBox& aabb) {

		mDevice = Device::CPU();
		
		mCpuAspect.mVertexBufferDescs = std::move(vertexBufferDescs);
		mCpuAspect.mIndexBufferDesc = indexBufferDesc;
		mCpuAspect.mVertexBufferDatas = std::move(vertexBufferDatas);
		mCpuAspect.mIndexBufferData = indexBufferData;
		mCpuAspect.bHasIndexBuffer = true;

		mShared.mIndexedAttribs = indexedDrawAttribs;
		mShared.mLayout = layout;
		mShared.mBoundingBox = aabb;
	}

	void Geometry::AdoptData(Geometry&& other) {
		mCpuAspect = std::move(other.mCpuAspect);
		mShared = std::move(other.mShared);
		mRasterAspect = std::move(other.mRasterAspect);
		mExtAspect = std::move(other.mExtAspect);
		mDevice = std::move(other.mDevice);
		mSource = std::move(other.mSource);
	}

	void Geometry::ToDiligent(DG::IRenderDevice* device, 
		DG::IBuffer** vertexBufferOut, 
		DG::IBuffer** indexBufferOut) const {

		if (!mDevice.IsCPU())
			throw std::runtime_error("Spawn on GPU requires geometry have a raw aspect!");

		if (mCpuAspect.mVertexBufferDatas.size() == 0)
			throw std::runtime_error("Spawning on GPU requires at least one channel!");

		DG::BufferData data;
		data.pData = &mCpuAspect.mVertexBufferDatas[0][0];
		data.DataSize = mCpuAspect.mVertexBufferDatas[0].size();
		device->CreateBuffer(mCpuAspect.mVertexBufferDescs[0], &data, vertexBufferOut);

		if (mCpuAspect.bHasIndexBuffer) {
			data.pData = &mCpuAspect.mIndexBufferData[0];
			data.DataSize = mCpuAspect.mIndexBufferData.size();
			device->CreateBuffer(mCpuAspect.mIndexBufferDesc, &data, indexBufferOut);
		}
	}

	UniqueFuture<Geometry> Geometry::ReadAssimpAsync(const LoadParams<Geometry>& params) {

		FunctionPrototype<Promise<Geometry>> prototype([params](const TaskParams& e, Promise<Geometry> result) {
			std::vector<uint8_t> data;
			ReadBinaryFile(params.mPath, data);

			Assimp::Importer importer;

			unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | 
				aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices |
				aiProcess_GenUVCoords | aiProcess_CalcTangentSpace | 
				aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_Quality;

			const aiScene* pScene = importer.ReadFileFromMemory(&data[0], data.size(), 
				flags,
				params.mPath.c_str());
			
			if (!pScene) {
				std::cout << importer.GetErrorString() << std::endl;
				throw std::runtime_error("Failed to load geometry!");
			}

			if (!pScene->HasMeshes()) {
				throw std::runtime_error("Geometry has no meshes!");
			}

			const VertexLayout* layout = &params.mVertexLayout;
	
			result = ReadAssimpRaw(pScene, *layout);
		});
		
		Promise<Geometry> result;
		prototype(result).SetName("Load Raw Geometry (Assimp)");
		return result;
	}

	void ComputeLayoutProperties(
		size_t vertex_count,
		const VertexLayout& layout,
		std::vector<size_t>& offsets,
		std::vector<size_t>& strides,
		std::vector<size_t>& channel_sizes) {
		offsets.clear();
		strides.clear();

		size_t nVerts = vertex_count;

		auto& layoutElements = layout.mElements;

		int channelCount = 0;
		for (auto& layoutItem : layoutElements) {
			channelCount = std::max<int>(channelCount, layoutItem.BufferSlot + 1);
		}

		channel_sizes.resize(channelCount);
		std::vector<size_t> channel_auto_strides(channelCount);

		std::fill(channel_sizes.begin(), channel_sizes.end(), 0u);
		std::fill(channel_auto_strides.begin(), channel_auto_strides.end(), 0u);

		// Compute offsets
		for (auto& layoutItem : layoutElements) {
			uint size = GetSize(layoutItem.ValueType) * layoutItem.NumComponents;

			if (layoutItem.Frequency == DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX) {
				uint channel = layoutItem.BufferSlot;

				size_t offset = 0;

				if (layoutItem.RelativeOffset == DG::LAYOUT_ELEMENT_AUTO_OFFSET) {
					offset = channel_auto_strides[channel];
				} else {
					offset = layoutItem.RelativeOffset;
				}

				offsets.emplace_back(offset);
				channel_auto_strides[channel] += size;
			} else {
				offsets.emplace_back(0);
			}
		}

		// Compute strides
		for (int i = 0; i < offsets.size(); ++i) {
			auto& layoutItem = layoutElements[i];
			size_t offset = offsets[i];
			uint size = GetSize(layoutItem.ValueType) * layoutItem.NumComponents;

			if (layoutItem.Frequency == DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX) {
				uint channel = layoutItem.BufferSlot;

				size_t stride = 0;

				if (layoutItem.Stride == DG::LAYOUT_ELEMENT_AUTO_STRIDE) {
					stride = channel_auto_strides[channel];
				} else {
					stride = layoutItem.Stride;
				}

				strides.emplace_back(stride);

				size_t lastIndex = offset + size + (nVerts - 1) * stride;

				channel_sizes[channel] = std::max<size_t>(channel_sizes[channel], lastIndex);

			} else {
				strides.emplace_back(0);
			}
		}
	}

	template <typename T>
	struct V4Packer;

	template <typename T>
	struct V3Packer;

	template <typename T>
	struct V2Packer;

	template <typename T>
	struct I3Packer;

	template <>
	struct V4Packer<DG::float4> {
		static constexpr size_t Stride = 1;

		inline static void Pack(float* dest, const DG::float4* src) {
			dest[0] = src->x;
			dest[1] = src->y;
			dest[2] = src->z;
			dest[3] = src->w;
		}
	};

	template <>
	struct V4Packer<float> {
		static constexpr size_t Stride = 4;

		inline static void Pack(float* dest, const float* src) {
			dest[0] = src[0];
			dest[1] = src[1];
			dest[2] = src[2];
			dest[3] = src[3];
		}
	};

	template <>
	struct V3Packer<DG::float3> {
		static constexpr size_t Stride = 1;

		inline static void Pack(float* dest, const DG::float3* src) {
			dest[0] = src->x;
			dest[1] = src->y;
			dest[2] = src->z;
		}
	};

	template <>
	struct V3Packer<float> {
		static constexpr size_t Stride = 3;

		inline static void Pack(float* dest, const float* src) {
			dest[0] = src[0];
			dest[1] = src[1];
			dest[2] = src[2];
		}
	};

	template <>
	struct V2Packer<float> {
		static constexpr size_t Stride = 2;

		inline static void Pack(float* dest, const float* src) {
			dest[0] = src[0];
			dest[1] = src[1];
		}
	};

	template <>
	struct V2Packer<DG::float2> {
		static constexpr size_t Stride = 1;

		inline static void Pack(float* dest, const DG::float2* src) {
			dest[0] = src->x;
			dest[1] = src->y;
		}
	};

	template <>
	struct V3Packer<aiVector3D> {
		static constexpr size_t Stride = 1;

		inline static void Pack(float* dest, const aiVector3D* src) {
			dest[0] = src->x;
			dest[1] = src->y;
			dest[2] = src->z;
		}
	};

	template <>
	struct V2Packer<aiVector3D> {
		static constexpr size_t Stride = 1;

		inline static void Pack(float* dest, const aiVector3D* src) {
			dest[0] = src->x;
			dest[1] = src->y;
		}
	};

	template <>
	struct I3Packer<aiFace> {
		static constexpr size_t Stride = 1;

		inline static void Pack(uint32_t* dest, const aiFace* src) {
			dest[0] = src->mIndices[0];
			dest[1] = src->mIndices[1];
			dest[2] = src->mIndices[2];
		}
	};

	template <>
	struct I3Packer<uint32_t> {
		static constexpr size_t Stride = 3;

		inline static void Pack(uint32_t* dest, const uint32_t* src) {
			dest[0] = src[0];
			dest[1] = src[1];
			dest[2] = src[2];
		}
	};

	template <
		typename destT, 
		typename srcT, 
		void(*MoveFunc)(destT*, const srcT*)>
	void ArraySliceCopy(destT* dest, const srcT* source, 
		size_t destStride, size_t srcStride, size_t count) {
		size_t destIndex = 0;
		size_t srcIndex = 0;
		
		for (size_t i = 0; i < count; ++i, destIndex += destStride, srcIndex += srcStride) {
			(*MoveFunc)(&dest[destIndex], &source[srcIndex]);
		}
	}

	template <typename T, size_t dim>
	void ArraySliceCopy(T* dest, const T* source,
		size_t destStride, size_t srcStride, size_t count) {
		size_t destIndex = 0;
		size_t srcIndex = 0;
		for (size_t i = 0; i < count; ++i, destIndex += destStride, srcIndex += srcStride) {
			for (size_t component = 0; component < dim; ++component) {
				dest[destIndex + component] = source[srcIndex + component];
			}
		}
	}

	template <typename T, size_t dim>
	void ArraySliceBoundingBox(T* arr, 
		size_t stride, size_t count, 
		std::array<T, dim>& lower, std::array<T, dim>& upper) {
		std::fill(upper.begin(), upper.end(), -std::numeric_limits<T>::infinity());
		std::fill(lower.begin(), lower.end(), std::numeric_limits<T>::infinity());

		size_t srcIndex = 0;
		for (size_t i = 0; i < count; ++i, srcIndex += stride) {
			for (size_t component = 0; component < dim; ++component) {
				upper[component] = std::max<T>(upper[component], arr[srcIndex]);
				lower[component] = std::min<T>(lower[component], arr[srcIndex]);
			}
		}
	}

	BoundingBox ArraySliceBoundingBox(float* arr,
		size_t stride, size_t count) {
		std::array<float, 3> upper;
		std::array<float, 3> lower;

		ArraySliceBoundingBox<float, 3>(arr, stride, count, lower, upper);

		BoundingBox result;
		result.mLower = DG::float3(lower[0], lower[1], lower[2]);
		result.mUpper = DG::float3(upper[0], upper[1], upper[2]);
		return result;
	}

	template <typename destT, size_t componentCount>
	void ArraySliceFill(destT* dest,
		const destT& value,
		size_t stride,
		size_t vertex_count) {

		for (size_t i = 0; i < vertex_count; i += stride) {
			for (size_t component = 0; component < componentCount; ++component) {
				dest[i + component] = value;
			}
		}
	}

	struct PackIndexing {
		int mPositionOffset = -1;
		int mPositionChannel = -1;
		int mPositionStride = -1;

		std::vector<int> mUVOffsets = {};
		std::vector<int> mUVChannels = {};
		std::vector<int> mUVStrides = {};

		int mNormalOffset = -1;
		int mNormalChannel = -1;
		int mNormalStride = -1;

		int mTangentOffset = -1;
		int mTangentChannel = -1;
		int mTangentStride = -1;

		int mBitangentOffset = -1;
		int mBitangentChannel = -1;
		int mBitangentStride = -1;

		std::vector<int> mColorOffsets = {};
		std::vector<int> mColorChannels = {};
		std::vector<int> mColorStrides = {};

		std::vector<size_t> mChannelSizes;

		static PackIndexing From(const VertexLayout& layout,
			size_t vertex_count) {

			std::vector<size_t> offsets;
			std::vector<size_t> strides;

			PackIndexing indexing;

			auto& layoutElements = layout.mElements;	
			ComputeLayoutProperties(vertex_count, layout, offsets, strides, indexing.mChannelSizes);

			auto verifyAttrib = [](const DG::LayoutElement& element) {
				if (element.ValueType != DG::VT_FLOAT32) {
					throw std::runtime_error("Attribute type must be VT_FLOAT32!");
				}
			};

			if (layout.mPosition >= 0) {
				auto& posAttrib = layoutElements[layout.mPosition];
				verifyAttrib(posAttrib);
				indexing.mPositionOffset = offsets[layout.mPosition];
				indexing.mPositionChannel = posAttrib.BufferSlot;
				indexing.mPositionStride = strides[layout.mPosition];
			}

			for (auto& uv : layout.mUVs) {
				auto& uvAttrib = layoutElements[uv];
				verifyAttrib(uvAttrib);
				indexing.mUVOffsets.emplace_back(offsets[uv]);
				indexing.mUVChannels.emplace_back(uvAttrib.BufferSlot);
				indexing.mUVStrides.emplace_back(strides[uv]);
			}

			if (layout.mNormal >= 0) {
				auto& normalAttrib = layoutElements[layout.mNormal];
				verifyAttrib(normalAttrib);
				indexing.mNormalOffset = offsets[layout.mNormal];
				indexing.mNormalChannel = normalAttrib.BufferSlot;
				indexing.mNormalStride = strides[layout.mNormal];
			}

			if (layout.mTangent >= 0) {
				auto& tangentAttrib = layoutElements[layout.mTangent];
				verifyAttrib(tangentAttrib);
				indexing.mTangentOffset = offsets[layout.mTangent];
				indexing.mTangentChannel = tangentAttrib.BufferSlot;
				indexing.mTangentStride = strides[layout.mTangent];
			}

			if (layout.mBitangent >= 0) {
				auto& bitangentAttrib = layoutElements[layout.mBitangent];
				verifyAttrib(bitangentAttrib);
				indexing.mBitangentOffset = offsets[layout.mBitangent];
				indexing.mBitangentChannel = bitangentAttrib.BufferSlot;
				indexing.mBitangentStride = strides[layout.mBitangent];
			}

			for (auto& color : layout.mColors) {
				auto& colorAttrib = layoutElements[color];
				verifyAttrib(colorAttrib);
				indexing.mColorOffsets.emplace_back(offsets[color]);
				indexing.mColorChannels.emplace_back(colorAttrib.BufferSlot);
				indexing.mColorStrides.emplace_back(strides[color]);
			}

			return indexing;
		}
	};
	
	template <typename I3T, typename V2T, typename V3T, typename V4T>
	void Geometry::Pack(const VertexLayout& layout,
		const GeometryDataSource<I3T, V2T, V3T, V4T>& data) {

		mDevice = Device::CPU();

		size_t vertex_count = data.mVertexCount;
		size_t index_count = data.mIndexCount;

		auto indexing = PackIndexing::From(layout, vertex_count);

		auto& channel_sizes = indexing.mChannelSizes;
		uint channelCount = channel_sizes.size();
	
		std::vector<std::vector<uint8_t>> vert_buffers(channelCount);
		for (int i = 0; i < channelCount; ++i)
			vert_buffers[i] = std::vector<uint8_t>(channel_sizes[i]);

		std::vector<uint8_t> indx_buffer_raw(index_count * sizeof(DG::Uint32));
		DG::Uint32* indx_buffer = (DG::Uint32*)(&indx_buffer_raw[0]);

		BoundingBox aabb;

		bool bHasNormals = data.mNormals != nullptr;
		bool bHasPositions = data.mPositions != nullptr;
		bool bHasBitangents = data.mBitangents != nullptr;
		bool bHasTangents = data.mTangents != nullptr;

		if (indexing.mPositionOffset >= 0) {
			auto& channel = vert_buffers[indexing.mPositionChannel];
			auto arr = reinterpret_cast<float*>(&channel[indexing.mPositionOffset]);
			if (bHasPositions) {
				ArraySliceCopy<float, V3T, &V3Packer<V3T>::Pack>(
					arr, &data.mPositions[0], indexing.mPositionStride, V3Packer<V3T>::Stride, vertex_count);
				aabb = ArraySliceBoundingBox(arr, indexing.mPositionStride,  vertex_count);
			} else {
				std::cout << "Warning: Pipeline expects positions, but model has none!" << std::endl;
				ArraySliceFill<float, 3>(arr, 0.0f, indexing.mPositionStride, vertex_count);
				aabb.mLower = DG::float3(0.0f, 0.0f, 0.0f);
				aabb.mUpper = DG::float3(0.0f, 0.0f, 0.0f);
			}
		}

		for (int iuv = 0; iuv < indexing.mUVChannels.size(); ++iuv) {
			auto& vertexChannel = vert_buffers[indexing.mUVChannels[iuv]];
			auto arr = reinterpret_cast<float*>(&vertexChannel[indexing.mUVOffsets[iuv]]);
			if (iuv <= data.mUVs.size()) {
				ArraySliceCopy<float, V2T, &V2Packer<V2T>::Pack>(
					arr, &data.mUVs[iuv][0], indexing.mUVStrides[iuv], V2Packer<V2T>::Stride, vertex_count);
			} else {
				std::cout << "Warning: Pipeline expects UVs, but model has none!" << std::endl;
				ArraySliceFill<float, 2>(arr, 0.0f, indexing.mUVStrides[iuv], vertex_count);
			}
		}

		if (indexing.mNormalOffset >= 0) {
			auto& channel = vert_buffers[indexing.mNormalChannel];
			auto arr = reinterpret_cast<float*>(&channel[indexing.mNormalOffset]);
			if (bHasNormals) {
				ArraySliceCopy<float, V3T, &V3Packer<V3T>::Pack>(
					arr, &data.mNormals[0], indexing.mNormalStride, V3Packer<V3T>::Stride, vertex_count);
			} else {
				std::cout << "Warning: Pipeline expects normals, but model has none!" << std::endl;
				ArraySliceFill<float, 3>(arr, 0.0f, indexing.mNormalStride, vertex_count);
			}
		}

		if (indexing.mTangentOffset >= 0) {
			auto& channel = vert_buffers[indexing.mTangentChannel];
			auto arr = reinterpret_cast<float*>(&channel[indexing.mTangentOffset]);
			if (bHasTangents) {
				ArraySliceCopy<float, V3T, &V3Packer<V3T>::Pack>(
					arr, &data.mTangents[0], indexing.mTangentStride, 
						V3Packer<V3T>::Stride, vertex_count);
			} else {
				std::cout << "Warning: Pipeline expects tangents, but model has none!" << std::endl;
				ArraySliceFill<float, 3>(arr, 0.0f, indexing.mTangentStride, vertex_count);
			}
		}

		if (indexing.mBitangentOffset >= 0) {
			auto& channel = vert_buffers[indexing.mBitangentChannel];
			auto arr = reinterpret_cast<float*>(&channel[indexing.mBitangentOffset]);
			if (bHasBitangents) {
				ArraySliceCopy<float, V3T, &V3Packer<V3T>::Pack>(
					arr, &data.mBitangents[0], indexing.mBitangentStride, 
						V3Packer<V3T>::Stride, vertex_count);
			} else {
				std::cout << "Warning: Pipeline expects bitangents, but model has none!" << std::endl;
				ArraySliceFill<float, 3>(arr, 0.0f, indexing.mBitangentStride, vertex_count);
			}
		}

		for (int icolor = 0; icolor < indexing.mColorChannels.size(); ++icolor) {
			auto& vertexChannel = vert_buffers[indexing.mColorChannels[icolor]];
			auto arr = reinterpret_cast<float*>(&vertexChannel[indexing.mColorOffsets[icolor]]);
			if (icolor <= data.mColors.size()) {
				ArraySliceCopy<float, V4T, &V4Packer<V4T>::Pack>(
					arr, &data.mColors[icolor][0], indexing.mColorStrides[icolor], 
						V4Packer<V4T>::Stride, vertex_count);
			} else {
				std::cout << "Warning: Pipeline expects colors, but model has none!" << std::endl;
				ArraySliceFill<float, 4>(arr, 1.0f, indexing.mColorStrides[icolor], vertex_count);
			}
		}

		if (data.mIndices != nullptr) {
			ArraySliceCopy<uint32_t, I3T, &I3Packer<I3T>::Pack>(
				&indx_buffer[0], &data.mIndices[0], 3, I3Packer<I3T>::Stride, index_count);
		}

		std::vector<DG::BufferDesc> bufferDescs;

		for (int i = 0; i < channelCount; ++i) {
			DG::BufferDesc vertexBufferDesc;
			vertexBufferDesc.Usage         = DG::USAGE_IMMUTABLE;
			vertexBufferDesc.BindFlags     = DG::BIND_VERTEX_BUFFER;
			vertexBufferDesc.uiSizeInBytes = vert_buffers[i].size();
			bufferDescs.emplace_back(vertexBufferDesc);
		}

		DG::BufferDesc indexBufferDesc;
		indexBufferDesc.Usage 			= DG::USAGE_IMMUTABLE;
		indexBufferDesc.BindFlags 		= DG::BIND_INDEX_BUFFER;
		indexBufferDesc.uiSizeInBytes 	= indx_buffer_raw.size();

		DG::DrawIndexedAttribs indexedAttribs;
		indexedAttribs.IndexType 	= DG::VT_UINT32;
		indexedAttribs.NumIndices 	= indx_buffer_raw.size() / sizeof(DG::Uint32);
		
		// Write to output raw geometry
		Set(layout, 
			std::move(bufferDescs), 
			indexBufferDesc, 
			std::move(vert_buffers), 
			std::move(indx_buffer_raw),
			indexedAttribs, aabb);
	}

	GeometryDataFloat Geometry::Unpack() const {
		GeometryDataFloat result;

		if (!mDevice.IsCPU()) {
			throw std::runtime_error("Resource must be on the CPU!");
		}

		size_t vertex_count = 0;
		if (mCpuAspect.bHasIndexBuffer) {
			if (mShared.mIndexedAttribs.IndexType != DG::VT_UINT32) {
				throw std::runtime_error("Index type must be VT_UINT32!");
			}

			result.mIndices.resize(mShared.mIndexedAttribs.NumIndices);
			std::memcpy(&result.mIndices[0], &mCpuAspect.mIndexBufferData[0],
				sizeof(uint32_t) * mShared.mIndexedAttribs.NumIndices);
			vertex_count = mShared.mIndexedAttribs.NumIndices;
		} else {
			vertex_count = mShared.mUnindexedAttribs.NumVertices;
		}

		auto& layout = mShared.mLayout;
		auto indexing = PackIndexing::From(layout, vertex_count);

		if (layout.mPosition >= 0) {
			result.mPositions.resize(3 * vertex_count);
			auto arr = reinterpret_cast<const float*>(
				&mCpuAspect.mVertexBufferDatas[indexing.mPositionChannel][indexing.mPositionOffset]);
			ArraySliceCopy<float, 3>(&result.mPositions[0], arr, 3, indexing.mPositionStride, vertex_count);
		}

		result.mUVs.resize(indexing.mUVChannels.size());
		for (int iuv = 0; iuv < indexing.mUVChannels.size(); ++iuv) {
			result.mUVs[iuv].resize(2 * vertex_count);
			auto arr = reinterpret_cast<const float*>(
				&mCpuAspect.mVertexBufferDatas[indexing.mUVChannels[iuv]][indexing.mUVOffsets[iuv]]);
			ArraySliceCopy<float, 2>(&result.mUVs[iuv][0], arr, 2, indexing.mUVStrides[iuv], vertex_count);
		}

		if (layout.mNormal >= 0) {
			result.mNormals.resize(3 * vertex_count);
			auto arr = reinterpret_cast<const float*>(
				&mCpuAspect.mVertexBufferDatas[indexing.mNormalChannel][indexing.mNormalOffset]);
			ArraySliceCopy<float, 3>(&result.mNormals[0], arr, 3, indexing.mNormalStride, vertex_count);
		}

		if (layout.mTangent >= 0) {
			result.mTangents.resize(3 * vertex_count);
			auto arr = reinterpret_cast<const float*>(
				&mCpuAspect.mVertexBufferDatas[indexing.mTangentChannel][indexing.mTangentOffset]);
			ArraySliceCopy<float, 3>(&result.mTangents[0], arr, 3, indexing.mTangentStride, vertex_count);
		}

		if (layout.mBitangent >= 0) {
			result.mBitangents.resize(3 * vertex_count);
			auto arr = reinterpret_cast<const float*>(
				&mCpuAspect.mVertexBufferDatas[indexing.mBitangentChannel][indexing.mBitangentOffset]);
			ArraySliceCopy<float, 3>(&result.mBitangents[0], arr, 3, indexing.mBitangentStride, vertex_count);
		}

		result.mColors.resize(indexing.mUVChannels.size());
		for (int icolor = 0; icolor < indexing.mUVChannels.size(); ++icolor) {
			result.mColors[icolor].resize(4 * vertex_count);
			auto arr = reinterpret_cast<const float*>(
				&mCpuAspect.mVertexBufferDatas[indexing.mColorChannels[icolor]][indexing.mColorOffsets[icolor]]);
			ArraySliceCopy<float, 4>(&result.mColors[icolor][0], arr, 4, indexing.mColorStrides[icolor], vertex_count);
		}

		return result;
	}

	void Geometry::FromMemory(const VertexLayout& layout,
		const GeometryDataSourceFloat& data) {
		Pack(layout, data);
	}

	void Geometry::FromMemory(const VertexLayout& layout,
		const GeometryDataSourceVectorFloat& data) {
		Pack(layout, data);
	}

	Geometry Geometry::ReadAssimpRaw(const aiScene* scene, const VertexLayout& layout) {

		size_t nVerts;
		size_t nIndices;

		if (!scene->HasMeshes()) {
			throw std::runtime_error("Assimp scene has no meshes!");
		}

		aiMesh* mesh = scene->mMeshes[0];

		nVerts = mesh->mNumVertices;
		nIndices = mesh->mNumFaces * 3;

		Geometry result;

		GeometryDataSource<aiFace, aiVector3D, aiVector3D> data(
			nVerts, nIndices,
			mesh->mFaces,
			mesh->mVertices,
			mesh->mTextureCoords[0],
			mesh->mNormals,
			mesh->mTangents,
			mesh->mBitangents);

		result.Pack<aiFace, aiVector3D, aiVector3D>(layout, data);

		return result;
	}

	UniqueFuture<Geometry> Geometry::ReadAsync(const LoadParams<Geometry>& params) {
		return ReadAssimpAsync(params);
	}

	void Geometry::Clear() {
		mRasterAspect = RasterizerAspect();
		mCpuAspect = CpuAspect();
		mShared = SharedAspect();
		mExtAspect = ExternalAspect<ExtObjectType::GEOMETRY>();

		mDevice = Device::None();
	}

	void Geometry::RegisterMetaData() {
		entt::meta<Geometry>()
			.type("Geometry"_hs)
			.base<IResource>();
		
		MakeSerializableResourceType<Geometry>();
	}

	Geometry Geometry::To(Device device, Context context) {
		auto future = ToAsync(device, context);
		context.Flush();
		return std::move(future.Evaluate());
	}

	BarrierOut Geometry::MoveAsync(Device device, Context context) {
		Barrier result;

		FunctionPrototype<UniqueFuture<Geometry>, Barrier> swapPrototype([this](
			const TaskParams& e, 
			UniqueFuture<Geometry> inGeometry, 
			Barrier) {
			*this = std::move(inGeometry.Get());
		});

		UniqueFuture<Geometry> movedGeo = ToAsync(device, context);
		swapPrototype(movedGeo, result);
		return result;
	}

	UniqueFuture<Geometry> Geometry::ToAsync(Device device, Context context) {
		Device currentDevice = mDevice;

		if (device.IsDisk())
			throw std::runtime_error("Cannot move to disk! Use a save method instead!");

		Promise<Handle<Geometry>> cpuGeometryHandle;
		Promise<Geometry> result;

		if (currentDevice.IsDisk()) {
			auto readResult = ReadAsync(mSource);
			PipeToHandle(readResult, cpuGeometryHandle);
		} else if (currentDevice.IsCPU()) {
			cpuGeometryHandle.Set(Handle<Geometry>(this));
		} else {
			auto readResult = GPUToCPUAsync(device, context);
			PipeToHandle(readResult, cpuGeometryHandle);
		}

		FunctionPrototype<Future<Handle<Geometry>>, Promise<Geometry>> moveToDevice([device](
			const TaskParams& e,
			Future<Handle<Geometry>> in,
			Promise<Geometry> out) {
			out = Geometry::CopyToDevice(device, *in.Get());
		});

		if (device.IsCPU()) {
			PipeFromHandle(cpuGeometryHandle.GetUniqueFuture(), result);
			return result;
		} else {
			moveToDevice(cpuGeometryHandle, result).OnlyThread(THREAD_MAIN);
			return result;
		}
	}

	UniqueFuture<Geometry> Geometry::GPUToCPUAsync(Device device, Context context) {
		if (!mDevice.IsGPU()) {
			throw std::runtime_error("Texture must be on GPU!");
		}

		std::vector<DG::IBuffer*> buffers = { mRasterAspect.mVertexBuffer };

		if (mRasterAspect.mIndexBuffer) {
			buffers.emplace_back(mRasterAspect.mIndexBuffer);
		}

		auto readProc = Buffer::BeginGPUMultiRead(buffers, mDevice, context);

		Promise<Geometry> result;

		auto lambda = [readProc, device, context, 
			geoHandle = Handle<Geometry>(this)](const TaskParams& e, BarrierOut, Promise<Geometry> output) {
			auto data = Buffer::FinishGPUMultiRead(context, readProc);

			bool bHasIndices = data.size() > 1;

			auto& vert = data[0];
			auto& vertDesc = readProc.mBufferDesc[0];
			
			std::vector<DG::BufferDesc> vertDescs = { vertDesc };
			std::vector<std::vector<uint8_t>> vertData = { std::move(vert) };
			auto& layout = geoHandle->GetLayout();

			Geometry geo;
			if (!bHasIndices) {
				geo.Set(layout,
					std::move(vertDescs), 
					std::move(vertData),
					geoHandle->GetDrawAttribs(),
					geoHandle->GetBoundingBox());
			} else {
				auto& indx = data[1];
				auto& indxDesc = readProc.mBufferDesc[1];

				geo.Set(layout,
					std::move(vertDescs),
					indxDesc,
					std::move(vertData),
					std::move(indx),
					geoHandle->GetIndexedDrawAttribs(),
					geoHandle->GetBoundingBox());
			}
			output = std::move(geo);
		};

		if (!readProc.mFence) {
			lambda(TaskParams(), Barrier(), result);
			return result;
		} else {
			FunctionPrototype<BarrierOut, Promise<Geometry>> prototype(std::move(lambda));
			Barrier gpuBarrier(readProc.mFence, readProc.mFenceCompletedValue);
			gpuBarrier.Node().OnlyThread(THREAD_MAIN);
			prototype(gpuBarrier, result)
				.SetName("Copy Staging Buffer to CPU")
				.OnlyThread(THREAD_MAIN);
			return result;
		}
	}

	std::filesystem::path Geometry::GetPath() const {
		return mSource.mPath;
	}

	Handle<IResource> Geometry::MoveIntoHandle() {
		return Handle<Geometry>(std::move(*this)).DownCast<IResource>();
	}

	void Geometry::BinarySerialize(std::ostream& output, IDependencyResolver* dependencies) {
		cereal::PortableBinaryOutputArchive arr(output);

		GeometryDataFloat data = Unpack();

		arr(mShared.mLayout);
		arr(data);
	}

	void Geometry::BinaryDeserialize(std::istream& input, const IDependencyResolver* dependencies) {
		cereal::PortableBinaryInputArchive arr(input);

		VertexLayout layout;
		GeometryDataFloat data;

		arr(layout);
		arr(data);

		Pack<uint32_t, float, float, float>(layout, data);
	}

	void Geometry::BinarySerializeReference(
		const std::filesystem::path& workingPath, 
		cereal::PortableBinaryOutputArchive& output) {
		auto params = mSource;
		params.mPath = std::filesystem::relative(params.mPath, workingPath);
		output(params);
	}

	void Geometry::BinaryDeserializeReference(
		const std::filesystem::path& workingPath,
		cereal::PortableBinaryInputArchive& input) {
		input(mSource);
		mSource.mPath = workingPath / mSource.mPath;
		mDevice = Device::Disk();
	}
}