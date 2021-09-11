#pragma once

#include <Engine/Defines.hpp>
#include <Engine/Resources/Resource.hpp>


namespace Morpheus {

	class BufferMap;

	template <typename T>
	class TypedBufferMap {
	private:
		Context mContext;
		Handle<DG::IBuffer> mGpuBuffer;
		T* mPtr;
		size_t mSize;
		DG::MAP_TYPE mType;
		DG::MAP_FLAGS mFlags;

	public:
		~TypedBufferMap() {
			if (mGpuBuffer) {
				mContext.mUnderlying.mGpuContext->UnmapBuffer(mGpuBuffer, mType);
			}
		}

		TypedBufferMap(Context context,
			Handle<DG::IBuffer> buffer,
			T* ptr,
			size_t size,
			DG::MAP_TYPE type,
			DG::MAP_FLAGS flags);
		TypedBufferMap(BufferMap&& other);

		TypedBufferMap(TypedBufferMap<T>&&) = default;
		TypedBufferMap(const TypedBufferMap<T>&) = delete;
		
		TypedBufferMap<T>& operator=(TypedBufferMap<T>&&) = default;
		TypedBufferMap<T>& operator=(const TypedBufferMap<T>&) = delete;

		inline DG::MAP_TYPE type() const {
			return mType;
		}

		inline DG::MAP_FLAGS flags() const {
			return mFlags;
		}

		inline T& operator[](size_t i) const {
			return mPtr[i];
		}

		inline T* data() const {
			return mPtr;
		}

		inline size_t size() const {
			return mSize;
		}

		inline size_t byte_size() const {
			return mSize * sizeof(T);
		}
	};

	class BufferMap : public TypedBufferMap<uint8_t> {
	public:
		inline BufferMap(Context context,
			Handle<DG::IBuffer> buffer,
			uint8_t* ptr,
			size_t size,
			DG::MAP_TYPE type,
			DG::MAP_FLAGS flags) :
			TypedBufferMap<uint8_t>(context, buffer, ptr, size, type, flags) {
		}
	};

	template <typename T>
	TypedBufferMap<T>::TypedBufferMap(Context context,
		Handle<DG::IBuffer> buffer,
		T* ptr,
		size_t size,
		DG::MAP_TYPE type,
		DG::MAP_FLAGS flags) :
		mContext(context),
		mGpuBuffer(buffer),
		mPtr(ptr),
		mSize(size),
		mType(type),
		mFlags(flags) {
	}

	template <typename T>
	TypedBufferMap<T>::TypedBufferMap(BufferMap&& other) :
		mContext(std::move(other.mContext)),
		mGpuBuffer(std::move(other.mGpuBuffer)),
		mPtr(std::move(other.mPtr)),
		mSize(std::move(other.mSize)),
		mType(std::move(other.mType)),
		mFlags(std::move(other.mFlags)) {
		mSize /= (sizeof(T) / sizeof(uint8_t));
	}

	struct GPUBufferRead {
		Handle<DG::IFence> mFence;
		Handle<DG::IBuffer> mStagingBuffer;
		DG::BufferDesc mBufferDesc;
		DG::Uint64 mFenceCompletedValue;

		inline bool IsReady() const {
			return mFence->GetCompletedValue() == mFenceCompletedValue;
		}
	};

	struct GPUMultiBufferRead {
		Handle<DG::IFence> mFence;
		std::vector<Handle<DG::IBuffer>> mStagingBuffers;
		std::vector<DG::BufferDesc> mBufferDesc;
		DG::Uint64 mFenceCompletedValue;

		inline bool IsReady() const {
			return mFence->GetCompletedValue() == mFenceCompletedValue;
		}
	};


	class Buffer : public IResource {
	private:
		struct GpuAspect {
			Handle<DG::IBuffer> mBuffer;
		} mGpuAspect;

		struct CpuAspect {
			std::vector<uint8_t> mData;
			DG::BufferDesc mDesc;
		} mCpuAspect;

		void CreateGpuAspect(Device device, Buffer* other);
	
	public:
		Buffer() = default;

		Buffer(Buffer&& other) = default;
		Buffer(const Buffer& other) = delete;

		Buffer& operator=(Buffer&& other) = default;
		Buffer& operator=(const Buffer& other) = delete;

		void CopyFrom(const Buffer* buffer);
		void CopyTo(Buffer* buffer) const;

		Buffer(const DG::BufferDesc& desc);
		Buffer(const DG::BufferDesc& desc, std::vector<uint8_t>&& data);
		Buffer(const DG::BufferDesc& desc, const std::vector<uint8_t>& data);

		Buffer(Device device, const DG::BufferDesc& desc);
		Buffer(Device device, const DG::BufferDesc& desc, std::vector<uint8_t>&& data);
		Buffer(Device device, const DG::BufferDesc& desc, const std::vector<uint8_t>& data);

		entt::meta_type GetType() const override;
		entt::meta_any GetSourceMeta() const override;
		std::filesystem::path GetPath() const override;
		void BinarySerialize(cereal::PortableBinaryOutputArchive& archive) const;
		void BinaryDeserialize(cereal::PortableBinaryInputArchive& archive);
		void BinarySerialize(std::ostream& output) const override;
		void BinaryDeserialize(std::istream& input) override;
		void BinarySerializeReference(
			const std::filesystem::path& workingPath, 
			cereal::PortableBinaryOutputArchive& output) const override;
		void BinaryDeserializeReference(
			const std::filesystem::path& workingPath,
			cereal::PortableBinaryInputArchive& input) override;
		BarrierOut MoveAsync(Device device, Context context = Context()) override;
		Handle<IResource> MoveIntoHandle() override;

		UniqueFuture<Buffer> ToAsync(Device device, Context context = Context());
		Buffer To(Device device, Context context = Context());

		BufferMap Map(Context context, DG::MAP_TYPE type, DG::MAP_FLAGS flags);

		template <typename T>
		TypedBufferMap<T> TypedMap(Context context, DG::MAP_TYPE type, DG::MAP_FLAGS flags) {
			return Map(context, type, flags);
		}

		static GPUBufferRead BeginGPURead(
			DG::IBuffer* buffer, 
			DG::IRenderDevice* device, 
			DG::IDeviceContext* context);

		static void FinishGPURead(
			DG::IDeviceContext* context,
			const GPUBufferRead& read,
			Buffer& textureOut);

		static GPUMultiBufferRead BeginGPUMultiRead(
			const std::vector<DG::IBuffer*>& buffers,
			DG::IRenderDevice* device, 
			DG::IDeviceContext* context);

		static std::vector<std::vector<uint8_t>> FinishGPUMultiRead(
			DG::IDeviceContext* context,
			const GPUMultiBufferRead& read);

		UniqueFuture<Buffer> GPUToCPUAsync(Device device, Context context) const;
	};
}