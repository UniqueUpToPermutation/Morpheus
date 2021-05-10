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

		struct Data {
			std::unique_ptr<RawGeometry> mRawGeo;
		};

		Task task([device, params, loadinto, data = Data()](const TaskParams& e) mutable {
			if (e.mTask->SubTask()) {
				data.mRawGeo = std::make_unique<RawGeometry>();
				Task task = data.mRawGeo->LoadTask(params);

				e.mTask->In().Lock().Connect(&task);
				e.mQueue->AdoptAndTrigger(std::move(task));
				return TaskResult::WAITING;
			}

			if (e.mTask->SubTask())
				if (e.mTask->RequestThreadSwitch(e, ASSIGN_THREAD_MAIN))
					return TaskResult::REQUEST_THREAD_SWITCH;

			if (e.mTask->SubTask()) {
				data.mRawGeo->SpawnOnGPU(device, loadinto);
				loadinto->SetLoaded(true);
			}

			return TaskResult::FINISHED;
		}, 
		std::string("Load Geometry ") + params.mSource,
		TaskType::FILE_IO);

		loadinto->GetLoadBarrier()->mIn.Lock().Connect(&task->Out());
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