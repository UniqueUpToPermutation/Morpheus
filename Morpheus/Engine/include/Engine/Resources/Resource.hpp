#pragma once

#include <Engine/Entity.hpp>
#include <Engine/ThreadPool.hpp>

namespace Morpheus {
	typedef uint32_t ResourceFlags;

	enum ResourceFlag : ResourceFlags {
		RESOURCE_LOADED_FROM_DISK = 1u << 0,
		RESOURCE_MANAGED = 1u << 1,
		RESOURCE_RASTERIZER_ASPECT = 1u << 2,
		RESOURCE_RAW_ASPECT = 1u << 3,
		RESOURCE_RAYTRACER_ASPECT = 1u << 4,
		RESOURCE_GPU_RESIDENT = 1u << 5,
		RESOURCE_CPU_RESIDENT = 1u << 6
	};

	void ReadBinaryFile(const std::string& source, std::vector<uint8_t>& out);

	template <typename T>
	struct LoadParams {
	};

	class IResource {
	private:
		std::atomic<uint> mRefCount;

	protected:
		ResourceFlags mFlags = 0u;

	public:
		inline ResourceFlags GetFlags() const {
			return mFlags;
		}

		inline bool IsManaged() const {
			return mFlags & RESOURCE_MANAGED;
		}

		inline bool IsFromDisk() const {
			return mFlags & RESOURCE_LOADED_FROM_DISK;
		}

		inline bool IsRaw() const {
			return mFlags & RESOURCE_RAW_ASPECT;
		}

		inline bool IsRasterResource() const {
			return mFlags & RESOURCE_RASTERIZER_ASPECT;
		}

		inline bool IsRaytraceResource() const {
			return mFlags & RESOURCE_RAYTRACER_ASPECT;
		}

		inline bool IsGpu() const {
			return mFlags & RESOURCE_GPU_RESIDENT;
		}

		inline bool IsCpu() const {
			return mFlags & RESOURCE_CPU_RESIDENT;
		}

		inline IResource() {
			mRefCount = 1;
		}

		inline uint AddRef() {
			return mRefCount.fetch_add(1) + 1;
		}

		inline uint GetRefCount() {
			return mRefCount;
		}

		virtual ~IResource() = default;

		inline uint Release() {
			auto val = mRefCount.fetch_sub(1);
			assert(val >= 1);

			if (val == 1)
				delete this;

			return val - 1;
		}
	};

	template <typename T>
	class Handle {
	private:
		T* mResource;

	public:
		inline Handle(T&& resource) : mResource(new T(std::move(resource))) {
		}

		inline Handle() : mResource(nullptr) {
		}

		inline Handle(T* resource) : mResource(resource) {
			if (resource)
				resource->AddRef();
		}

		inline Handle(const Handle<T>& h) : mResource(h.mResource) {
			if (mResource)
				mResource->AddRef();
		}

		inline Handle(Handle<T>&& h) : mResource(h.mResource) {
			h.mResource = nullptr;
		}

		inline void Adopt(T* resource) {
			if (mResource)
				mResource->Release();

			mResource = resource;
		}

		inline Handle<T>& operator=(const Handle<T>& h) {
			if (mResource)
				mResource->Release();

			mResource = h.mResource;

			if (mResource)
				mResource->AddRef();

			return *this;
		}

		inline Handle<T>& operator=(Handle<T>&& h) {
			if (mResource)
				mResource->Release();

			mResource = h.mResource;
			h.mResource = nullptr;
			return *this;
		}

		inline ~Handle() {
			if (mResource)
				mResource->Release();
		}

		inline T* Ptr() const {
			return mResource;
		}

		inline T* operator->() const {
			return mResource;
		}

		inline T** Ref() {
			return &mResource;
		}

		inline operator bool() const {
			return mResource;
		}

		inline operator T*() const {
			return mResource;
		}

		inline bool operator==(const Handle<T>& other) const {
			return other.mResource == mResource;
		}

		inline T* Release() {
			T* result = mResource;
			if (mResource) {
				result->Release();
				mResource = nullptr;
			}
			return result;
		}
	};
}