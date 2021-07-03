#include <Engine/Resources/Geometry.hpp>

namespace Morpheus {

	template <typename T>
	ResourceTask<T> LoadTemplated(DG::IRenderDevice* device, 
		const LoadParams<Geometry>& params) {
		Promise<T> promise;
		Future<T> future(promise);

		struct Data {
			RawGeometry mRaw;
		};

		Task task([params, device, promise = std::move(promise), data = Data()](const TaskParams& e) mutable {
			if (e.mTask->BeginSubTask()) {
				data.mRaw.Load(params);
				e.mTask->EndSubTask();
			}

			e.mTask->RequestThreadSwitch(e, ASSIGN_THREAD_MAIN);

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

	Handle<Geometry> Geometry::Prefabs::MaterialBall(DG::IRenderDevice* device, const VertexLayout& layout) {
		Handle<Geometry> handle;
		handle.Adopt(new Geometry(device, RawGeometry::Prefabs::MaterialBall(layout)));
		return handle;
	}
	Handle<Geometry> Geometry::Prefabs::Box(DG::IRenderDevice* device, const VertexLayout& layout) {
		Handle<Geometry> handle;
		handle.Adopt(new Geometry(device, RawGeometry::Prefabs::Box(layout)));
		return handle;
	}
	Handle<Geometry> Geometry::Prefabs::Sphere(DG::IRenderDevice* device, const VertexLayout& layout) {
		Handle<Geometry> handle;
		handle.Adopt(new Geometry(device, RawGeometry::Prefabs::Sphere(layout)));
		return handle;
	}
	Handle<Geometry> Geometry::Prefabs::BlenderMonkey(DG::IRenderDevice* device, const VertexLayout& layout) {
		Handle<Geometry> handle;
		handle.Adopt(new Geometry(device, RawGeometry::Prefabs::BlenderMonkey(layout)));
		return handle;
	}
	Handle<Geometry> Geometry::Prefabs::Torus(DG::IRenderDevice* device, const VertexLayout& layout) {
		Handle<Geometry> handle;
		handle.Adopt(new Geometry(device, RawGeometry::Prefabs::Torus(layout)));
		return handle;
	}
	Handle<Geometry> Geometry::Prefabs::Plane(DG::IRenderDevice* device, const VertexLayout& layout) {
		Handle<Geometry> handle;
		handle.Adopt(new Geometry(device, RawGeometry::Prefabs::Plane(layout)));
		return handle;
	}
	Handle<Geometry> Geometry::Prefabs::StanfordBunny(DG::IRenderDevice* device, const VertexLayout& layout) {
		Handle<Geometry> handle;
		handle.Adopt(new Geometry(device, RawGeometry::Prefabs::StanfordBunny(layout)));
		return handle;
	}
	Handle<Geometry> Geometry::Prefabs::UtahTeapot(DG::IRenderDevice* device, const VertexLayout& layout) {
		Handle<Geometry> handle;
		handle.Adopt(new Geometry(device, RawGeometry::Prefabs::UtahTeapot(layout)));
		return handle;
	}

	void Geometry::Set(DG::IBuffer* vertexBuffer, 
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

	void Geometry::Set(DG::IBuffer* vertexBuffer,
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

	ResourceTask<Handle<Geometry>> Geometry::LoadHandle(
		DG::IRenderDevice* device, const LoadParams<Geometry>& params) {
		return LoadTemplated<Handle<Geometry>>(device, params);
	}

	ResourceTask<Geometry*> Geometry::Load(DG::IRenderDevice* device, 
		const LoadParams<Geometry>& params) {
		return LoadTemplated<Geometry*>(device, params);
	}
}