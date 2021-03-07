#include <Engine/Resources/GeometryResource.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/ResourceData.hpp>

#include <Engine/ThreadTasks.hpp>

#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

using namespace Assimp;
using namespace std;

namespace Morpheus {
	void GeometryResource::InitIndexed(DG::IBuffer* vertexBuffer, 
		DG::IBuffer* indexBuffer,
		uint vertexBufferOffset, 
		const DG::DrawIndexedAttribs& attribs,
		const VertexLayout& layout,
		const BoundingBox& aabb) {
		mVertexBuffer = vertexBuffer;
		mIndexBuffer = indexBuffer;
		mVertexBufferOffset = vertexBufferOffset;
		mLayout = layout,
		mBoundingBox = aabb;
		mIndexedAttribs = attribs;
	}

	void GeometryResource::Init(DG::IBuffer* vertexBuffer,
		uint vertexBufferOffset,
		const DG::DrawAttribs& attribs,
		const VertexLayout& layout,
		const BoundingBox& aabb) {
		mVertexBuffer = vertexBuffer;
		mIndexBuffer = nullptr;
		mVertexBufferOffset = vertexBufferOffset;
		mLayout = layout;
		mBoundingBox = aabb;
		mUnindexedAttribs = attribs;
	}

	GeometryResource::GeometryResource(ResourceManager* manager) :
		IResource(manager),
		mVertexBuffer(nullptr),
		mIndexBuffer(nullptr) {
	}

	GeometryResource::~GeometryResource() {
		if (mVertexBuffer)
			mVertexBuffer->Release();
		if (mIndexBuffer)
			mIndexBuffer->Release();
	}

	GeometryResource* GeometryResource::ToGeometry() {
		return this;
	}

	void GeometryLoader::Load(DG::IRenderDevice* device, 
		const LoadParams<GeometryResource>& params, 
		GeometryResource* resource) {
		RawGeometry geo(params);
		geo.SpawnOnGPU(device, resource);
	}

	TaskId GeometryLoader::LoadAsync(DG::IRenderDevice* device, 
		ThreadPool* pool,
		const LoadParams<GeometryResource>& params,
		const TaskBarrierCallback& callback,
		GeometryResource* loadinto) {

		auto raw = std::make_shared<RawGeometry>();

		auto taskId = raw->LoadAsyncDeferred(params, pool);

		auto queue = pool->GetQueue();
		auto rawBarrier = raw->GetLoadBarrier();
		auto gpuSpawnBarrier = loadinto->GetLoadBarrier();

		gpuSpawnBarrier->SetCallback(callback);

		TaskId rawToGpu = queue.MakeTask([raw, device, loadinto](const TaskParams& params) {
			raw->SpawnOnGPU(device, loadinto);
			raw->Clear();
		}, gpuSpawnBarrier);

		queue.Dependencies(rawToGpu).After(rawBarrier);

		return taskId;
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

		auto resource = new GeometryResource(mManager);
		mLoader.Load(mManager->GetParent()->GetDevice(), *params_cast, resource);

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

		auto resource = new GeometryResource(mManager);
		TaskId loadGeoTask = mLoader.LoadAsync(mManager->GetParent()->GetDevice(),
			threadPool, *params_cast, callback, resource);

		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			resource->mIterator = mResourceMap.emplace(params_cast->mSource, resource).first;
		}

		*output = resource;

		return loadGeoTask;
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