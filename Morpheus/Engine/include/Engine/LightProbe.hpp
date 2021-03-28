#pragma once

#include <Engine/Resources/TextureResource.hpp>

namespace Morpheus {

	class LightProbe {
	private:
		RefHandle<TextureResource> mIrradianceMap;
		RefHandle<TextureResource> mPrefilteredEnvMap;
		DG::RefCntAutoPtr<DG::IBuffer> mIrradianceSH;

		DG::ITextureView* mIrradianceMapView;
		DG::ITextureView* mPrefilteredEnvMapView;

	public:
		inline DG::ITextureView* GetIrradianceView() {
			return mIrradianceMapView;
		}

		inline DG::ITextureView* GetPrefilteredEnvView() {
			return mPrefilteredEnvMapView;
		}

		inline TextureResource* GetIrradianceMap() {
			return mIrradianceMap;
		}

		inline DG::IBuffer* GetIrradianceSH() {
			return mIrradianceSH.RawPtr();
		}

		inline TextureResource* GetPrefilteredEnvMap() {
			return mPrefilteredEnvMap;
		}

		inline void SetIrradiance(TextureResource* irradiance, 
			DG::ITextureView* irradianceView = nullptr);

		inline void SetIrradianceSH(DG::RefCntAutoPtr<DG::IBuffer> irradiance) {
			mIrradianceSH = irradiance;
		}

		void SetPrefilteredEnvMap(TextureResource* prefilteredEnvMap, 
			DG::ITextureView* prefilteredEnvMapView = nullptr);

		inline LightProbe() : 
			mIrradianceMap(nullptr),
			mPrefilteredEnvMap(nullptr),
			mIrradianceSH(nullptr),
			mIrradianceMapView(nullptr),
			mPrefilteredEnvMapView(nullptr) {
		}

		// Note that either irradianceMap or irradianceSH should be nullptr
		inline LightProbe(TextureResource* irradianceMap,
			DG::RefCntAutoPtr<DG::IBuffer> irradianceSH,
			TextureResource* prefilteredEnvMap) :
			mIrradianceMap(irradianceMap),
			mIrradianceSH(irradianceSH),
			mPrefilteredEnvMap(prefilteredEnvMap) {

			mPrefilteredEnvMapView = mPrefilteredEnvMap ? mPrefilteredEnvMap->GetShaderView() : nullptr;
			mIrradianceMapView = mIrradianceMap ? mIrradianceMap->GetShaderView() : nullptr;
		}
	};
}