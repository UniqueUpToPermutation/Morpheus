#include <Engine/Resources/GeometryResource.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/ResourceData.hpp>

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

	Task GeometryLoader::LoadTask(DG::IRenderDevice* device, 
		const LoadParams<GeometryResource>& params,
		GeometryResource* loadinto) {

		Task task;
		task.mType = TaskType::FILE_IO;
		task.mSyncPoint = loadinto->GetLoadBarrier();
		task.mFunc = [device, params, loadinto](const TaskParams& e) {
			auto raw = std::make_unique<RawGeometry>();
			raw->LoadTask(params)(e);

			e.mQueue->Submit([device, loadinto, raw = std::move(raw)](const TaskParams& params) mutable {
				raw->SpawnOnGPU(device, loadinto);
			}, TaskType::RENDER, loadinto->GetLoadBarrier(), ASSIGN_THREAD_MAIN);
		};

		return task;
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

	Task ResourceCache<GeometryResource>::LoadTask(const void* params,
		IResource** output) {
		auto params_cast = reinterpret_cast<const LoadParams<GeometryResource>*>(params);
		
		decltype(mResourceMap.find(params_cast->mSource)) it;

		{
			std::shared_lock<std::shared_mutex> lock(mMutex);
			it = mResourceMap.find(params_cast->mSource);
			if (it != mResourceMap.end()) {
				*output = it->second;
				return Task();
			}
		}

		auto resource = new GeometryResource(mManager);
		Task loadGeoTask = mLoader.LoadTask(mManager->GetParent()->GetDevice(), *params_cast, resource);

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