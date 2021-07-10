#include <Engine/Resources/Geometry.hpp>
#include <Engine/Resources/ResourceSerialization.hpp>
#include <Engine/Resources/ResourceData.hpp>

#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

using namespace Assimp;
using namespace std;

namespace Morpheus {
	template <typename T>
	ResourceTask<T> LoadTemplated(GraphicsDevice device, 
		const LoadParams<Geometry>& params) {
		Promise<T> promise;
		Future<T> future(promise);

		struct Data {
			Geometry mRaw;
		};

		Task task([params, device, promise = std::move(promise), data = Data()](const TaskParams& e) mutable {
			if (e.mTask->BeginSubTask()) {
				data.mRaw.LoadRaw(params);
				e.mTask->EndSubTask();
			}

			if (device.mGpuDevice) {
				// GPU devices can only create objects on the main thread!
				e.mTask->RequestThreadSwitch(e, ASSIGN_THREAD_MAIN);
			}

			if (e.mTask->BeginSubTask()) {
				Geometry* geo = new Geometry(device, data.mRaw);
				promise.Set(geo, e.mQueue);
				e.mTask->EndSubTask();

				if constexpr (std::is_same_v<T, Handle<Geometry>>) {
					geo->Release();
				}
			}
		}, 
		std::string("Load ") + params.mSource, 
		TaskType::FILE_IO);

		ResourceTask<T> resourceTask;
		resourceTask.mTask = std::move(task);
		resourceTask.mFuture = std::move(future);

		return resourceTask;
	}

	Geometry Geometry::Prefabs::MaterialBall(GraphicsDevice device, const VertexLayout& layout) {
		return Geometry(device, Geometry::Prefabs::MaterialBall(layout));
	}

	Geometry Geometry::Prefabs::Box(GraphicsDevice device, const VertexLayout& layout) {
		return Geometry(device, Geometry::Prefabs::Box(layout));
	}

	Geometry Geometry::Prefabs::Sphere(GraphicsDevice device, const VertexLayout& layout) {
		return Geometry(device, Geometry::Prefabs::Sphere(layout));
	}

	Geometry Geometry::Prefabs::BlenderMonkey(GraphicsDevice device, const VertexLayout& layout) {
		return Geometry(device, Geometry::Prefabs::BlenderMonkey(layout));
	}

	Geometry Geometry::Prefabs::Torus(GraphicsDevice device, const VertexLayout& layout) {
		return Geometry(device, Geometry::Prefabs::Torus(layout));
	}

	Geometry Geometry::Prefabs::Plane(GraphicsDevice device, const VertexLayout& layout) {
		return Geometry(device, Geometry::Prefabs::Plane(layout));
	}

	Geometry Geometry::Prefabs::StanfordBunny(GraphicsDevice device, const VertexLayout& layout) {
		return Geometry(device, Geometry::Prefabs::StanfordBunny(layout));
	}

	Geometry Geometry::Prefabs::UtahTeapot(GraphicsDevice device, const VertexLayout& layout) {
		return Geometry(device, Geometry::Prefabs::UtahTeapot(layout));
	}

	void Geometry::Set(DG::IBuffer* vertexBuffer, 
		DG::IBuffer* indexBuffer,
		uint vertexBufferOffset, 
		const DG::DrawIndexedAttribs& attribs,
		const VertexLayout& layout,
		const BoundingBox& aabb) {
		mFlags |= RESOURCE_RASTERIZER_ASPECT;

		mRasterAspect.mVertexBuffer = vertexBuffer;
		mRasterAspect.mIndexBuffer = indexBuffer;
		mRasterAspect.mVertexBufferOffset = vertexBufferOffset;
		mShared.mLayout = layout,
		mShared.mBoundingBox = aabb;
		mShared.mIndexedAttribs = attribs;
	}

	void Geometry::Set(DG::IBuffer* vertexBuffer,
		uint vertexBufferOffset,
		const DG::DrawAttribs& attribs,
		const VertexLayout& layout,
		const BoundingBox& aabb) {
		mFlags |= RESOURCE_RASTERIZER_ASPECT;

		mRasterAspect.mVertexBuffer = vertexBuffer;
		mRasterAspect.mIndexBuffer = nullptr;
		mRasterAspect.mVertexBufferOffset = vertexBufferOffset;
		mShared.mLayout = layout;
		mShared.mBoundingBox = aabb;
		mShared.mUnindexedAttribs = attribs;
	}

	void Geometry::CreateRasterAspect(DG::IRenderDevice* device, const Geometry* geometry) {
		
		mFlags |= RESOURCE_RASTERIZER_ASPECT;

		DG::IBuffer* vertexBuffer = nullptr;
		DG::IBuffer* indexBuffer = nullptr;

		geometry->SpawnOnGPU(device, &vertexBuffer, &indexBuffer);

		if (indexBuffer) {
			Set(vertexBuffer, indexBuffer, 0,
				geometry->GetIndexedDrawAttribs(), 
				mShared.mLayout, geometry->GetBoundingBox());
		} else {
			Set(vertexBuffer, 0, geometry->GetDrawAttribs(), 
				mShared.mLayout,  geometry->GetBoundingBox());
		}
	}

	void Geometry::CreateRaytraceAspect(Raytrace::IRaytraceDevice* device, const Geometry* source) {
		throw std::runtime_error("Not implemented yet!");
	}

	void Geometry::CreateDeviceAspect(GraphicsDevice device, const Geometry* source) {
		if (device.mGpuDevice)
			CreateRasterAspect(device.mGpuDevice, source);
		else if (device.mRtDevice)
			CreateRaytraceAspect(device.mRtDevice, source);
		else
			throw std::runtime_error("Device cannot be null!");
	}

	ResourceTask<Handle<Geometry>> Geometry::LoadHandle(
		GraphicsDevice device, const LoadParams<Geometry>& params) {
		return LoadTemplated<Handle<Geometry>>(device, params);
	}

	ResourceTask<Geometry*> Geometry::Load(GraphicsDevice device, 
		const LoadParams<Geometry>& params) {
		return LoadTemplated<Geometry*>(device, params);
	}

	void Geometry::CopyTo(Geometry* geometry) const {
		geometry->CopyFrom(*this);
	}

	void Geometry::CopyFrom(const Geometry& geometry) {
		if (geometry.mFlags & RESOURCE_RASTERIZER_ASPECT)
			throw std::runtime_error("Cannot copy GPU geometry with CopyFrom!");

		mFlags = geometry.mFlags;
		mRawAspect = geometry.mRawAspect;
		mShared = geometry.mShared;
	}

	void Geometry::Set(const VertexLayout& layout,
		std::vector<DG::BufferDesc>&& vertexBufferDescs, 
		std::vector<std::vector<uint8_t>>&& vertexBufferDatas,
		const DG::DrawAttribs& unindexedDrawAttribs,
		const BoundingBox& aabb) {

		mFlags |= RESOURCE_RAW_ASPECT;
		
		mRawAspect.mVertexBufferDescs = std::move(vertexBufferDescs);
		mRawAspect.mVertexBufferDatas = std::move(vertexBufferDatas);
		mRawAspect.bHasIndexBuffer = false;

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

		mFlags |= RESOURCE_RAW_ASPECT;
		
		mRawAspect.mVertexBufferDescs = std::move(vertexBufferDescs);
		mRawAspect.mIndexBufferDesc = indexBufferDesc;
		mRawAspect.mVertexBufferDatas = std::move(vertexBufferDatas);
		mRawAspect.mIndexBufferData = indexBufferData;
		mRawAspect.bHasIndexBuffer = true;

		mShared.mIndexedAttribs = indexedDrawAttribs;
		mShared.mLayout = layout;
		mShared.mBoundingBox = aabb;
	}

	void Geometry::AdoptData(Geometry&& other) {
		mRawAspect = std::move(other.mRawAspect);
		mShared = std::move(other.mShared);
		mRasterAspect = std::move(other.mRasterAspect);
		mRtAspect = std::move(other.mRtAspect);
		
		mFlags = other.mFlags;
		mBarrier.mOut.SetFinishedUnsafe(other.mBarrier.mOut.Lock().IsFinished());
	}

	void Geometry::SpawnOnGPU(DG::IRenderDevice* device, 
		DG::IBuffer** vertexBufferOut, 
		DG::IBuffer** indexBufferOut) const {

		if (!(mFlags & RESOURCE_RAW_ASPECT))
			throw std::runtime_error("Spawn on GPU requires geometry have a raw aspect!");

		if (mRawAspect.mVertexBufferDatas.size() == 0)
			throw std::runtime_error("Spawning on GPU requires at least one channel!");

		DG::BufferData data;
		data.pData = &mRawAspect.mVertexBufferDatas[0][0];
		data.DataSize = mRawAspect.mVertexBufferDatas[0].size();
		device->CreateBuffer(mRawAspect.mVertexBufferDescs[0], &data, vertexBufferOut);

		if (mRawAspect.bHasIndexBuffer) {
			data.pData = &mRawAspect.mIndexBufferData[0];
			data.DataSize = mRawAspect.mIndexBufferData.size();
			device->CreateBuffer(mRawAspect.mIndexBufferDesc, &data, indexBufferOut);
		}
	}

	Task Geometry::LoadAssimpRawTask(const LoadParams<Geometry>& params) {

		Task task([this, params](const TaskParams& e) {
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
	
			LoadAssimpRaw(pScene, *layout);
		},
		std::string("Load Raw Geometry ") + params.mSource + " (Assimp)",
		TaskType::FILE_IO);

		mBarrier.mIn.Lock().Connect(&task->Out());

		return task;
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

		mFlags |= RESOURCE_RAW_ASPECT;

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
		uint32_t indices[],
		float positions[],
		float uvs[],
		float normals[],
		float tangents[],
		float bitangents[]) {

		Unpack<uint32_t, float, float>(layout,
			vertex_count, index_count,
			indices,
			positions,
			uvs,
			normals,
			tangents,
			bitangents);
	}

	void Geometry::LoadAssimpRaw(const aiScene* scene, const VertexLayout& layout) {

		uint nVerts;
		uint nIndices;

		if (!scene->HasMeshes()) {
			throw std::runtime_error("Assimp scene has no meshes!");
		}

		aiMesh* mesh = scene->mMeshes[0];

		nVerts = mesh->mNumVertices;
		nIndices = mesh->mNumFaces * 3;

		Unpack<aiFace, aiVector3D, aiVector3D>(layout,
			nVerts, nIndices,
			mesh->mFaces,
			mesh->mVertices,
			mesh->mTextureCoords[0],
			mesh->mNormals,
			mesh->mTangents,
			mesh->mBitangents);
	}

	Task Geometry::LoadRawTask(const LoadParams<Geometry>& params) {
		auto pos = params.mSource.rfind('.');
		if (pos == std::string::npos) {
			throw std::runtime_error("Source does not have file extension!");
		}
		auto ext = params.mSource.substr(pos);

		return LoadAssimpRawTask(params);
	}

	void Geometry::Clear() {
		mRasterAspect = RasterizerAspect();
		mRawAspect = RawAspect();
		mRtAspect = RaytracerAspect();
		mShared = SharedAspect();
		mBarrier.Reset();
	}
}