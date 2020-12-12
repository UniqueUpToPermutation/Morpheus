#include <Engine/MaterialView.hpp>
#include <Engine/LightProbe.hpp>

namespace Morpheus {

	ImageBasedLightingView::ImageBasedLightingView(
		DG::IShaderResourceVariable* irradianceMapLoc,
		DG::IShaderResourceVariable* prefilteredEnvMapLoc) :
		mIrradianceMapLoc(irradianceMapLoc),
		mPrefilteredEnvMapLoc(prefilteredEnvMapLoc) {
	}

	void ImageBasedLightingView::SetEnvironment(
		DG::ITextureView* irradiance,
		DG::ITextureView* prefilteredEnvMap) {

		mIrradianceMapLoc->Set(irradiance);
		mPrefilteredEnvMapLoc->Set(prefilteredEnvMap);
	}

	void ImageBasedLightingView::SetEnvironment(
		LightProbe* lightProbe) {
		mIrradianceMapLoc->Set(lightProbe->GetIrradianceView());
		mPrefilteredEnvMapLoc->Set(lightProbe->GetPrefilteredEnvView());
	}
}