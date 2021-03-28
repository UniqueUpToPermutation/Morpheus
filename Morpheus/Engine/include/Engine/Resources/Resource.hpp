#pragma once

#include <Engine/Entity.hpp>
#include <Engine/ThreadPool.hpp>

namespace Morpheus {
	class TextureResource;
	class PipelineResource;
	class MaterialResource;
	class GeometryResource;
	class ShaderResource;
	class ResourceManager;
	class CollisionShapeResource;
	class RawTexture;
	class RawGeometry;
	class RawShader;

	template <typename T>
	struct RefHandle {
	private:
		T* mPtr;

	public:
		inline T* RawPtr() {
			return mPtr;
		}

		inline const T* RawPtr() const {
			return mPtr;
		}

		inline RefHandle() noexcept : mPtr(nullptr) {
		}

		inline RefHandle(T* ptr) noexcept : mPtr(ptr) {
			if (ptr)
				ptr->AddRef();
		}

		inline ~RefHandle() {
			if (mPtr)
				mPtr->Release();
		}

		inline RefHandle(const RefHandle& other) noexcept : mPtr(other.mPtr) {
			if (mPtr)
				mPtr->AddRef();
		}

		inline RefHandle(RefHandle&& other) noexcept : mPtr(other.mPtr) {
			other.mPtr = nullptr;
		}

		inline RefHandle& operator=(const RefHandle& other) noexcept {
			mPtr = other.mPtr;
			if (mPtr)
				mPtr->AddRef();

			return *this;
		}

		inline RefHandle& operator=(RefHandle&& other) {
			if (mPtr) 
				mPtr->Release();

			mPtr = other.mPtr;
			other.mPtr = nullptr;

			return *this;
		}

		inline T* operator->() noexcept {
			return mPtr;
		}

		inline const T* operator->() const noexcept {
			return mPtr;
		}

		inline RefHandle& operator=(T* ptr) {
			if (mPtr) 
				mPtr->Release();

			mPtr = ptr;

			if (mPtr)
				mPtr->AddRef();

			return *this;
		}

		inline operator bool() const noexcept {
			return mPtr != nullptr;
		}

		inline operator T*() noexcept {
			return mPtr;
		}

		inline operator const T*() const noexcept {
			return mPtr;
		}
	};

	struct AsyncResourceParams {
		bool bUseAsync;
		TaskBarrierCallback mCallback;
		ThreadPool* mThreadPool;	
	};
	using resource_type = 
		entt::identifier<
			TextureResource,
			PipelineResource,
			MaterialResource,
			GeometryResource,
			CollisionShapeResource,
			ShaderResource>;

	class IResource;
	class IResourceCache {
	public:
		// Loads resource on the current thread with no async
		virtual IResource* Load(const void* params) = 0;
		// Returns a task desc that the caller must then trigger with the thread pool
		virtual TaskId AsyncLoadDeferred(const void* params,
			ThreadPool* threadPool,
			IResource** output,
			const TaskBarrierCallback& callback = nullptr) = 0;
		
		inline IResource* AsyncLoad(const void* params, 
			ThreadPool* threadPool,
			const TaskBarrierCallback& callback = nullptr) {

			// Create a task for the thread pool and a resource to load into
			IResource* result = nullptr;
			TaskId task = AsyncLoadDeferred(params, threadPool, &result, callback);

			// Emplace task into the thread pool queue
			if (task.IsValid()) {
				auto queue = threadPool->GetQueue();
				queue.Schedule(task);
			}

			return result;
		}
			
		virtual void Add(IResource* resource, const void* params) = 0;
		virtual void Unload(IResource* resource) = 0;
		virtual void Clear() = 0;
		virtual ~IResourceCache() {
		}
	};

	template <typename T>
	class ResourceCache;

	template <typename T>
	struct LoadParams;

	template <typename T>
	struct ResourceConvert;

	class IResource {
	private:
		std::atomic<unsigned int> mRefCount = 0;
		ResourceManager* mManager;

	protected:
		TaskBarrier mLoadBarrier;

	public:
		IResource(ResourceManager* manager) : 
			mManager(manager) {
		}

		virtual ~IResource() {
			assert(mRefCount == 0);
		}

		inline void AddRef() noexcept {
			++mRefCount;
		}

		void Release();
		
		inline void ResetRefCount() {
			mRefCount = 0;
		}

		inline int GetRefCount() const {
			return mRefCount;
		}

		virtual entt::id_type GetType() const = 0;

		virtual PipelineResource* ToPipeline();
		virtual GeometryResource* ToGeometry();
		virtual MaterialResource* ToMaterial();
		virtual TextureResource* ToTexture();
		virtual CollisionShapeResource* ToCollisionShape();
		virtual ShaderResource* ToShader();

		inline bool IsLoaded() const {
			return mLoadBarrier.ActiveTaskCount() == 0;
		}

		// A barrier that is invoked when the resource is loaded.
		inline TaskBarrier* GetLoadBarrier() {
			return &mLoadBarrier;
		}
		inline ResourceManager* GetManager() {
			return mManager;
		}

		template <typename T>
		inline T* To() {
			return ResourceConvert<T>::Convert(this);
		}
	};

	template <>
	struct ResourceConvert<PipelineResource> {
		static inline PipelineResource* Convert(IResource* resource) {
			return resource->ToPipeline();
		}
	};

	template <>
	struct ResourceConvert<GeometryResource> {
		static inline GeometryResource* Convert(IResource* resource) {
			return resource->ToGeometry();
		}
	};

	template <>
	struct ResourceConvert<MaterialResource> {
		static inline MaterialResource* Convert(IResource* resource) {
			return resource->ToMaterial();
		}
	};

	template <>
	struct ResourceConvert<TextureResource> {
		static inline TextureResource* Convert(IResource* resource) {
			return resource->ToTexture();
		}
	};

	template <>
	struct ResourceConvert<CollisionShapeResource> {
		static inline CollisionShapeResource* Convert(IResource* resource) {
			return resource->ToCollisionShape();
		}
	};

	
	template <>
	struct ResourceConvert<ShaderResource> {
		static inline ShaderResource* Convert(IResource* resource) {
			return resource->ToShader();
		}
	};
}