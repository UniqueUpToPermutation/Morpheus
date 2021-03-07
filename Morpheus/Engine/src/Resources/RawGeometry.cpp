#include <Engine/Resources/RawGeometry.hpp>
#include <Engine/Resources/GeometryResource.hpp>
#include <Engine/Resources/ResourceSerialization.hpp>
#include <Engine/ThreadTasks.hpp>
#include <Engine/Resources/ResourceData.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/MaterialResource.hpp>

#include <fstream>

#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

using namespace Assimp;
using namespace std;

namespace Morpheus {
	
	void RawGeometry::CopyTo(RawGeometry* geometry) const {
		geometry->CopyFrom(*this);
	}

	void RawGeometry::CopyFrom(const RawGeometry& geometry) {
		mAabb = geometry.mAabb;
		mIndexBufferData = geometry.mIndexBufferData;
		mIndexBufferDesc = geometry.mIndexBufferDesc;
		mIndexedDrawAttribs = geometry.mIndexedDrawAttribs;
		mLayout = geometry.mLayout;
		mUnindexedDrawAttribs = geometry.mUnindexedDrawAttribs;
		mVertexBufferData = geometry.mVertexBufferData;
		mVertexBufferDesc = geometry.mVertexBufferDesc;
	}

	void RawGeometry::Set(const VertexLayout& layout,
		const DG::BufferDesc& vertexBufferDesc, 
		std::vector<uint8_t>&& vertexBufferData,
		const DG::DrawAttribs& unindexedDrawAttribs,
		const BoundingBox& aabb) {
		
		mLayout = layout;
		mVertexBufferDesc = vertexBufferDesc;
		mVertexBufferData = vertexBufferData;
		mUnindexedDrawAttribs = unindexedDrawAttribs;
		bHasIndexBuffer = false;
		mAabb = aabb;
	}

	void RawGeometry::Set(const VertexLayout& layout,
		const DG::BufferDesc& vertexBufferDesc,
		const DG::BufferDesc& indexBufferDesc,
		std::vector<uint8_t>&& vertexBufferData,
		std::vector<uint8_t>&& indexBufferData,
		DG::DrawIndexedAttribs& indexedDrawAttribs,
		const BoundingBox& aabb) {
		
		mLayout = layout;
		mVertexBufferDesc = vertexBufferDesc;
		mIndexBufferDesc = indexBufferDesc;
		mVertexBufferData = vertexBufferData;
		mIndexBufferData = indexBufferData;
		mIndexedDrawAttribs = indexedDrawAttribs;
		mAabb = aabb;

		bHasIndexBuffer = true;
	}

	RawGeometry::RawGeometry(RawGeometry&& other) :
		mVertexBufferData(std::move(other.mVertexBufferData)),
		mIndexBufferData(std::move(other.mIndexBufferData)),
		mVertexBufferDesc(other.mVertexBufferDesc),
		mIndexBufferDesc(other.mIndexBufferDesc),
		mAabb(other.mAabb),
		bHasIndexBuffer(other.bHasIndexBuffer),
		mIndexedDrawAttribs(other.mIndexedDrawAttribs),
		mUnindexedDrawAttribs(other.mUnindexedDrawAttribs),
		mLayout(other.mLayout) {
	}

	void RawGeometry::SpawnOnGPU(DG::IRenderDevice* device, 
		DG::IBuffer** vertexBufferOut, 
		DG::IBuffer** indexBufferOut) {

		DG::BufferData data;
		data.pData = &mVertexBufferData[0];
		data.DataSize = mVertexBufferData.size();
		device->CreateBuffer(mVertexBufferDesc, &data, vertexBufferOut);

		if (bHasIndexBuffer) {
			data.pData = &mIndexBufferData[0];
			data.DataSize = mIndexBufferData.size();
			device->CreateBuffer(mIndexBufferDesc, &data, indexBufferOut);
		}
	}

	void RawGeometry::SpawnOnGPU(DG::IRenderDevice* device,
		GeometryResource* writeTo) {

		DG::IBuffer* vertexBuffer = nullptr;
		DG::IBuffer* indexBuffer = nullptr;

		SpawnOnGPU(device, &vertexBuffer, &indexBuffer);

		if (indexBuffer) {
			writeTo->InitIndexed(vertexBuffer, indexBuffer, 0,
				mIndexedDrawAttribs, mLayout, mAabb);
		} else {
			writeTo->Init(vertexBuffer, 0, mUnindexedDrawAttribs, 
				mLayout, mAabb);
		}
	}

	void RawGeometry::LoadAssimpAsync(ThreadPool* pool, const LoadParams<GeometryResource>& params) {
		auto taskId = LoadAssimpAsyncDeferred(pool, params);
		pool->GetQueue().Schedule(taskId);
	}

	void RawGeometry::LoadAssimp(const LoadParams<GeometryResource>& params) {

		if (params.mMaterial && params.mPipeline) {
			throw std::runtime_error("Cannot specify both material and pipeline for raw geometry vertex layout!");
		}

		auto& source = params.mSource;
		cout << "Loading geometry " << source << "..." << endl;
		std::unique_ptr<Assimp::Importer> importer(new Assimp::Importer());
		const aiScene* pScene = importer->ReadFile(source.c_str(),
			aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices |
			aiProcess_GenUVCoords | aiProcess_CalcTangentSpace | aiProcessPreset_TargetRealtime_Quality);

		if (!pScene) {
			cout << "Error: failed to load " << source << endl;
			throw std::runtime_error("Failed to load geometry!");
		}

		if (!pScene->HasMeshes()) {
			cout << "Error: " << source << " has no meshes!" << endl;
			throw std::runtime_error("Geometry has no meshes!");
		}

		if (pScene->mNumMeshes > 1) {
			cout << "Warning: " << source << " has more than one mesh, we will just load the first." << endl;
		}

		const VertexLayout* layout = &params.mVertexLayout;

		if (params.mMaterial) {
			if (params.mMaterial->IsLoaded())
				layout = &params.mMaterial->GetVertexLayout();
			else 
				throw std::runtime_error("Material has not loaded yet!");
		}
		
		if (params.mPipeline) {
			if (params.mPipeline->IsLoaded())
				layout = &params.mPipeline->GetVertexLayout();
			else 
				throw std::runtime_error("Pipeline has not loaded yet!");
		}

		LoadAssimp(pScene, *layout);
	}

	TaskId RawGeometry::LoadAssimpAsyncDeferred(ThreadPool* pool,
		const LoadParams<GeometryResource>& params) {

		if (params.mMaterial && params.mPipeline) {
			throw std::runtime_error("Cannot specify both material and pipeline for raw geometry vertex layout!");
		}

		auto barrier = GetLoadBarrier();

		auto queue = pool->GetQueue();

		PipeId filePipe;
		TaskId readFile = ReadFileToMemoryJobDeferred(params.mSource, &queue, &filePipe);
		PipeId rawGeoPipe = queue.MakePipe();

		TaskId convertToRaw = queue.MakeTask([this, filePipe, params, rawGeoPipe](const TaskParams& taskParams) {
			try {
				auto& value = taskParams.mPool->ReadPipe(filePipe);
				std::unique_ptr<ReadFileToMemoryResult> contents(value.cast<ReadFileToMemoryResult*>());

				std::unique_ptr<Assimp::Importer> importer(new Assimp::Importer());

				const aiScene* pScene = importer->ReadFileFromMemory(contents->GetData(), contents->GetSize(), 
					aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices |
					aiProcess_GenUVCoords | aiProcess_CalcTangentSpace | aiProcessPreset_TargetRealtime_Quality,
					params.mSource.c_str());

				if (!pScene) {
					std::cout << importer->GetErrorString() << std::endl;
					throw std::runtime_error("Failed to load geometry!");
				}

				if (!pScene->HasMeshes()) {
					throw std::runtime_error("Geometry has no meshes!");
				}

				std::unique_ptr<RawGeometry> rawGeo(new RawGeometry());

				const VertexLayout* layout = &params.mVertexLayout;
				if (params.mPipeline)
					layout = &params.mPipeline->GetVertexLayout();
				if (params.mMaterial)
					layout = &params.mMaterial->GetVertexLayout();

				LoadAssimp(pScene, *layout);

				taskParams.mPool->WritePipe(rawGeoPipe, rawGeo.release());
			}
			catch (...) {
				taskParams.mPool->WritePipeException(rawGeoPipe, std::current_exception());
			}
		}, GetLoadBarrier());

		queue.Dependencies(convertToRaw).After(filePipe);

		if (params.mPipeline)
			queue.Dependencies(readFile).After(params.mPipeline->GetLoadBarrier());
		if (params.mMaterial)
			queue.Dependencies(readFile).After(params.mMaterial->GetLoadBarrier());

		return readFile;
	}

	void RawGeometry::LoadArchive(const uint8_t* rawArchive, const size_t length) {
		MemoryInputStream stream(rawArchive, length);
		cereal::PortableBinaryInputArchive ar(stream);
		Morpheus::Load(ar, this);
	}

	void RawGeometry::LoadArchive(const std::string& source) {
		std::ifstream f(source, std::ios::binary);

		if (f.is_open()) {
			cereal::PortableBinaryInputArchive ar(f);
			Morpheus::Load(ar, this);
			f.close();
		} else {
			throw std::runtime_error("Could not open file!");
		}
	}

	TaskId RawGeometry::LoadArchiveAsyncDeferred(const std::string& path, ThreadPool* pool) {
		auto queue = pool->GetQueue();

		PipeId filePipe;
		TaskId readFile = ReadFileToMemoryJobDeferred(path, &queue, &filePipe);

		TaskId fileToSelf = queue.MakeTask([this, filePipe](const TaskParams& taskParams) {
			auto& val = taskParams.mPool->ReadPipe(filePipe);
			std::unique_ptr<ReadFileToMemoryResult> result(val.cast<ReadFileToMemoryResult*>());
			this->LoadArchive(result->GetData(), result->GetSize());
		}, GetLoadBarrier());

		queue.Dependencies(fileToSelf).After(filePipe);

		return readFile;
	}

	void RawGeometry::LoadArchiveAsync(const std::string& source, ThreadPool* pool) {
		TaskId task = LoadArchiveAsyncDeferred(source, pool);
		pool->GetQueue().Schedule(task);
	}

	void RawGeometry::LoadAssimp(const aiScene* scene, const VertexLayout& layout) {
		auto& layoutElements = layout.mElements;
		std::vector<uint> offsets;
		offsets.emplace_back(0);

		for (auto& layoutItem : layoutElements) {
			uint size = GetSize(layoutItem.ValueType) * layoutItem.NumComponents;
			if (layoutItem.BufferSlot == 0) 
				offsets.emplace_back(offsets[offsets.size() - 1] + size);
			else 
				offsets.emplace_back(offsets[offsets.size() - 1]);
		}

		uint stride = offsets[offsets.size() - 1];

		// Override stride if specified.
		if (layout.mStride > 0) {
			stride = layout.mStride;
		}

		int positionOffset = -1;
		int uvOffset = -1;
		int normalOffset = -1;
		int tangentOffset = -1;
		int bitangentOffset = -1;

		auto verifyAttrib = [](const DG::LayoutElement& element) {
			if (element.BufferSlot != 0) {
				throw std::runtime_error("Buffer slot must be 0!");
			}

			if (element.ValueType != DG::VT_FLOAT32) {
				throw std::runtime_error("Attribute type must be VT_FLOAT32!");
			}
		};

		if (layout.mPosition >= 0) {
			auto& posAttrib = layoutElements[layout.mPosition];
			verifyAttrib(posAttrib);
			positionOffset = offsets[layout.mPosition];
		}

		if (layout.mUV >= 0) {
			auto& uvAttrib = layoutElements[layout.mUV];
			verifyAttrib(uvAttrib);
			uvOffset = offsets[layout.mUV];
		}

		if (layout.mNormal >= 0) {
			auto& normalAttrib = layoutElements[layout.mNormal];
			verifyAttrib(normalAttrib);
			normalOffset = offsets[layout.mNormal];
		}

		if (layout.mTangent >= 0) {
			auto& tangentAttrib = layoutElements[layout.mTangent];
			verifyAttrib(tangentAttrib);
			tangentOffset = offsets[layout.mTangent];
		}

		if (layout.mBitangent >= 0) {
			auto& bitangentAttrib = layoutElements[layout.mBitangent];
			verifyAttrib(bitangentAttrib);
			bitangentOffset = offsets[layout.mBitangent];	
		}

		uint nVerts;
		uint nIndices;

		aiMesh* mesh = scene->mMeshes[0];

		nVerts = mesh->mNumVertices;
		nIndices = mesh->mNumFaces * 3;

		std::vector<uint8_t> vert_buffer(nVerts * stride);
		std::vector<uint8_t> indx_buffer_raw(nIndices * sizeof(DG::Uint32));
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

		bool bHasUVs = mesh->HasTextureCoords(0);
		bool bHasNormals = mesh->HasNormals();
		bool bHasPositions = mesh->HasPositions();
		bool bHasBitangents = mesh->HasTangentsAndBitangents();
		bool bHasTangents = mesh->HasTangentsAndBitangents();

		if (positionOffset >= 0) {
			if (bHasPositions) {
				for (uint32_t i = 0, bufindx = positionOffset; i < nVerts; ++i, bufindx += stride) {
					float* position_ptr = reinterpret_cast<float*>(&vert_buffer[bufindx]);

					position_ptr[0] = mesh->mVertices[i].x;
					position_ptr[1] = mesh->mVertices[i].y;
					position_ptr[2] = mesh->mVertices[i].z;

					aabb.mLower = DG::min(aabb.mLower, DG::float3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z));
					aabb.mUpper = DG::max(aabb.mUpper, DG::float3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z));
				}
			} else {
				std::cout << "Warning: Pipeline expects positions, but model has none!" << std::endl;
				for (uint32_t i = 0, bufindx = positionOffset; i < nVerts; ++i, bufindx += stride) {
					float* position_ptr = reinterpret_cast<float*>(&vert_buffer[bufindx]);

					position_ptr[0] = 0.0f;
					position_ptr[1] = 0.0f;
					position_ptr[2] = 0.0f;

					aabb.mLower = DG::min(aabb.mLower, DG::float3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z));
					aabb.mUpper = DG::max(aabb.mUpper, DG::float3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z));
				}
			}
		}

		if (uvOffset >= 0) {
			if (bHasUVs) {
				for (uint32_t i = 0, bufindx = uvOffset; i < nVerts; ++i, bufindx += stride) {
					float* uv_ptr = reinterpret_cast<float*>(&vert_buffer[bufindx]);

					uv_ptr[0] = mesh->mTextureCoords[0][i].x;
					uv_ptr[1] = mesh->mTextureCoords[0][i].y;
				}
			} else {
				std::cout << "Warning: Pipeline expects UVs, but model has none!" << std::endl;
				for (uint32_t i = 0, bufindx = uvOffset; i < nVerts; ++i, bufindx += stride) {
					float* uv_ptr = reinterpret_cast<float*>(&vert_buffer[bufindx]);

					uv_ptr[0] = 0.0f;
					uv_ptr[1] = 0.0f;
				}
			}
		}

		if (normalOffset >= 0) {
			if (bHasNormals) {
				for (uint32_t i = 0, bufindx = normalOffset; i < nVerts; ++i, bufindx += stride) {
					float* normal_ptr = reinterpret_cast<float*>(&vert_buffer[bufindx]);

					normal_ptr[0] = mesh->mNormals[i].x;
					normal_ptr[1] = mesh->mNormals[i].y;
					normal_ptr[2] = mesh->mNormals[i].z;
				}
			} else {
				std::cout << "Warning: Pipeline expects normals, but model has none!" << std::endl;
				for (uint32_t i = 0, bufindx = normalOffset; i < nVerts; ++i, bufindx += stride) {
					float* normal_ptr = reinterpret_cast<float*>(&vert_buffer[bufindx]);

					normal_ptr[0] = 0.0f;
					normal_ptr[1] = 0.0f;
					normal_ptr[2] = 0.0f;
				}
			}
		}

		if (tangentOffset >= 0) {
			if (bHasTangents) {
				for (uint32_t i = 0, bufindx = tangentOffset; i < nVerts; ++i, bufindx += stride) {
					float* tangent_ptr = reinterpret_cast<float*>(&vert_buffer[bufindx]);

					tangent_ptr[0] = mesh->mTangents[i].x;
					tangent_ptr[1] = mesh->mTangents[i].y;
					tangent_ptr[2] = mesh->mTangents[i].z;
				}
			} else {
				std::cout << "Warning: Pipeline expects tangents, but model has none!" << std::endl;
				for (uint32_t i = 0, bufindx = tangentOffset; i < nVerts; ++i, bufindx += stride) {
					float* tangent_ptr = reinterpret_cast<float*>(&vert_buffer[bufindx]);

					tangent_ptr[0] = 0.0f;
					tangent_ptr[1] = 0.0f;
					tangent_ptr[2] = 0.0f;
				}
			}
		}

		if (bitangentOffset >= 0) {
			if (bHasBitangents) {
				for (uint32_t i = 0, bufindx = tangentOffset; i < nVerts; ++i, bufindx += stride) {
					float* bitangent_ptr = reinterpret_cast<float*>(&vert_buffer[bufindx]);

					bitangent_ptr[0] = mesh->mBitangents[i].x;
					bitangent_ptr[1] = mesh->mBitangents[i].y;
					bitangent_ptr[2] = mesh->mBitangents[i].z;
				}
			} else {
				std::cout << "Warning: Pipeline expects bitangents, but model has none!" << std::endl;
				for (uint32_t i = 0, bufindx = tangentOffset; i < nVerts; ++i, bufindx += stride) {
					float* bitangent_ptr = reinterpret_cast<float*>(&vert_buffer[bufindx]);

					bitangent_ptr[0] = 0.0f;
					bitangent_ptr[1] = 0.0f;
					bitangent_ptr[2] = 0.0f;
				}
			}
		}

		for (uint32_t i_face = 0, i = 0; i_face < mesh->mNumFaces; ++i_face) {
			indx_buffer[i++] = mesh->mFaces[i_face].mIndices[0];
			indx_buffer[i++] = mesh->mFaces[i_face].mIndices[1];
			indx_buffer[i++] = mesh->mFaces[i_face].mIndices[2];
		}

		// Upload everything to the GPU
		DG::BufferDesc vertexBufferDesc;
		vertexBufferDesc.Usage         = DG::USAGE_IMMUTABLE;
		vertexBufferDesc.BindFlags     = DG::BIND_VERTEX_BUFFER;
		vertexBufferDesc.uiSizeInBytes = vert_buffer.size();

		DG::BufferDesc indexBufferDesc;
		indexBufferDesc.Usage 			= DG::USAGE_IMMUTABLE;
		indexBufferDesc.BindFlags 		= DG::BIND_INDEX_BUFFER;
		indexBufferDesc.uiSizeInBytes 	= indx_buffer_raw.size();

		DG::DrawIndexedAttribs indexedAttribs;
		indexedAttribs.IndexType 	= DG::VT_UINT32;
		indexedAttribs.NumIndices 	= indx_buffer_raw.size() / sizeof(DG::Uint32);
		
		// Write to output raw geometry
		Set(layout, vertexBufferDesc, 
			indexBufferDesc, 
			std::move(vert_buffer), 
			std::move(indx_buffer_raw),
			indexedAttribs, aabb);
	}

	void RawGeometry::Load(const LoadParams<GeometryResource>& params) {
		auto pos = params.mSource.rfind('.');
		if (pos == std::string::npos) {
			throw std::runtime_error("Source does not have file extension!");
		}
		auto ext = params.mSource.substr(pos);

		if (ext == GEOMETRY_ARCHIVE_EXTENSION) {
			LoadArchive(params.mSource);
		} else {
			LoadAssimp(params);
		}
	}

	TaskId RawGeometry::LoadAsyncDeferred(const LoadParams<GeometryResource>& params, ThreadPool* pool) {
		auto pos = params.mSource.rfind('.');
		if (pos == std::string::npos) {
			throw std::runtime_error("Source does not have file extension!");
		}
		auto ext = params.mSource.substr(pos);

		if (ext == GEOMETRY_ARCHIVE_EXTENSION) {
			return LoadArchiveAsyncDeferred(params.mSource, pool);
		} else {
			return LoadAssimpAsyncDeferred(pool, params);
		}
	}

	void RawGeometry::LoadAsync(const LoadParams<GeometryResource>& params, ThreadPool* pool) {
		TaskId task = LoadAsyncDeferred(params, pool);
		pool->GetQueue().Schedule(task);
	}

	void RawGeometry::Save(const std::string& destination) {
		std::ofstream f(destination, std::ios::binary);

		if (f.is_open()) {
			cereal::PortableBinaryOutputArchive ar(f);
			Morpheus::Save(ar, this);
			f.close();
		} else {
			throw std::runtime_error("Could not open file for writing!");
		}
	}

	void RawGeometry::Clear() {
		mVertexBufferData.clear();
		mIndexBufferData.clear();
	}
}