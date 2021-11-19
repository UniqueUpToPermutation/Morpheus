#pragma once	

#include "RenderDevice.h"
#include "MapHelper.hpp"

#include <cstring>

#include <Engine/Resources/Resource.hpp>

namespace DG = Diligent;

namespace Morpheus {
	template <typename T>
	class DynamicUniformBuffer {
	private:
		Handle<DG::IBuffer> mBuffer;

	public:
		DynamicUniformBuffer() = default;

		inline void Initialize(DG::IRenderDevice* device, const uint count = 1) {
			DG::BufferDesc CBDesc;
			CBDesc.Name           = "Dyanmic Globals Buffer";
			CBDesc.uiSizeInBytes  = sizeof(T) * count;
			CBDesc.Usage          = DG::USAGE_DYNAMIC;
			CBDesc.BindFlags      = DG::BIND_UNIFORM_BUFFER;
			CBDesc.CPUAccessFlags = DG::CPU_ACCESS_WRITE;

			device->CreateBuffer(CBDesc, nullptr, mBuffer.Ref());
		}

		inline DynamicUniformBuffer(DG::IRenderDevice* device, const uint count = 1) {
			Initialize(device, count);
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

	struct RendererGlobalData;
	struct LightProbe;
}