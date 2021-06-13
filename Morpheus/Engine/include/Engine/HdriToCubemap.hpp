#pragma once

#include "RenderDevice.h"
#include "DeviceContext.h"

namespace DG = Diligent;

namespace Morpheus {
	class HDRIToCubemapConverter {
	private:
		DG::IPipelineState* mPipelineState;
		DG::IShaderResourceBinding* mSRB;
		DG::IBuffer* mTransformConstantBuffer;
		DG::TEXTURE_FORMAT mCubemapFormat;

	public:
		HDRIToCubemapConverter(DG::IRenderDevice* device);
		~HDRIToCubemapConverter();

		void Initialize(DG::IRenderDevice* device, 
			DG::TEXTURE_FORMAT cubemapFormat,
			bool bConvertSRGBToLinear = false);

		void Convert(DG::IDeviceContext* context, 
			DG::ITextureView* hdri,
			DG::ITexture* outputCubemap);

		DG::ITexture* Convert(DG::IRenderDevice* device,
			DG::IDeviceContext* context,
			DG::ITextureView* hdri,
			uint size,
			bool bGenerateMips = false);
	};
}