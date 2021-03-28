#include <Engine/LightProbe.hpp>

namespace Morpheus {
	void LightProbe::SetIrradiance(TextureResource* irradiance, 
		DG::ITextureView* irradianceView) {

		mIrradianceMap = irradiance;
		mIrradianceMapView = irradianceView;

		if (!mIrradianceMapView) {
			mIrradianceMapView = mIrradianceMap->GetShaderView();
		}
	}

	void LightProbe::SetPrefilteredEnvMap(TextureResource* prefilteredEnvMap, 
		DG::ITextureView* prefilteredEnvMapView) {
		mPrefilteredEnvMap = prefilteredEnvMap;
		mPrefilteredEnvMapView = prefilteredEnvMapView;

		if (!mPrefilteredEnvMapView) {
			mPrefilteredEnvMapView = mPrefilteredEnvMap->GetShaderView();
		}
	}
}