#pragma once

#include <Engine/Resources/Texture.hpp>

namespace Morpheus {

	class LightProbe {
	private:
		Handle<Texture> mIrradianceMap;
		Handle<Texture> mPrefilteredEnvMap;
		Handle<DG::IBuffer> mIrradianceSH;

		DG::ITextureView* mIrradianceMapView;
		DG::ITextureView* mPrefilteredEnvMapView;

	public:
		inline DG::ITextureView* GetIrradianceView() {
			return mIrradianceMapView;
		}

		inline DG::ITextureView* GetPrefilteredEnvView() {
			return mPrefilteredEnvMapView;
		}

		inline Handle<Texture> GetIrradianceMap() {
			return mIrradianceMap;
		}

		inline DG::IBuffer* GetIrradianceSH() {
			return mIrradianceSH.Ptr();
		}

		inline Handle<Texture> GetPrefilteredEnvMap() {
			return mPrefilteredEnvMap;
		}

		inline void SetIrradiance(Handle<Texture> irradiance, 
			DG::ITextureView* irradianceView = nullptr);

		inline void SetIrradianceSH(Handle<DG::IBuffer> irradiance) {
			mIrradianceSH = irradiance;
		}

		void SetPrefilteredEnvMap(Handle<Texture> prefilteredEnvMap, 
			DG::ITextureView* prefilteredEnvMapView = nullptr);

		inline LightProbe() : 
			mIrradianceMap(nullptr),
			mPrefilteredEnvMap(nullptr),
			mIrradianceSH(nullptr),
			mIrradianceMapView(nullptr),
			mPrefilteredEnvMapView(nullptr) {
		}

		// Note that either irradianceMap or irradianceSH should be nullptr
		inline LightProbe(Handle<Texture> irradianceMap,
			Handle<DG::IBuffer> irradianceSH,
			Handle<Texture> prefilteredEnvMap) :
			mIrradianceMap(irradianceMap),
			mIrradianceSH(irradianceSH),
			mPrefilteredEnvMap(prefilteredEnvMap) {

			mPrefilteredEnvMapView = mPrefilteredEnvMap ? mPrefilteredEnvMap->GetShaderView() : nullptr;
			mIrradianceMapView = mIrradianceMap ? mIrradianceMap->GetShaderView() : nullptr;
		}
	};
}