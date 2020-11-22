#pragma once

#include "RenderDevice.h"
#include "DeviceContext.h"

#include <Engine/ResourceManager.hpp>

namespace DG = Diligent;

namespace Morpheus {
	class CookTorranceLUT {
	private:
		DG::ITexture* mLut;

	public:
		CookTorranceLUT(DG::IRenderDevice* device, 
			uint SamplesPerPixel=512, 
			uint NdotVSamples=512);

		~CookTorranceLUT();

		void Compute(ResourceManager* resourceManager,
			DG::IDeviceContext* context, 
			DG::IRenderDevice* device,
			uint Samples=512);

		inline DG::ITexture* GetLUT() {
			return mLut;
		}

		inline DG::ITextureView* GetShaderView() {
			return mLut->GetDefaultView(DG::TEXTURE_VIEW_SHADER_RESOURCE);
		}

		void SavePng(const std::string& path, DG::IDeviceContext* context, DG::IRenderDevice* device);
		void SaveGli(const std::string& path, DG::IDeviceContext* context, DG::IRenderDevice* device);
	};
}