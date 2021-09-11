#pragma once

#include <Engine/Entity.hpp>
#include <Engine/ThreadPool.hpp>
#include <Engine/Defines.hpp>
#include <Engine/Graphics.hpp>

#include <filesystem>

#include "RenderDevice.h"
#include "DeviceContext.h"

namespace std {
	template <typename Archive>
	void save(Archive& arr, const std::filesystem::path& path) {
		std::string str = path;
		arr(str);
	}

	template <typename Archive>
	void load(Archive& arr, std::filesystem::path& path) {
		std::string str;
		arr(str);
		path = str;
	}
}

namespace Morpheus {
	void ReadBinaryFile(const std::string& source, std::vector<uint8_t>& out);

	struct ArchiveBlobPointer {
		std::streamsize mBegin;
		std::streamsize mSize;
	};

	enum ArchiveLoadType {
		NONE,
		DIRECT,
		USE_FRAME_TABLE
	};

	template <typename T>
	struct LoadParams {
	};

	typedef int FrameId;

	constexpr FrameId InvalidFrameId = -1;

	class IFrameAbstract;

	template <typename T>
	class Handle {
	private:
		T* mResource;

	public:
		inline Handle(T&& resource) : 
			mResource(new T(std::move(resource))) {
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

			mResource = h;
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

		template <typename T2>
		inline Handle<T2> TryCast() const {
			return Handle<T2>(dynamic_cast<T2*>(mResource));
		}

		template <typename T2>
		inline Handle<T2> DownCast() const {
			return Handle<T2>(mResource);
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
	struct ArchiveLoad {
		ArchiveLoadType mType = ArchiveLoadType::NONE;

		// Used if ArchiveLoadType is DIRECT
		ArchiveBlobPointer mPosition;

		// Use if ArchiveLoadType is USE_FRAME_TABLE
		Handle<IFrameAbstract> mFrame;
		entt::entity mEntity = entt::null;
	};

	struct UniversalIdentifier {
		std::filesystem::path mPath;
		entt::entity mEntity = entt::null;

		struct Hasher {
			std::size_t operator() (const UniversalIdentifier &pair) const {
				return std::filesystem::hash_value(pair.mPath) ^ 
					std::hash<entt::entity>()(pair.mEntity);
			}
		};

		template <typename Archive>
		void save(Archive& arr) const {
			std::save<Archive>(arr, mPath);
			arr(mEntity);
		}

		template <typename Archive>
		void load(Archive& arr) {
			std::load<Archive>(arr, mPath);
			arr(mEntity);
		}

		inline bool operator==(const UniversalIdentifier& other) const {
			bool pathEq = mPath.compare(other.mPath) == 0;
			return pathEq && mEntity == other.mEntity;
		}
	};

	struct RefCounter {
		std::atomic<uint> mCount;

		inline RefCounter() {
			mCount = 1;
		}
		inline RefCounter(const RefCounter&) {
		}
		inline RefCounter& operator=(const RefCounter&) {
			return *this;
		}
		inline RefCounter(RefCounter&&) {
		}
		inline RefCounter& operator=(RefCounter&&) {
			return *this;
		}
	};

	class IResource {
	private:
		RefCounter mRefCount;

	protected:
		entt::entity mEntity = entt::null;
		IFrameAbstract* mFrame;
		Device mDevice;

	public:
		// Used for resource caching
		UniversalIdentifier GetUniversalId() const;

		inline IFrameAbstract* GetFrame() const;

		inline entt::entity GetEntity() const {
			return mEntity;
		}

		inline Device GetDevice() const {
			return mDevice;
		}

		inline uint AddRef() {
			return mRefCount.mCount.fetch_add(1) + 1;
		}

		inline uint GetRefCount() {
			return mRefCount.mCount;
		}

		virtual ~IResource() = default;

		inline uint Release() {
			auto val = mRefCount.mCount.fetch_sub(1);
			assert(val >= 1);

			if (val == 1)
				delete this;

			return val - 1;
		}

		virtual entt::meta_type GetType() const = 0;
		virtual entt::meta_any GetSourceMeta() const = 0;
		virtual std::filesystem::path GetPath() const = 0;
		virtual void BinarySerialize(std::ostream& output) const = 0;
		virtual void BinaryDeserialize(std::istream& input) = 0;
		virtual void BinarySerializeReference(
			const std::filesystem::path& workingPath, 
			cereal::PortableBinaryOutputArchive& output) const = 0;
		virtual void BinaryDeserializeReference(
			const std::filesystem::path& workingPath,
			cereal::PortableBinaryInputArchive& input) = 0;
		virtual BarrierOut MoveAsync(Device device, Context context = Context()) = 0;
		virtual Handle<IResource> MoveIntoHandle() = 0;

		void BinarySerializeReference(
			const std::filesystem::path& workingPath, 
			std::ostream& output) const;
		void BinaryDeserializeReference(
			const std::filesystem::path& workingPath,
			std::istream& input);
		void BinarySerialize(const std::filesystem::path& output) const;
		void BinaryDeserialize(const std::filesystem::path& input);

		void Move(Device device, Context context = Context());

		static void RegisterMetaData();

		friend class Frame;
	};

	class IFrameAbstract : public IResource {
	public:
		virtual Handle<IResource> GetResourceAbstract(entt::entity e) const = 0;
		virtual entt::entity GetEntity(const std::string& name) const = 0;
		virtual const std::unordered_map<entt::entity, ArchiveBlobPointer>&
			GetResourceTable() const = 0;
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

	IFrameAbstract* IResource::GetFrame() const {
		return mFrame;
	}
}