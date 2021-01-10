#pragma once

#include "RenderDevice.h"
#include "DeviceContext.h"

namespace DG = Diligent;

#include <Engine/Resources/ResourceManager.hpp>

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

		void Initialize(ResourceManager* resourceManager, 
			DG::TEXTURE_FORMAT cubemapFormat,
			bool bConvertSRGBToLinear = false);

		void Convert(DG::IDeviceContext* context, 
			DG::ITextureView* hdri,
			DG::ITexture* outputCubemap);

		DG::ITexture* Convert(DG::IRenderDevice* device,
			DG::IDeviceContext* context,
			DG::ITextureView* hdri,
			uint size);
	};
}