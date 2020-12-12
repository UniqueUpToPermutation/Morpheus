#include <Engine/MaterialView.hpp>

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
}