#include <Engine/Resources/Geometry.hpp>
#include <Engine/Resources/ResourceSerialization.hpp>
#include <Engine/Resources/ResourceData.hpp>

#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

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
		DG::DrawIndexedAttribs& indexedDrawAttribs,
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

	UniqueFuture<Geometry> Geometry::ReadAssimpRawAsync(const LoadParams<Geometry>& params) {

		FunctionPrototype<Promise<Geometry>> prototype([params](const TaskParams& e, Promise<Geometry> result) {
			std::vector<uint8_t> data;
			ReadBinaryFile(params.mSource, data);

			Assimp::Importer importer;

			unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | 
				aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices |
				aiProcess_GenUVCoords | aiProcess_CalcTangentSpace | 
				aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_Quality;

			const aiScene* pScene = importer.ReadFileFromMemory(&data[0], data.size(), 
				flags,
				params.mSource.c_str());
			
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
	struct V3Unpacker;

	template <typename T>
	struct V2Unpacker;

	template <typename T>
	struct I3Unpacker;

	template <>
	struct V3Unpacker<float> {
		static constexpr size_t Stride = 3;

		inline static void Unpack(float* dest, const float* src) {
			dest[0] = src[0];
			dest[1] = src[1];
			dest[2] = src[2];
		}
	};

	template <>
	struct V3Unpacker<DG::float3> {
		static constexpr size_t Stride = 1;

		inline static void Unpack(float* dest, const DG::float3* src) {
			dest[0] = src->x;
			dest[1] = src->y;
			dest[2] = src->z;
		}
	};

	template <>
	struct V2Unpacker<float> {
		static constexpr size_t Stride = 2;

		inline static void Unpack(float* dest, const float* src) {
			dest[0] = src[0];
			dest[1] = src[1];
		}
	};

	template <>
	struct V2Unpacker<DG::float2> {
		static constexpr size_t Stride = 1;

		inline static void Unpack(float* dest, const DG::float2* src) {
			dest[0] = src->x;
			dest[1] = src->y;
		}
	};

	template <>
	struct V3Unpacker<aiVector3D> {
		static constexpr size_t Stride = 1;

		inline static void Unpack(float* dest, const aiVector3D* src) {
			dest[0] = src->x;
			dest[1] = src->y;
			dest[2] = src->z;
		}
	};

	template <>
	struct V2Unpacker<aiVector3D> {
		static constexpr size_t Stride = 1;

		inline static void Unpack(float* dest, const aiVector3D* src) {
			dest[0] = src->x;
			dest[1] = src->y;
		}
	};

	template <>
	struct I3Unpacker<aiFace> {
		static constexpr size_t Stride = 1;

		inline static void Unpack(uint32_t* dest, const aiFace* src) {
			dest[0] = src->mIndices[0];
			dest[1] = src->mIndices[1];
			dest[2] = src->mIndices[2];
		}
	};

	template <>
	struct I3Unpacker<uint32_t> {
		static constexpr size_t Stride = 3;

		inline static void Unpack(uint32_t* dest, const uint32_t* src) {
			dest[0] = src[0];
			dest[1] = src[1];
			dest[2] = src[2];
		}
	};

	template <typename I3T, typename V3T, typename V2T>
	void Geometry::Unpack(const VertexLayout& layout,
		size_t vertex_count,
		size_t index_count,
		const I3T indices[],
		const V3T positions[],
		const V2T uvs[],
		const V3T normals[],
		const V3T tangents[],
		const V3T bitangents[]) {

		mDevice = Device::CPU();

		auto& layoutElements = layout.mElements;
		std::vector<size_t> offsets;
		std::vector<size_t> strides;
		std::vector<size_t> channel_sizes;
		
		ComputeLayoutProperties(vertex_count, layout, offsets, strides, channel_sizes);

		uint channelCount = channel_sizes.size();

		int positionOffset = -1;
		int positionChannel = -1;
		int positionStride = -1;

		int uvOffset = -1;
		int uvChannel = -1;
		int uvStride = -1;

		int normalOffset = -1;
		int normalChannel = -1;
		int normalStride = -1;

		int tangentOffset = -1;
		int tangentChannel = -1;
		int tangentStride = -1;

		int bitangentOffset = -1;
		int bitangentChannel = -1;
		int bitangentStride = -1;

		auto verifyAttrib = [](const DG::LayoutElement& element) {
			if (element.ValueType != DG::VT_FLOAT32) {
				throw std::runtime_error("Attribute type must be VT_FLOAT32!");
			}
		};

		if (layout.mPosition >= 0) {
			auto& posAttrib = layoutElements[layout.mPosition];
			verifyAttrib(posAttrib);
			positionOffset = offsets[layout.mPosition];
			positionChannel = posAttrib.BufferSlot;
			positionStride = strides[layout.mPosition];
		}

		if (layout.mUV >= 0) {
			auto& uvAttrib = layoutElements[layout.mUV];
			verifyAttrib(uvAttrib);
			uvOffset = offsets[layout.mUV];
			uvChannel = uvAttrib.BufferSlot;
			uvStride = strides[layout.mUV];
		}

		if (layout.mNormal >= 0) {
			auto& normalAttrib = layoutElements[layout.mNormal];
			verifyAttrib(normalAttrib);
			normalOffset = offsets[layout.mNormal];
			normalChannel = normalAttrib.BufferSlot;
			normalStride = strides[layout.mNormal];
		}

		if (layout.mTangent >= 0) {
			auto& tangentAttrib = layoutElements[layout.mTangent];
			verifyAttrib(tangentAttrib);
			tangentOffset = offsets[layout.mTangent];
			tangentChannel = tangentAttrib.BufferSlot;
			tangentStride = strides[layout.mTangent];
		}

		if (layout.mBitangent >= 0) {
			auto& bitangentAttrib = layoutElements[layout.mBitangent];
			verifyAttrib(bitangentAttrib);
			bitangentOffset = offsets[layout.mBitangent];
			bitangentChannel = bitangentAttrib.BufferSlot;
			bitangentStride = strides[layout.mBitangent];
		}

		std::vector<std::vector<uint8_t>> vert_buffers(channelCount);

		for (int i = 0; i < channelCount; ++i)
			vert_buffers[i] = std::vector<uint8_t>(channel_sizes[i]);

		std::vector<uint8_t> indx_buffer_raw(index_count * sizeof(DG::Uint32));
		DG::Uint32* indx_buffer = (DG::Uint32*)(&indx_buffer_raw[0]);

		BoundingBox aabb;
		aabb.mLower = DG::float3(
			std::numeric_limits<float>::infinity(),
			std::numeric_limits<float>::infinity(),
			std::numeric_limits<float>::infinity());
		aabb.mUpper = DG::float3(
			-std::numeric_limits<float>::infinity(),
			-std::numeric_limits<float>::infinity(),
			-std::numeric_limits<float>::infinity());

		bool bHasUVs = uvs != nullptr;
		bool bHasNormals = normals != nullptr;
		bool bHasPositions = positions != nullptr;
		bool bHasBitangents = bitangents != nullptr;
		bool bHasTangents = tangents != nullptr;

		if (positionOffset >= 0) {
			auto& channel = vert_buffers[positionChannel];
			auto stride = positionStride;
			if (bHasPositions) {
				size_t end = vertex_count * V3Unpacker<V3T>::Stride;
				for (size_t i = 0, bufindx = positionOffset; i < end; 
					i += V3Unpacker<V3T>::Stride, bufindx += stride) {

					float* position_ptr = reinterpret_cast<float*>(&channel[bufindx]);

					V3Unpacker<V3T>::Unpack(position_ptr, &positions[i]);

					aabb.mLower = DG::min(aabb.mLower, DG::float3(position_ptr[0], position_ptr[1], position_ptr[2]));
					aabb.mUpper = DG::max(aabb.mUpper, DG::float3(position_ptr[0], position_ptr[1], position_ptr[2]));
				}
			} else {
				std::cout << "Warning: Pipeline expects positions, but model has none!" << std::endl;
				for (size_t i = 0, bufindx = positionOffset; i < vertex_count; ++i, bufindx += stride) {
					float* position_ptr = reinterpret_cast<float*>(&channel[bufindx]);

					position_ptr[0] = 0.0f;
					position_ptr[1] = 0.0f;
					position_ptr[2] = 0.0f;
				}

				aabb.mLower = DG::float3(0.0f, 0.0f, 0.0f);
				aabb.mUpper = DG::float3(0.0f, 0.0f, 0.0f);
			}
		}

		if (uvOffset >= 0) {
			auto& channel = vert_buffers[uvChannel];
			auto stride = uvStride;
			if (bHasUVs) {
				size_t end = vertex_count * V2Unpacker<V3T>::Stride;
				for (size_t i = 0, bufindx = uvOffset; i < end; 
					i += V2Unpacker<V2T>::Stride, bufindx += stride) {
					float* uv_ptr = reinterpret_cast<float*>(&channel[bufindx]);

					V2Unpacker<V2T>::Unpack(uv_ptr, &uvs[i]);
				}
			} else {
				std::cout << "Warning: Pipeline expects UVs, but model has none!" << std::endl;
				for (size_t i = 0, bufindx = uvOffset; i < vertex_count; ++i, bufindx += stride) {
					float* uv_ptr = reinterpret_cast<float*>(&channel[bufindx]);

					uv_ptr[0] = 0.0f;
					uv_ptr[1] = 0.0f;
				}
			}
		}

		if (normalOffset >= 0) {
			auto& channel = vert_buffers[normalChannel];
			auto stride = normalStride;
			if (bHasNormals) {
				size_t end = vertex_count * V3Unpacker<V3T>::Stride;
				for (size_t i = 0, bufindx = normalOffset; i < end;
					i += V3Unpacker<V3T>::Stride, bufindx += stride) {
					float* normal_ptr = reinterpret_cast<float*>(&channel[bufindx]);

					V3Unpacker<V3T>::Unpack(normal_ptr, &normals[i]);
				}
			} else {
				std::cout << "Warning: Pipeline expects normals, but model has none!" << std::endl;
				for (size_t i = 0, bufindx = normalOffset; i < vertex_count; ++i, bufindx += stride) {
					float* normal_ptr = reinterpret_cast<float*>(&channel[bufindx]);

					normal_ptr[0] = 0.0f;
					normal_ptr[1] = 0.0f;
					normal_ptr[2] = 0.0f;
				}
			}
		}

		if (tangentOffset >= 0) {
			auto& channel = vert_buffers[tangentChannel];
			auto stride = tangentStride;
			if (bHasTangents) {
				size_t end = vertex_count * V3Unpacker<V3T>::Stride;
				for (size_t i = 0, bufindx = tangentOffset; i < end;
					i += V3Unpacker<V3T>::Stride, bufindx += stride) {

					float* tangent_ptr = reinterpret_cast<float*>(&channel[bufindx]);

					V3Unpacker<V3T>::Unpack(tangent_ptr, &tangents[i]);
				}
			} else {
				std::cout << "Warning: Pipeline expects tangents, but model has none!" << std::endl;
				for (size_t i = 0, bufindx = tangentOffset; i < vertex_count; ++i, bufindx += stride) {
					float* tangent_ptr = reinterpret_cast<float*>(&channel[bufindx]);

					tangent_ptr[0] = 0.0f;
					tangent_ptr[1] = 0.0f;
					tangent_ptr[2] = 0.0f;
				}
			}
		}

		if (bitangentOffset >= 0) {
			auto& channel = vert_buffers[bitangentChannel];
			auto stride = bitangentStride;
			if (bHasBitangents) {
				size_t end = vertex_count * V3Unpacker<V3T>::Stride;
				for (size_t i = 0, bufindx = tangentOffset; i < end;
					i += V3Unpacker<V3T>::Stride, bufindx += stride) {
					float* bitangent_ptr = reinterpret_cast<float*>(&channel[bufindx]);

					V3Unpacker<V3T>::Unpack(bitangent_ptr, &bitangents[i]);
				}
			} else {
				std::cout << "Warning: Pipeline expects bitangents, but model has none!" << std::endl;
				for (size_t i = 0, bufindx = tangentOffset; i < vertex_count; ++i, bufindx += stride) {
					float* bitangent_ptr = reinterpret_cast<float*>(&channel[bufindx]);

					bitangent_ptr[0] = 0.0f;
					bitangent_ptr[1] = 0.0f;
					bitangent_ptr[2] = 0.0f;
				}
			}
		}

		for (size_t read_idx = 0, write_idx = 0; 
			write_idx < index_count;
			read_idx += I3Unpacker<I3T>::Stride,
			write_idx += 3) {

			I3Unpacker<I3T>::Unpack(&indx_buffer[write_idx], &indices[read_idx]);
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

	void Geometry::FromMemory(const VertexLayout& layout,
		size_t vertex_count,
		size_t index_count,
		const uint32_t indices[],
		const float positions[],
		const float uvs[],
		const float normals[],
		const float tangents[],
		const float bitangents[]) {

		Unpack<uint32_t, float, float>(layout,
			vertex_count, index_count,
			indices,
			positions,
			uvs,
			normals,
			tangents,
			bitangents);
	}

	Geometry Geometry::ReadAssimpRaw(const aiScene* scene, const VertexLayout& layout) {

		uint nVerts;
		uint nIndices;

		if (!scene->HasMeshes()) {
			throw std::runtime_error("Assimp scene has no meshes!");
		}

		aiMesh* mesh = scene->mMeshes[0];

		nVerts = mesh->mNumVertices;
		nIndices = mesh->mNumFaces * 3;

		Geometry result;

		result.Unpack<aiFace, aiVector3D, aiVector3D>(layout,
			nVerts, nIndices,
			mesh->mFaces,
			mesh->mVertices,
			mesh->mTextureCoords[0],
			mesh->mNormals,
			mesh->mTangents,
			mesh->mBitangents);

		return result;
	}

	UniqueFuture<Geometry> Geometry::ReadAsync(const LoadParams<Geometry>& params) {
		auto pos = params.mSource.rfind('.');
		if (pos == std::string::npos) {
			throw std::runtime_error("Source does not have file extension!");
		}
		auto ext = params.mSource.substr(pos);

		return ReadAssimpRawAsync(params);
	}

	void Geometry::Clear() {
		mRasterAspect = RasterizerAspect();
		mCpuAspect = CpuAspect();
		mShared = SharedAspect();
		mExtAspect = ExternalAspect<ExtObjectType::GEOMETRY>();

		mDevice = Device::None();
	}

	void Geometry::RegisterMetaData() {
		meta<Geometry>().type("Geometry"_hs)
			.base<IResource>();
	}

	Geometry Geometry::To(Device device, Context context) {
		return std::move(ToAsync(device, context).Evaluate());
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

	UniqueFuture<Geometry> Geometry::GPUToCPUAsync(Device device, Context context) const {
		throw std::runtime_error("Not implemented!");
	}
}