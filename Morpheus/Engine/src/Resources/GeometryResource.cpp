#include <Engine/Resources/GeometryResource.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>

#include <Engine/ThreadTasks.hpp>

#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

using namespace Assimp;
using namespace std;

namespace Morpheus {

	void RawGeometry::Set(PipelineResource* pipeline,
		const DG::BufferDesc& vertexBufferDesc, 
		std::vector<uint8_t>&& vertexBufferData,
		const DG::DrawAttribs& unindexedDrawAttribs,
		const BoundingBox& aabb) {

		if (mPipeline)
			mPipeline->Release();
			
		mPipeline = pipeline;
		mPipeline->AddRef();

		mVertexBufferDesc = vertexBufferDesc;
		mVertexBufferData = vertexBufferData;
		mUnindexedDrawAttribs = unindexedDrawAttribs;
		bHasIndexBuffer = false;
		mAabb = aabb;
	}

	void RawGeometry::Set(PipelineResource* pipeline,
		const DG::BufferDesc& vertexBufferDesc,
		const DG::BufferDesc& indexBufferDesc,
		std::vector<uint8_t>&& vertexBufferData,
		std::vector<uint8_t>&& indexBufferData,
		DG::DrawIndexedAttribs& indexedDrawAttribs,
		const BoundingBox& aabb) {
		if (mPipeline)
			mPipeline->Release();

		mPipeline = pipeline;
		mPipeline->AddRef();

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
		mUnindexedDrawAttribs(other.mUnindexedDrawAttribs) {

		if (mPipeline)
			mPipeline->Release();
			
		mPipeline = other.mPipeline;
		mPipeline->AddRef();
	}

	RawGeometry::~RawGeometry() {
		if (mPipeline)
			mPipeline->Release();
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
				mIndexedDrawAttribs, mPipeline, mAabb);
		} else {
			writeTo->Init(vertexBuffer, 0, mUnindexedDrawAttribs, 
				mPipeline, mAabb);
		}
	}

	void GeometryResource::InitIndexed(DG::IBuffer* vertexBuffer, 
		DG::IBuffer* indexBuffer,
		uint vertexBufferOffset, 
		const DG::DrawIndexedAttribs& attribs,
		PipelineResource* pipeline, 
		const BoundingBox& aabb) {
		mVertexBuffer = vertexBuffer;
		mIndexBuffer = indexBuffer;
		mVertexBufferOffset = vertexBufferOffset;
		mPipeline = pipeline;
		mBoundingBox = aabb;
		mIndexedAttribs = attribs;
		pipeline->AddRef();
	}

	void GeometryResource::Init(DG::IBuffer* vertexBuffer,
		uint vertexBufferOffset,
		const DG::DrawAttribs& attribs,
		PipelineResource* pipeline, 
		const BoundingBox& aabb) {
		mVertexBuffer = vertexBuffer;
		mIndexBuffer = nullptr;
		mVertexBufferOffset = vertexBufferOffset;
		mPipeline = pipeline;
		mBoundingBox = aabb;
		mUnindexedAttribs = attribs;
		pipeline->AddRef();
	}

	GeometryResource::GeometryResource(ResourceManager* manager) :
		IResource(manager),
		mVertexBuffer(nullptr),
		mIndexBuffer(nullptr),
		mPipeline(nullptr) {
	}

	GeometryResource::~GeometryResource() {
		mPipeline->Release();
		if (mVertexBuffer)
			mVertexBuffer->Release();
		if (mIndexBuffer)
			mIndexBuffer->Release();
	}

	GeometryResource* GeometryResource::ToGeometry() {
		return this;
	}

	uint GetSize(DG::VALUE_TYPE v) {
		switch (v) {
			case DG::VT_FLOAT32:
				return 4;
			case DG::VT_FLOAT16:
				return 2;
			case DG::VT_INT8:
				return 1;
			case DG::VT_INT16:
				return 2;
			case DG::VT_INT32:
				return 4;
			case DG::VT_UINT8:
				return 1;
			case DG::VT_UINT16:
				return 2;
			case DG::VT_UINT32:
				return 4;
			default:
				throw std::runtime_error("Unexpected value type!");
		}
	}

	void GeometryLoader::Load(const aiScene* scene,
		PipelineResource* pipeline,
		RawGeometry* geometryOut) {

		VertexAttributeLayout attributes = pipeline->GetAttributeLayout();
		std::vector<DG::LayoutElement> layout = pipeline->GetVertexLayout();
		std::vector<uint> offsets;
		offsets.emplace_back(0);

		for (auto& layoutItem : layout) {
			uint size = GetSize(layoutItem.ValueType) * layoutItem.NumComponents;
			if (layoutItem.BufferSlot == 0) 
				offsets.emplace_back(offsets[offsets.size() - 1] + size);
			else 
				offsets.emplace_back(offsets[offsets.size() - 1]);
		}

		uint stride = offsets[offsets.size() - 1];

		// Override stride if specified.
		if (attributes.mStride > 0) {
			stride = attributes.mStride;
		}

		int positionOffset = -1;
		int uvOffset = -1;
		int normalOffset = -1;
		int tangentOffset = -1;
		int bitangentOffset = -1;

		auto verifyAttrib = [](DG::LayoutElement& element) {
			if (element.BufferSlot != 0) {
				throw std::runtime_error("Buffer slot must be 0!");
			}

			if (element.ValueType != DG::VT_FLOAT32) {
				throw std::runtime_error("Attribute type must be VT_FLOAT32!");
			}
		};

		if (attributes.mPosition >= 0) {
			auto& posAttrib = layout[attributes.mPosition];
			verifyAttrib(posAttrib);
			positionOffset = offsets[attributes.mPosition];
		}

		if (attributes.mUV >= 0) {
			auto& uvAttrib = layout[attributes.mUV];
			verifyAttrib(uvAttrib);
			uvOffset = offsets[attributes.mUV];
		}

		if (attributes.mNormal >= 0) {
			auto& normalAttrib = layout[attributes.mNormal];
			verifyAttrib(normalAttrib);
			normalOffset = offsets[attributes.mNormal];
		}

		if (attributes.mTangent >= 0) {
			auto& tangentAttrib = layout[attributes.mTangent];
			verifyAttrib(tangentAttrib);
			tangentOffset = offsets[attributes.mTangent];
		}

		if (attributes.mBitangent >= 0) {
			auto& bitangentAttrib = layout[attributes.mBitangent];
			verifyAttrib(bitangentAttrib);
			bitangentOffset = offsets[attributes.mBitangent];	
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
		indexBufferDesc.Usage = DG::USAGE_IMMUTABLE;
		indexBufferDesc.BindFlags = DG::BIND_INDEX_BUFFER;
		indexBufferDesc.uiSizeInBytes = indx_buffer_raw.size();

		DG::DrawIndexedAttribs indexedAttribs;
		indexedAttribs.IndexType = DG::VT_UINT32;
		indexedAttribs.NumIndices = indx_buffer_raw.size();
		
		// Write to output raw geometry
		geometryOut->Set(pipeline, vertexBufferDesc, indexBufferDesc, 
			std::move(vert_buffer), std::move(indx_buffer_raw),
			indexedAttribs, aabb);
	}

	void GeometryLoader::Load(DG::IRenderDevice* device, const std::string& source, 
		PipelineResource* pipeline, GeometryResource* resource) {
		
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

		RawGeometry rawGeo;
		Load(pScene, pipeline, &rawGeo);
		rawGeo.SpawnOnGPU(device, resource);
	}

	TaskId GeometryLoader::LoadAsync(DG::IRenderDevice* device, 
		ThreadPool* pool,
		const std::string& source,
		const TaskBarrierCallback& callback,
		PipelineResource* pipeline, 
		GeometryResource* loadinto) {

		auto barrier = loadinto->GetLoadBarrier();

		auto queue = pool->GetQueue();

		PipeId filePipe;
		TaskId readFile = ReadFileToMemoryJobDeferred(source, &queue, &filePipe);
		PipeId rawGeoPipe = queue.MakePipe();

		TaskId convertToRaw = queue.MakeTask([filePipe, source, pipeline, rawGeoPipe](const TaskParams& params) {
			try {
				auto& value = params.mPool->ReadPipe(filePipe);
				std::unique_ptr<ReadFileToMemoryResult> contents(value.cast<ReadFileToMemoryResult*>());

				std::unique_ptr<Assimp::Importer> importer(new Assimp::Importer());

				const aiScene* pScene = importer->ReadFileFromMemory(contents->GetData(), contents->GetSize(), 
					aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices |
					aiProcess_GenUVCoords | aiProcess_CalcTangentSpace | aiProcessPreset_TargetRealtime_Quality,
					source.c_str());

				if (!pScene) {
					std::cout << importer->GetErrorString() << std::endl;
					throw std::runtime_error("Failed to load geometry!");
				}

				if (!pScene->HasMeshes()) {
					throw std::runtime_error("Geometry has no meshes!");
				}

				std::unique_ptr<RawGeometry> rawGeo(new RawGeometry());
				Load(pScene, pipeline, rawGeo.get());

				params.mPool->WritePipe(rawGeoPipe, rawGeo.release());
			}
			catch (...) {
				params.mPool->WritePipeException(rawGeoPipe, std::current_exception());
			}
		});

		barrier->SetCallback(callback);

		TaskId rawToGpu = queue.MakeTask([device, rawGeoPipe, loadinto](const TaskParams& params) {
			auto& value = params.mPool->ReadPipe(rawGeoPipe);
			std::unique_ptr<RawGeometry> rawGeo(value.cast<RawGeometry*>());
			rawGeo->SpawnOnGPU(device, loadinto);
		}, barrier, ASSIGN_THREAD_MAIN);

		// Schedule everything linearly, make sure to schedule after pipeline has been loaded!
		queue.Dependencies(readFile).After(pipeline->GetLoadBarrier());
		queue.Dependencies(convertToRaw).After(filePipe);
		queue.Dependencies(rawGeoPipe).After(convertToRaw);
		queue.Dependencies(rawToGpu).After(rawGeoPipe);

		return readFile;
	}

	void ResourceCache<GeometryResource>::Clear() {
		for (auto& item : mResourceMap) {
			item.second->ResetRefCount();
			delete item.second;
		}

		mResourceMap.clear();
	}

	ResourceCache<GeometryResource>::ResourceCache(ResourceManager* manager) : 
		mManager(manager) {
	}

	ResourceCache<GeometryResource>::~ResourceCache() {
		Clear();
	}

	IResource* ResourceCache<GeometryResource>::Load(const void* params) {

		auto params_cast = reinterpret_cast<const LoadParams<GeometryResource>*>(params);
		
		decltype(mResourceMap.find(params_cast->mSource)) it;

		{
			std::shared_lock<std::shared_mutex> lock(mMutex);
			it = mResourceMap.find(params_cast->mSource);
			if (it != mResourceMap.end()) 
				return it->second;
		}

		PipelineResource* pipeline = nullptr;
		if (params_cast->mPipelineResource) {
			pipeline = params_cast->mPipelineResource;
		} else {
			pipeline = mManager->Load<PipelineResource>(params_cast->mPipelineSource);
		}

		auto resource = new GeometryResource(mManager);
		mLoader.Load(mManager->GetParent()->GetDevice(), 
			params_cast->mSource, pipeline, resource);

		if (!params_cast->mPipelineResource) {
			// If we loaded the pipeline, we should release it
			pipeline->Release();
		}

		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			resource->mIterator = mResourceMap.emplace(params_cast->mSource, resource).first;
		}
	
		return resource;
	}

	TaskId ResourceCache<GeometryResource>::AsyncLoadDeferred(const void* params,
		ThreadPool* threadPool,
		IResource** output,
		const TaskBarrierCallback& callback) {
				auto params_cast = reinterpret_cast<const LoadParams<GeometryResource>*>(params);
		
		decltype(mResourceMap.find(params_cast->mSource)) it;

		{
			std::shared_lock<std::shared_mutex> lock(mMutex);
			it = mResourceMap.find(params_cast->mSource);
			if (it != mResourceMap.end()) {
				*output = it->second;
				return TASK_NONE;
			}
		}

		bool bLoadedPipeline = false;

		PipelineResource* pipeline = nullptr;
		TaskId loadPipelineTask = TASK_NONE;
		if (params_cast->mPipelineResource) {
			pipeline = params_cast->mPipelineResource;
		} else {
			loadPipelineTask = mManager->AsyncLoadDeferred<PipelineResource>(
				params_cast->mPipelineSource, &pipeline);
			bLoadedPipeline = true;
		}

		TaskBarrierCallback geometryCallback;
		if (bLoadedPipeline) {
			geometryCallback = [pipeline, callback](ThreadPool* pool) {
				// Make sure to release the pipeline after the geometry has been loaded!
				// Geometry now has reference to pipeline, so pipeline won't be unloaded
				pipeline->Release();
				callback(pool);
			};
 		} else {
			geometryCallback = callback;
		}

		auto resource = new GeometryResource(mManager);
		TaskId loadGeoTask = mLoader.LoadAsync(mManager->GetParent()->GetDevice(),
			threadPool, params_cast->mSource, geometryCallback, pipeline, resource);

		if (!params_cast->mPipelineResource) {
			// If we loaded the pipeline, we should release it
			pipeline->Release();
		}

		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			resource->mIterator = mResourceMap.emplace(params_cast->mSource, resource).first;
		}

		*output = resource;

		if (bLoadedPipeline) {
			// Pipeline has already been or is in the process of being loaded,
			// we should schedule the geometry load.
			return loadGeoTask;
		} else {
			// Pipeline has not yet been loaded, we should schedule the pipeline load.
			return loadPipelineTask;
		}
	}

	void ResourceCache<GeometryResource>::Add(IResource* resource, const void* params) {
		std::lock_guard<std::shared_mutex> lock(mMutex);

		auto params_cast = reinterpret_cast<const LoadParams<GeometryResource>*>(params);
		auto it = mResourceMap.find(params_cast->mSource);
		auto geometryResource = resource->ToGeometry();

		if (it != mResourceMap.end()) {
			if (it->second != geometryResource)
				Unload(it->second);
			else
				return;
		} 

		geometryResource->mIterator = 
			mResourceMap.emplace(params_cast->mSource, 
			geometryResource).first;
	}

	void ResourceCache<GeometryResource>::Unload(IResource* resource) {
		std::lock_guard<std::shared_mutex> lock(mMutex);
		auto geo = resource->ToGeometry();

		if (geo->mIterator != mResourceMap.end()) {
			mResourceMap.erase(geo->mIterator);
		}

		delete resource;
	}
}