#pragma once

#include <Engine/TextureResource.hpp>

namespace Morpheus {

	class LightProbe {
	private:
		TextureResource* mIrradianceMap;
		TextureResource* mPrefilteredEnvMap;
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

		inline TextureResource* GetPrefilteredEnvMap() {
			return mPrefilteredEnvMap;
		}

		void SetIrradiance(TextureResource* irradiance, DG::ITextureView* irradianceView = nullptr) {

			if (mIrradianceMap) {
				mIrradianceMap->Release();
				mIrradianceMap = nullptr;
			}

			mIrradianceMap = irradiance;
			mIrradianceMapView = irradianceView;

			if (!mIrradianceMapView) {
				mIrradianceMapView = mIrradianceMap->GetShaderView();
			}
		}

		void SetPrefilteredEnvMap(TextureResource* prefilteredEnvMap, DG::ITextureView* prefilteredEnvMapView = nullptr) {
			if (mPrefilteredEnvMap) {
				mPrefilteredEnvMap->Release();
				mPrefilteredEnvMap = nullptr;
			}

			mPrefilteredEnvMap = prefilteredEnvMap;
			mPrefilteredEnvMapView = prefilteredEnvMapView;

			if (!mPrefilteredEnvMapView) {
				mPrefilteredEnvMapView = mPrefilteredEnvMap->GetShaderView();
			}
		}

		inline LightProbe() : 
			mIrradianceMap(nullptr),
			mPrefilteredEnvMap(nullptr) {
		}

		inline LightProbe(TextureResource* irradianceMap, 
			TextureResource* prefilteredEnvMap) :
			mIrradianceMap(irradianceMap),
			mPrefilteredEnvMap(prefilteredEnvMap) {

			mIrradianceMap->AddRef();
			mIrradianceMapView = mIrradianceMap->GetShaderView();

			mPrefilteredEnvMap->AddRef();
			mPrefilteredEnvMapView = mPrefilteredEnvMap->GetShaderView();
		}

		inline LightProbe(const LightProbe& other) :
			mIrradianceMap(other.mIrradianceMap),
			mPrefilteredEnvMap(other.mPrefilteredEnvMap),
			mIrradianceMapView(other.mIrradianceMapView),
			mPrefilteredEnvMapView(other.mPrefilteredEnvMapView) {
			if (mIrradianceMap)
				mIrradianceMap->AddRef();
			if (mPrefilteredEnvMap)
				mPrefilteredEnvMap->AddRef();
		}

		inline ~LightProbe() {
			if (mIrradianceMap)
				mIrradianceMap->Release();
			if (mPrefilteredEnvMap)
				mPrefilteredEnvMap->Release();
		}
	};
}