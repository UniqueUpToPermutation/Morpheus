#include <Engine/Resources/Buffer.hpp>

#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/vector.hpp>

namespace Diligent {
	template <class Archive>
	void serialize(Archive& archive,
		DG::BufferDesc& m)
	{
		archive(m.BindFlags);
		archive(m.CPUAccessFlags);
		archive(m.ElementByteStride);
		archive(m.ImmediateContextMask);
		archive(m.Mode);
		archive(m.uiSizeInBytes);
		archive(m.Usage);
	}
}

namespace Morpheus {
	void Buffer::CopyFrom(const Buffer* buffer) {
		if (!buffer->mDevice.IsCPU())
			throw std::runtime_error("Buffer must be on CPU!");
		
		mDevice = buffer->mDevice;
		mCpuAspect = buffer->mCpuAspect;
	}

	Buffer::Buffer(Buffer&& other) {
		*this = std::move(other);
	}

	Buffer& Buffer::operator=(Buffer&& other) {
		mCpuAspect = std::move(other.mCpuAspect);
		mGpuAspect = std::move(other.mGpuAspect);
		mDevice = std::move(other.mDevice);
	}

	void Buffer::CopyTo(Buffer* buffer) const {
		buffer->CopyFrom(this);
	}

	Buffer::Buffer(const DG::BufferDesc& desc) {
		mCpuAspect.mDesc = desc;
		mCpuAspect.mData.resize(desc.uiSizeInBytes);
		mDevice = Device::CPU();
	}

	Buffer::Buffer(const DG::BufferDesc& desc, std::vector<uint8_t>&& data) {
		mCpuAspect.mDesc = desc;
		mCpuAspect.mData = std::move(data);
		mDevice = Device::CPU();
	}

	Buffer::Buffer(const DG::BufferDesc& desc, const std::vector<uint8_t>& data) {
		mCpuAspect.mDesc = desc;
		mCpuAspect.mData = data;
		mDevice = Device::CPU();
	}

	Buffer::Buffer(Device device, const DG::BufferDesc& desc) : Buffer(desc) {
		Move(device);
	}

	Buffer::Buffer(Device device, const DG::BufferDesc& desc, std::vector<uint8_t>&& data) {
		Move(device);
	}

	Buffer::Buffer(Device device, const DG::BufferDesc& desc, const std::vector<uint8_t>& data) {
		Move(device);
	}

	entt::meta_type Buffer::GetType() const {
		return entt::resolve<Buffer>();
	}

	entt::meta_any Buffer::GetSourceMeta() const {
		return nullptr;
	}

	void Buffer::BinarySerialize(cereal::PortableBinaryOutputArchive& archive) const {
		if (!mDevice.IsCPU())
			std::runtime_error("Resource must be on the CPU!");

		archive(mCpuAspect.mDesc);
		archive(mCpuAspect.mData);
	}

	void Buffer::BinaryDeserialize(cereal::PortableBinaryInputArchive& archive) {
		mDevice = Device::CPU();

		archive(mCpuAspect.mDesc);
		archive(mCpuAspect.mData);
	}

	void Buffer::BinarySerialize(std::ostream& output) const {
		cereal::PortableBinaryOutputArchive arr(output);
		BinarySerialize(arr);
	}

	void Buffer::BinaryDeserialize(std::istream& input) {
		cereal::PortableBinaryInputArchive arr(input);
		BinaryDeserialize(input);
	}

	void Buffer::BinarySerializeSource(
		const std::filesystem::path& workingPath, 
		cereal::PortableBinaryOutputArchive& output) const {
		throw std::runtime_error("Not implemented!");
	}

	void Buffer::BinaryDeserializeSource(
		const std::filesystem::path& workingPath,
		cereal::PortableBinaryInputArchive& input) {
		throw std::runtime_error("Not implemented!");
	}

	GPUBufferRead Buffer::BeginGPURead(
		DG::IBuffer* buffer, 
		DG::IRenderDevice* device, 
		DG::IDeviceContext* context) {
		
		auto& desc = buffer->GetDesc();

		if (desc.CPUAccessFlags & DG::CPU_ACCESS_READ) {
			GPUBufferRead result;
			result.mFence = nullptr;
			result.mStagingBuffer = buffer;
			result.mBufferDesc = desc;
			return result;
		} else {
			// Create staging texture
			auto stage_desc = buffer->GetDesc();
			stage_desc.Name = "CPU Retrieval Buffer";
			stage_desc.CPUAccessFlags = DG::CPU_ACCESS_READ;
			stage_desc.Usage = DG::USAGE_STAGING;
			stage_desc.BindFlags = DG::BIND_NONE;

			DG::IBuffer* stage_buffer = nullptr;
			device->CreateBuffer(stage_desc, nullptr, &stage_buffer);

			DG::CopyTextureAttribs copy_attribs;

			context->CopyBuffer(buffer, 0,
				DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
				stage_buffer, 0,
				stage_desc.uiSizeInBytes,
				DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			// Retrieve data from staging texture
			DG::FenceDesc fence_desc;
			fence_desc.Name = "CPU Retrieval Fence";
			fence_desc.Type = DG::FENCE_TYPE_GENERAL;
			DG::IFence* fence = nullptr;
			device->CreateFence(fence_desc, &fence);
			context->EnqueueSignal(fence, 1);

			GPUBufferRead result;
			result.mFence.Adopt(fence);
			result.mStagingBuffer.Adopt(stage_buffer);
			result.mBufferDesc = desc;
			result.mFenceCompletedValue = 1;
			return result;
		}
	}

	void Buffer::FinishGPURead(
		DG::IDeviceContext* context,
		const GPUBufferRead& read,
		Buffer& textureOut) {
		textureOut.mDevice = Device::CPU();

		auto& desc = read.mStagingBuffer->GetDesc();

		if (!desc.CPUAccessFlags & DG::CPU_ACCESS_READ) {
			throw std::runtime_error("Invalid GPU Read!");
		}

		auto& cpuAspect = textureOut.mCpuAspect;

		cpuAspect.mDesc = read.mBufferDesc;
		cpuAspect.mData.resize(read.mBufferDesc.uiSizeInBytes);

		DG::PVoid ptr;
		context->MapBuffer(read.mStagingBuffer, 
			DG::MAP_READ, DG::MAP_FLAG_DO_NOT_WAIT,
			ptr);
		std::memcpy(&cpuAspect.mData[0], ptr, read.mBufferDesc.uiSizeInBytes);
		context->UnmapBuffer(read.mStagingBuffer, DG::MAP_READ);
	}

	UniqueFuture<Buffer> Buffer::GPUToCPUAsync(Device device, Context context) const {
			
		if (!mDevice.IsGPU()) {
			throw std::runtime_error("Texture must be on GPU!");
		}

		auto readProc = Buffer::BeginGPURead(mGpuAspect.mBuffer, mDevice, context);

		Promise<Buffer> result;

		auto lambda = [readProc, context](const TaskParams& e, BarrierOut, Promise<Buffer> output) {
			Buffer texture;
			Buffer::FinishGPURead(context, readProc, texture);
			output = std::move(texture);
		};

		if (!readProc.mFence) {
			lambda(TaskParams(), Barrier(), result);
			return result;
		} else {
			FunctionPrototype<BarrierOut, Promise<Buffer>> prototype(std::move(lambda));
			Barrier gpuBarrier(readProc.mFence, readProc.mFenceCompletedValue);
			gpuBarrier.Node().OnlyThread(THREAD_MAIN);
			prototype(gpuBarrier, result)
				.SetName("Copy Staging Buffer to CPU")
				.OnlyThread(THREAD_MAIN);
			return result;
		}
	}

	BarrierOut Buffer::MoveAsync(Device device, Context context) {
		Barrier result;

		FunctionPrototype<UniqueFuture<Buffer>, Barrier> swapPrototype([this](
			const TaskParams& e, 
			UniqueFuture<Buffer> inBuffer, 
			Barrier) {
			*this = std::move(inBuffer.Get());
		});

		UniqueFuture<Buffer> movedTexture = ToAsync(device, context);
		swapPrototype(movedTexture, result);
		return result;
	}

	UniqueFuture<Buffer> Buffer::ToAsync(Device device, Context context) {
		
		if (device.IsCPU()) {
			if (mDevice.IsCPU()) {
				Buffer buffer;
				buffer.CopyFrom(this);
				return Promise<Buffer>(std::move(buffer));
			} else if (mDevice.IsGPU()) {
				return GPUToCPUAsync(device, context);
			}
		} else if (mDevice.IsGPU()) {
			if (device.IsGPU()) {
				throw std::runtime_error("Not supported!");
			} else if (mDevice.IsCPU()) {
				Buffer buffer;
				buffer.CreateGpuAspect(device, this);
				return Promise<Buffer>(std::move(buffer));
			}
		}
	}

	Buffer Buffer::To(Device device, Context context) {
		auto future = ToAsync(device, context);
		context.Flush();
		return std::move(future.Evaluate());
	}

	BufferMap Buffer::Map(Context context, DG::MAP_TYPE type, DG::MAP_FLAGS flags) {
		if (mDevice.IsGPU()) {
			DG::PVoid out = nullptr;
			context.mUnderlying.mGpuContext->MapBuffer(
				mGpuAspect.mBuffer, type, flags, out);
			return BufferMap(context,
				mGpuAspect.mBuffer,
				(uint8_t*)out, 
				mGpuAspect.mBuffer->GetDesc().uiSizeInBytes,
				type,
				flags);
		} else {
			return BufferMap(context,
				nullptr,
				&mCpuAspect.mData[0],
				mCpuAspect.mData.size(),
				type,
				flags);
		}
	}
}