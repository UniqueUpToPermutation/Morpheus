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
		archive(m.Size);
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

	void Buffer::CopyTo(Buffer* buffer) const {
		buffer->CopyFrom(this);
	}

	Buffer::Buffer(const DG::BufferDesc& desc) {
		mCpuAspect.mDesc = desc;
		mCpuAspect.mData.resize(desc.Size);
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
		throw std::runtime_error("Does not have source!");
	}

	std::filesystem::path Buffer::GetPath() const {
		throw std::runtime_error("Does not have path!");
	}

	void Buffer::BinarySerialize(cereal::PortableBinaryOutputArchive& archive) {
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

	void Buffer::BinarySerialize(std::ostream& output, IDependencyResolver* dependencies) {
		cereal::PortableBinaryOutputArchive arr(output);
		BinarySerialize(arr);
	}

	void Buffer::BinaryDeserialize(std::istream& input, const IDependencyResolver* dependencies) {
		cereal::PortableBinaryInputArchive arr(input);
		BinaryDeserialize(arr);
	}

	void Buffer::BinarySerializeReference(
		const std::filesystem::path& workingPath, 
		cereal::PortableBinaryOutputArchive& output) {
		throw std::runtime_error("Not implemented!");
	}

	void Buffer::BinaryDeserializeReference(
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
				stage_desc.Size,
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
		cpuAspect.mData.resize(read.mBufferDesc.Size);

		DG::PVoid ptr;
		context->MapBuffer(read.mStagingBuffer, 
			DG::MAP_READ, DG::MAP_FLAG_DO_NOT_WAIT,
			ptr);
		std::memcpy(&cpuAspect.mData[0], ptr, read.mBufferDesc.Size);
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

		throw std::runtime_error("Improper devices detected!");
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
				mGpuAspect.mBuffer->GetDesc().Size,
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

	GPUMultiBufferRead Buffer::BeginGPUMultiRead(
		const std::vector<DG::IBuffer*>& buffers,
		DG::IRenderDevice* device, 
		DG::IDeviceContext* context) {

		GPUMultiBufferRead result;
		result.mFence = nullptr;

		for (auto buf : buffers) {
			auto& desc = buf->GetDesc();

			if (desc.CPUAccessFlags & DG::CPU_ACCESS_READ) {
				result.mStagingBuffers.emplace_back(buf);
				result.mBufferDesc.emplace_back(desc);
			} else {
				// Create staging texture
				auto stage_desc = desc;
				stage_desc.Name = "CPU Retrieval Buffer";
				stage_desc.CPUAccessFlags = DG::CPU_ACCESS_READ;
				stage_desc.Usage = DG::USAGE_STAGING;
				stage_desc.BindFlags = DG::BIND_NONE;

				DG::IBuffer* stage_buf = nullptr;
				device->CreateBuffer(stage_desc, nullptr, &stage_buf);

				result.mStagingBuffers.emplace_back(stage_buf);
				result.mBufferDesc.emplace_back(desc);

				context->CopyBuffer(buf, 0, 
					DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
					stage_buf, 0, stage_desc.Size,
					DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

				if (!result.mFence) {
					// Retrieve data from staging texture
					DG::FenceDesc fence_desc;
					fence_desc.Name = "CPU Retrieval Fence";
					fence_desc.Type = DG::FENCE_TYPE_GENERAL;
					DG::IFence* fence = nullptr;
					device->CreateFence(fence_desc, result.mFence.Ref());
				}
				
			}
		}

		if (result.mFence) {
			context->EnqueueSignal(result.mFence, 1);
		}

		return result;
	}

	std::vector<std::vector<uint8_t>> Buffer::FinishGPUMultiRead(
		DG::IDeviceContext* context,
		const GPUMultiBufferRead& read) {

		std::vector<std::vector<uint8_t>> result;

		for (int i = 0; i < read.mBufferDesc.size(); ++i) {
			const auto& desc = read.mBufferDesc[i];
			auto buf = read.mStagingBuffers[i];

			if (!desc.CPUAccessFlags & DG::CPU_ACCESS_READ) {
				throw std::runtime_error("Invalid GPU Read!");
			}

			std::vector<uint8_t> dest;
			dest.resize(desc.Size);

			DG::PVoid ptr = nullptr;
			context->MapBuffer(buf, DG::MAP_READ, DG::MAP_FLAG_DO_NOT_WAIT, ptr);
			std::memcpy(&dest[0], ptr, desc.Size);
			context->UnmapBuffer(buf, DG::MAP_READ);

			result.emplace_back(std::move(dest));
		}

		return result;
	}

	void Buffer::CreateGpuAspect(Device device, Buffer* other) {
		DG::BufferData data;
		data.DataSize = other->mCpuAspect.mData.size() * 
			sizeof(decltype(other->mCpuAspect.mData[0]));
		data.pData = &other->mCpuAspect.mData[0];

		DG::BufferData* pData = nullptr;
		if (other->mCpuAspect.mData.size() > 0) {
			pData = &data;
		}

		device.mUnderlying.mGpuDevice->CreateBuffer(other->mCpuAspect.mDesc, 
			pData, mGpuAspect.mBuffer.Ref());
	}

	Handle<IResource> Buffer::MoveIntoHandle() {
		return Handle<Buffer>(std::move(*this)).DownCast<IResource>();
	}
}