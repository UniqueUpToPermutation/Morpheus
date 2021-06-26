#pragma once

#include "RenderDevice.h"
#include "DeviceContext.h"

#include <Engine/Resources/EmbeddedFileLoader.hpp>
#include <Engine/Resources/Resource.hpp>
#include <Engine/ThreadPool.hpp>

namespace DG = Diligent;

namespace Morpheus {

	struct HDRIToCubemapShaders {
		Handle<DG::IShader> mVS;
		Handle<DG::IShader> mPS;

		static ResourceTask<HDRIToCubemapShaders> Load(DG::IRenderDevice* device,
			bool bConvertSRGBToLinear = false,
			IVirtualFileSystem* fileSystem = EmbeddedFileLoader::GetGlobalInstance());
	};

	class HDRIToCubemapConverter {
	private:
		Handle<DG::IPipelineState> mPipelineState;
		Handle<DG::IShaderResourceBinding> mSRB;
		Handle<DG::IBuffer> mTransformConstantBuffer;
		DG::TEXTURE_FORMAT mCubemapFormat;

	public:
		HDRIToCubemapConverter(DG::IRenderDevice* device, 
			const HDRIToCubemapShaders& shaders,
			DG::TEXTURE_FORMAT cubemapFormat);

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