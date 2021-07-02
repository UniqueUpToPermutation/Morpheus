#pragma once	

#include "RenderDevice.h"
#include "MapHelper.hpp"
#include "Pool.hpp"

#include <cstring>

#include <Engine/Resources/Resource.hpp>

namespace DG = Diligent;

namespace Morpheus {
	template <typename T>
	class DynamicGlobalsBuffer {
	private:
		Handle<DG::IBuffer> mBuffer;

	public:
		inline void Initialize(DG::IRenderDevice* device, const uint count = 1) {
			DG::BufferDesc CBDesc;
			CBDesc.Name           = "Dyanmic Globals Buffer";
			CBDesc.uiSizeInBytes  = sizeof(T) * count;
			CBDesc.Usage          = DG::USAGE_DYNAMIC;
			CBDesc.BindFlags      = DG::BIND_UNIFORM_BUFFER;
			CBDesc.CPUAccessFlags = DG::CPU_ACCESS_WRITE;

			device->CreateBuffer(CBDesc, nullptr, mBuffer.Ref());
		}

		inline DynamicGlobalsBuffer() {
		}

		inline DynamicGlobalsBuffer(DG::IRenderDevice* device) {
			Initialize(device);
		}

		inline DG::IBuffer* Get() {
			return mBuffer.Ptr();
		}

		inline void Write(DG::IDeviceContext* context, const T& t) {
			DG::MapHelper<T> data(context, mBuffer, DG::MAP_WRITE, DG::MAP_FLAG_DISCARD);
			*data = t;
		}

		inline void Write(DG::IDeviceContext* context, const T t[], const uint count) {
			DG::MapHelper<T> data(context, mBuffer, DG::MAP_WRITE, DG::MAP_FLAG_DISCARD);
			std::memcpy(data, t, sizeof(T) * count);
		}
	};

	template <typename T>
	class PoolBuffer : public PoolBase {
	private:
		Handle<DG::IBuffer> mInstanceBuffer;
	
	public:
		PoolBuffer() : PoolBase(0) {
		}

		PoolBuffer(DG::IRenderDevice* device, uint instanceCount) : PoolBase(instanceCount) {
			DG::BufferDesc CBDesc;
			CBDesc.Name           = "Dyanmic Globals Buffer";
			CBDesc.uiSizeInBytes  = sizeof(T) * instanceCount;
			CBDesc.Usage          = DG::USAGE_DYNAMIC;
			CBDesc.BindFlags      = DG::BIND_UNIFORM_BUFFER;
			CBDesc.CPUAccessFlags = DG::CPU_ACCESS_WRITE;

			device->CreateBuffer(CBDesc, nullptr, mInstanceBuffer.Ref());
		}

		inline void Write(DG::IDeviceContext* context, int id, const T& value) {
			context->UpdateBuffer(mInstanceBuffer, sizeof(T) * id, sizeof(T), &value, 
				DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		}

		inline int AllocWrite(DG::IDeviceContext* context, const T& value) {

		}
	};

	struct RendererGlobalData;
	struct LightProbe;
}