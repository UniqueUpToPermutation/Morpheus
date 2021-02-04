#include <Engine/Resources/GeometryResource.hpp>
#include <Engine/Resources/PipelineResource.hpp>

#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

using namespace Assimp;
using namespace std;

namespace Morpheus {

	void GeometryResource::Init(DG::IBuffer* vertexBuffer, DG::IBuffer* indexBuffer,
		uint vertexBufferOffset, PipelineResource* pipeline, 
		const BoundingBox& aabb) {
		mVertexBuffer = vertexBuffer;
		mIndexBuffer = indexBuffer;
		mVertexBufferOffset = vertexBufferOffset;
		mPipeline = pipeline;
		mBoundingBox = aabb;
		pipeline->AddRef();
	}

	GeometryResource::GeometryResource(ResourceManager* manager, 
		DG::IBuffer* vertexBuffer, DG::IBuffer* indexBuffer,
		uint vertexBufferOffset, PipelineResource* pipeline, 
		const BoundingBox& aabb) : 
		IResource(manager) {
		Init(vertexBuffer, indexBuffer, vertexBufferOffset, pipeline, aabb);
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

	GeometryLoader::GeometryLoader(ResourceManager* manager) :
		mManager(manager) {
	}

	GeometryLoader::~GeometryLoader() {
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

	void GeometryLoader::Load(const std::string& source, PipelineResource* pipeline, GeometryResource* resource) {
		cout << "Loading geometry " << source << "..." << endl;

		std::unique_ptr<Assimp::Importer> importer(new Assimp::Importer());

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

		const aiScene* pScene = importer->ReadFile(source.c_str(),
			aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices |
			aiProcess_GenUVCoords | aiProcess_CalcTangentSpace | aiProcessPreset_TargetRealtime_Quality);

		if (!pScene) {
			cout << "Error: failed to load " << source << endl;
		}

		uint nVerts;
		uint nIndices;

		if (!pScene->HasMeshes()) {
			cout << "Error: " << source << " has no meshes!" << endl;
		}

		if (pScene->mNumMeshes > 1) {
			cout << "Warning: " << source << " has more than one mesh, we will just load the first." << endl;
		}

		aiMesh* mesh = pScene->mMeshes[0];

		nVerts = mesh->mNumVertices;
		nIndices = mesh->mNumFaces * 3;

		std::vector<uint8_t> vert_buffer(nVerts * stride);
		std::vector<DG::Uint32> indx_buffer(nIndices);

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
			if (bHasTangents) {
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
		auto device = mManager->GetParent()->GetDevice();

		std::string name("Geometry Vertex Buffer : ");
		name += source;
		
		DG::BufferDesc vertexBufferDesc;
		vertexBufferDesc.Name          = name.c_str();
		vertexBufferDesc.Usage         = DG::USAGE_IMMUTABLE;
		vertexBufferDesc.BindFlags     = DG::BIND_VERTEX_BUFFER;
		vertexBufferDesc.uiSizeInBytes = vert_buffer.size();

		DG::BufferData VBData;
		VBData.pData    = vert_buffer.data();
		VBData.DataSize = vert_buffer.size();

		DG::IBuffer* vertexBuffer = nullptr;
		device->CreateBuffer(vertexBufferDesc, &VBData, &vertexBuffer);

		name = "Geometry Index Buffer : ";
		name += source;

		DG::BufferDesc indexBufferDesc;
		indexBufferDesc.Name = name.c_str();
		indexBufferDesc.Usage = DG::USAGE_IMMUTABLE;
		indexBufferDesc.BindFlags = DG::BIND_INDEX_BUFFER;
		indexBufferDesc.uiSizeInBytes = indx_buffer.size() * 
			sizeof(decltype(indx_buffer[0]));

		DG::BufferData IBData;
		IBData.DataSize = indx_buffer.size() * 
			sizeof(decltype(indx_buffer[0]));
		IBData.pData = indx_buffer.data();
		
		DG::IBuffer* indexBuffer = nullptr;
		device->CreateBuffer(indexBufferDesc, &IBData, &indexBuffer);

		resource->Init(vertexBuffer, indexBuffer, 0, pipeline, aabb);
		resource->mSource = source;
		resource->mIndexedAttribs.IndexType = DG::VT_UINT32;
		resource->mIndexedAttribs.NumIndices = indx_buffer.size();
	}

	void ResourceCache<GeometryResource>::Clear() {
		for (auto& item : mResourceMap) {
			item.second->ResetRefCount();
			delete item.second;
		}

		mResourceMap.clear();
	}

	ResourceCache<GeometryResource>::ResourceCache(ResourceManager* manager) : 
		mManager(manager), 
		mLoader(manager) {
	}

	ResourceCache<GeometryResource>::~ResourceCache() {
		Clear();
	}

	IResource* ResourceCache<GeometryResource>::Load(const void* params) {

		auto params_cast = reinterpret_cast<const LoadParams<GeometryResource>*>(params);
		
		auto it = mResourceMap.find(params_cast->mSource);
		if (it != mResourceMap.end()) 
			return it->second;

		PipelineResource* pipeline = nullptr;
		if (params_cast->mPipelineResource) {
			pipeline = params_cast->mPipelineResource;
		} else {
			pipeline = mManager->Load<PipelineResource>(params_cast->mPipelineSource);
		}

		auto resource = new GeometryResource(mManager);
		mLoader.Load(params_cast->mSource, pipeline, resource);

		if (!params_cast->mPipelineResource) {
			// If we loaded the pipeline, we should release it
			pipeline->Release();
		}

		mResourceMap[params_cast->mSource] = resource;
		
		return resource;
	}

	TaskId ResourceCache<GeometryResource>::AsyncLoadDeferred(const void* params,
	ThreadPool* threadPool,
		IResource** output,
		const TaskBarrierCallback& callback) {
		throw std::runtime_error("Not implemented!");
	}

	void ResourceCache<GeometryResource>::Add(IResource* resource, const void* params) {
		std::lock_guard<std::mutex> lock(mMutex);
		auto params_cast = reinterpret_cast<const LoadParams<GeometryResource>*>(params);
		
		auto it = mResourceMap.find(params_cast->mSource);

		auto geometryResource = resource->ToGeometry();

		if (it != mResourceMap.end()) {
			if (it->second != geometryResource)
				Unload(it->second);
			else
				return;
		} 

		mResourceMap[params_cast->mSource] = geometryResource;
	}

	void ResourceCache<GeometryResource>::Unload(IResource* resource) {
		auto geo = resource->ToGeometry();

		auto it = mResourceMap.find(geo->GetSource());
		if (it != mResourceMap.end()) {
			if (it->second == geo) {
				mResourceMap.erase(it);
			}
		}

		delete resource;
	}
}