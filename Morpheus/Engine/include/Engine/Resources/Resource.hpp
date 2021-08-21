#pragma once

#include <Engine/Entity.hpp>
#include <Engine/ThreadPool.hpp>
#include <Engine/Defines.hpp>
#include <Engine/Graphics.hpp>

#include <filesystem>

#include "RenderDevice.h"
#include "DeviceContext.h"

namespace Morpheus {
	void ReadBinaryFile(const std::string& source, std::vector<uint8_t>& out);

	template <typename T>
	struct LoadParams {
	};

	typedef int FrameId;

	constexpr FrameId InvalidFrameId = -1;

	class IResource {
	private:
		std::atomic<uint> mRefCount;

	protected:
		FrameId mFrameId;
		Device mDevice;

	public:
		inline FrameId GetFrameId() const {
			return mFrameId;
		}

		inline void SetFrameId(FrameId value) {
			mFrameId = value;
		}

		inline Device GetDevice() const {
			return mDevice;
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

		virtual entt::meta_type GetType() const = 0;
		virtual entt::meta_any GetSourceMeta() const = 0;
		virtual void BinarySerialize(std::ostream& output) const = 0;
		virtual void BinaryDeserialize(std::istream& input) = 0;
		virtual void BinarySerializeSource(
			const std::filesystem::path& workingPath, 
			cereal::PortableBinaryOutputArchive& output) const = 0;
		virtual void BinaryDeserializeSource(
			const std::filesystem::path& workingPath,
			cereal::PortableBinaryInputArchive& input) = 0;
		virtual BarrierOut MoveAsync(Device device, Context context = Context()) = 0;
		
		void Move(Device device, Context context = Context());

		static void RegisterMetaData();

		friend class Frame;
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

		struct Hasher {
			inline std::size_t operator()(const Handle<T>& h) const
			{
				using std::hash;
				return hash<T*>()(h.mResource);
			}
		};
	};

	template <typename T>
	TaskNode PipeToHandle(UniqueFuture<T> in, Promise<Handle<T>> out) {
		return FunctionPrototype<UniqueFuture<T>, Promise<Handle<T>>>([](
			const TaskParams& e, UniqueFuture<T> in, Promise<Handle<T>> out) {
			out.Set(Handle<T>(std::move(in.Get())));
		})(in, out);
	}

	template <typename T>
	TaskNode PipeFromHandle(UniqueFuture<Handle<T>> in, Promise<T> out) {
		return FunctionPrototype<UniqueFuture<Handle<T>>, Promise<T>>([](
			const TaskParams& e, UniqueFuture<Handle<T>> in, Promise<T> out) {
			out.Set(std::move(*in.Get()));
		})(in, out);
	}

	struct SerializationSet {
		std::vector<entt::entity> mToSerialize;
		std::vector<entt::entity> mSubFrames;
	};

	class IAbstractResourceCache {
	public:
		virtual entt::meta_type GetResourceType() const = 0;
	};

	template <typename T>
	class IResourceCache : public IAbstractResourceCache {
	public:
		virtual Future<Handle<T>> Load(const LoadParams<T>& params, IComputeQueue* queue) = 0;

		entt::meta_type GetResourceType() const override {
			return entt::resolve<T>();
		}
	};

	class IResourceCacheCollection {
	public:
		virtual bool TryQueryCache(
			const entt::meta_type& resourceType,
			entt::meta_any* cacheInterface) const = 0;

		virtual bool TryQueryCacheAbstract(
			const entt::meta_type& resourceType,
			IAbstractResourceCache** cacheOut) const = 0;

		virtual std::set<IAbstractResourceCache*> GetAllCaches() const = 0;

		std::set<IAbstractResourceCache*> GetSerializableCaches() const;
		
		template <typename T>
		IResourceCache<T>* QueryCache() const {
			entt::meta_any result;
			if (TryQueryCache(entt::resolve<T>(), &result))
				return result.cast<IResourceCache<T>*>();
			return nullptr;
		}

		IAbstractResourceCache* QueryCacheAbstract(const entt::meta_type& resourceType);
	};
}