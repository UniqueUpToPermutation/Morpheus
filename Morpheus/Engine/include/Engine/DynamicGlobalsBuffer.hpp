#pragma once	

#include "RenderDevice.h"
#include "MapHelper.hpp"

namespace DG = Diligent;

namespace Morpheus {
	template <typename T>
	class DynamicGlobalsBuffer {
	private:
		DG::IBuffer* mBuffer;

	public:
		inline void Initialize(DG::IRenderDevice* device) {
			DG::BufferDesc CBDesc;
			CBDesc.Name           = "VS constants CB";
			CBDesc.uiSizeInBytes  = sizeof(T);
			CBDesc.Usage          = DG::USAGE_DYNAMIC;
			CBDesc.BindFlags      = DG::BIND_UNIFORM_BUFFER;
			CBDesc.CPUAccessFlags = DG::CPU_ACCESS_WRITE;

			mBuffer = nullptr;
			device->CreateBuffer(CBDesc, nullptr, &mBuffer);
		}

		inline DynamicGlobalsBuffer() {
			mBuffer = nullptr;
		}

		inline DynamicGlobalsBuffer(DG::IRenderDevice* device) {
			Initialize(device);
		}

		inline ~DynamicGlobalsBuffer() {
			mBuffer->Release();
		}

		inline DG::IBuffer* Get() {
			return mBuffer;
		}

		inline void Write(DG::IDeviceContext* context, const T& t) {
			DG::MapHelper<T> data(context, mBuffer, DG::MAP_WRITE, DG::MAP_FLAG_DISCARD);
			*data = t;
		}
	};

	struct RendererGlobalData;
	struct LightProbe;
}