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

		if (mIrradianceMapLoc)
			mIrradianceMapLoc->Set(irradiance);
		if (mPrefilteredEnvMapLoc)
			mPrefilteredEnvMapLoc->Set(prefilteredEnvMap);
	}

	void ImageBasedLightingView::SetEnvironment(
		LightProbe* lightProbe) {
		if (mIrradianceMapLoc)
			mIrradianceMapLoc->Set(lightProbe->GetIrradianceView());
		if (mPrefilteredEnvMapLoc)
			mPrefilteredEnvMapLoc->Set(lightProbe->GetPrefilteredEnvView());
	}
}