#pragma once

#include <Engine/Resources/Texture.hpp>

namespace Morpheus {

	class LightProbe {
	private:
		Handle<Texture> mPrefilteredEnvMap;
		Handle<DG::IBuffer> mIrradianceSH;

		DG::ITextureView* mPrefilteredEnvMapView;

	public:
		inline DG::ITextureView* GetPrefilteredEnvView() {
			return mPrefilteredEnvMapView;
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
			mPrefilteredEnvMap(nullptr),
			mIrradianceSH(nullptr),
			mPrefilteredEnvMapView(nullptr) {
		}

		// Note that either irradianceMap or irradianceSH should be nullptr
		inline LightProbe(Handle<DG::IBuffer> irradianceSH,
			Handle<Texture> prefilteredEnvMap) :
			mIrradianceSH(irradianceSH),
			mPrefilteredEnvMap(prefilteredEnvMap) {

			mPrefilteredEnvMapView = mPrefilteredEnvMap ? 
				mPrefilteredEnvMap->GetShaderView() : nullptr;
		}

		static void Serialize(entt::registry* registry, 
			std::ostream& stream,
			const FrameHeader& header,
			const SerializationSet& subset);
		static void Deserialize(entt::registry* registry, 
			std::istream& stream,
			const FrameHeader& header);
		static void BuildResourceTable(entt::registry* registry,
			FrameHeader& header,
			const SerializationSet& subset);
	};
}