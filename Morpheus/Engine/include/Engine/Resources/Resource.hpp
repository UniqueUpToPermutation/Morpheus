#pragma once

#include <Engine/Entity.hpp>
#include <Engine/ThreadPool.hpp>

namespace Morpheus {
	class RawTexture;
	class Texture;
	class RawGeometry;
	class Geometry;

	struct MaterialDesc;
	class Material;

	enum class ResourceManagement {
		FROM_DISK_MANAGED,
		FROM_DISK_UNMANAGED,
		INTERNAL_MANAGED,
		INTERNAL_UNMANAGED
	};
	
	template <typename T>
	struct LoadParams {
	};

	void ReadBinaryFile(const std::string& source, std::vector<uint8_t>& out);

	class IResource {
	private:
		std::atomic<uint> mRefCount;

	public:
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

		inline T* Ptr() {
			return mResource;
		}

		inline T* operator->() {
			return mResource;
		}

		inline operator bool() {
			return mResource;
		}

		inline operator T*() {
			return mResource;
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