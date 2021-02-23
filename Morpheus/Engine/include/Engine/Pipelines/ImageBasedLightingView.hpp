#pragma once

#include <vector>

#include <Engine/Resources/PipelineResource.hpp>

namespace DG = Diligent;

namespace Morpheus {
	class LightProbe;

	class ImageBasedLightingView {
	private:
		bool bUseSH = false;
		std::vector<DG::IShaderResourceVariable*> mIrradianceMapLoc;
		std::vector<DG::IShaderResourceVariable*> mPrefilteredEnvMapLoc;
		std::vector<DG::IShaderResourceVariable*> mIrradianceSHLoc;
	
	public:
		ImageBasedLightingView(PipelineResource* pipeline);

		void SetEnvironment(
			DG::ITextureView* irradiance,
			DG::IBuffer* irradianceMapSH,
			DG::ITextureView* prefilteredEnvMap, 
			uint srbId);

		void SetEnvironment(
			LightProbe* lightProbe, uint srbId);
	};
}