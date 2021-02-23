#include <Engine/Pipelines/ImageBasedLightingView.hpp>
#include <Engine/LightProbe.hpp>

namespace Morpheus {
	ImageBasedLightingView::ImageBasedLightingView(
		PipelineResource* pipeline) {

		auto& srbs = pipeline->GetShaderResourceBindings();

		if (srbs[0]->GetVariableByName(
			DG::SHADER_TYPE_PIXEL, "mIrradianceMap") != nullptr) {
			bUseSH = false;
		} else if (srbs[0]->GetVariableByName(
			DG::SHADER_TYPE_PIXEL, "IrradianceSH") != nullptr) {
			bUseSH = true;
		} else {
			assert(true);
		}

		for (auto srb : srbs) {
			if (!bUseSH)
				mIrradianceMapLoc.emplace_back(
					srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mIrradianceMap"));
			else 
				mIrradianceSHLoc.emplace_back(
					srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "IrradianceSH"));
		
			mPrefilteredEnvMapLoc.emplace_back(
				srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mPrefilteredEnvMap"));
			
		}
	}

	void ImageBasedLightingView::SetEnvironment(
		DG::ITextureView* irradiance,
		DG::IBuffer* irradianceMapSH,
		DG::ITextureView* prefilteredEnvMap, 
		uint srbId) {
		if (bUseSH) {
			mIrradianceSHLoc[srbId]->Set(irradianceMapSH);
		} else {
			mIrradianceMapLoc[srbId]->Set(irradiance);
		}
		mPrefilteredEnvMapLoc[srbId]->Set(prefilteredEnvMap);
	}

	void ImageBasedLightingView::SetEnvironment(
		LightProbe* lightProbe, uint srbId) {
		
		DG::ITextureView* prefilteredEnvMap = lightProbe->GetPrefilteredEnvView();

		if (bUseSH) {
			DG::IBuffer* irradianceSH = nullptr;
			irradianceSH = lightProbe->GetIrradianceSH();
			mIrradianceSHLoc[srbId]->Set(irradianceSH);
		} else {
			DG::ITextureView* irradiance = nullptr;
			irradiance = lightProbe->GetIrradianceMap()->GetShaderView();
			mIrradianceMapLoc[srbId]->Set(irradiance);
		}

		mPrefilteredEnvMapLoc[srbId]->Set(prefilteredEnvMap);
	}
}