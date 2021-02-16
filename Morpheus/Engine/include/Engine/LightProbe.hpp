#pragma once

#include <Engine/Resources/TextureResource.hpp>

namespace Morpheus {

	class LightProbe {
	private:
		TextureResource* mIrradianceMap;
		TextureResource* mPrefilteredEnvMap;
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

		void SetIrradiance(TextureResource* irradiance, 
			DG::ITextureView* irradianceView = nullptr) {

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

		void SetIrradianceSH(DG::RefCntAutoPtr<DG::IBuffer> irradiance) {
			mIrradianceSH = irradiance;
		}

		void SetPrefilteredEnvMap(TextureResource* prefilteredEnvMap, 
			DG::ITextureView* prefilteredEnvMapView = nullptr) {
			if (mPrefilteredEnvMap) {
				mPrefilteredEnvMap->Release();
				mPrefilteredEnvMap = nullptr;
			}

			mPrefilteredEnvMap = prefilteredEnvMap;
			mPrefilteredEnvMapView = prefilteredEnvMapView;

			mPrefilteredEnvMap->AddRef();

			if (!mPrefilteredEnvMapView) {
				mPrefilteredEnvMapView = mPrefilteredEnvMap->GetShaderView();
			}
		}

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

			if (mIrradianceMap) {
				mIrradianceMap->AddRef();
				mIrradianceMapView = mIrradianceMap->GetShaderView();
			}

			mPrefilteredEnvMap->AddRef();
			mPrefilteredEnvMapView = mPrefilteredEnvMap->GetShaderView();
		}

		inline LightProbe(const LightProbe& other) :
			mIrradianceMap(other.mIrradianceMap),
			mPrefilteredEnvMap(other.mPrefilteredEnvMap),
			mIrradianceMapView(other.mIrradianceMapView),
			mPrefilteredEnvMapView(other.mPrefilteredEnvMapView),
			mIrradianceSH(other.mIrradianceSH) {

			if (mIrradianceMap)
				mIrradianceMap->AddRef();
			if (mPrefilteredEnvMap)
				mPrefilteredEnvMap->AddRef();
		}

		inline void MoveFrom(LightProbe&& other) {
			mIrradianceMap = other.mIrradianceMap;
			mIrradianceMapView =other.mIrradianceMapView;
			mPrefilteredEnvMap = other.mPrefilteredEnvMap;
			mPrefilteredEnvMapView = other.mPrefilteredEnvMapView;
			mIrradianceSH = other.mIrradianceSH;

			other.mIrradianceMap = nullptr;
			other.mIrradianceMapView = nullptr;
			other.mPrefilteredEnvMap = nullptr;
			other.mPrefilteredEnvMapView = nullptr;
			other.mIrradianceSH = nullptr;
		}

		inline LightProbe(LightProbe&& other) {
			MoveFrom(std::move(other));
		}

		inline LightProbe& operator=(LightProbe&& other) {
			MoveFrom(std::move(other));

			return *this;
		}

		inline LightProbe& operator=(const LightProbe& other) {
	
			if (mIrradianceMap)
				mIrradianceMap->Release();
			if (mPrefilteredEnvMap)
				mPrefilteredEnvMap->Release();

			mIrradianceMap = other.mIrradianceMap;
			mIrradianceMapView =other.mIrradianceMapView;
			mPrefilteredEnvMap = other.mPrefilteredEnvMap;
			mPrefilteredEnvMapView = other.mPrefilteredEnvMapView;
			mIrradianceSH = other.mIrradianceSH;

			if (mIrradianceMap)
				mIrradianceMap->AddRef();
			if (mPrefilteredEnvMap)
				mPrefilteredEnvMap->AddRef();

			return *this;
		}

		inline ~LightProbe() {
			if (mIrradianceMap)
				mIrradianceMap->Release();
			if (mPrefilteredEnvMap)
				mPrefilteredEnvMap->Release();
		}
	};
}