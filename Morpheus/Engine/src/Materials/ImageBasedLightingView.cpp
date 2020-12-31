#include <Engine/Materials/ImageBasedLightingView.hpp>
#include <Engine/MaterialView.hpp>
#include <Engine/LightProbe.hpp>

namespace Morpheus {

	ImageBasedLightingView::ImageBasedLightingView(
		DG::IShaderResourceVariable* irradianceMapLoc,
		DG::IShaderResourceVariable* irradianceMapSHLoc,
		DG::IShaderResourceVariable* prefilteredEnvMapLoc) :
		mIrradianceMapLoc(irradianceMapLoc),
		mIrradianceSHLoc(irradianceMapSHLoc),
		mPrefilteredEnvMapLoc(prefilteredEnvMapLoc) {
	}

	void ImageBasedLightingView::SetEnvironment(
		DG::ITextureView* irradiance,
		DG::IBufferView* irradianceSH,
		DG::ITextureView* prefilteredEnvMap) {

		if (mIrradianceMapLoc)
			mIrradianceMapLoc->Set(irradiance);
		if (mIrradianceSHLoc)
			mIrradianceSHLoc->Set(irradianceSH);
		if (mPrefilteredEnvMapLoc)
			mPrefilteredEnvMapLoc->Set(prefilteredEnvMap);
	}

	void ImageBasedLightingView::SetEnvironment(
		LightProbe* lightProbe) {
		if (mIrradianceMapLoc)
			mIrradianceMapLoc->Set(lightProbe->GetIrradianceView());
		if (mPrefilteredEnvMapLoc)
			mPrefilteredEnvMapLoc->Set(lightProbe->GetPrefilteredEnvView());
		if (mIrradianceSHLoc)
			mIrradianceSHLoc->Set(lightProbe->GetIrradianceSH());
	}
}