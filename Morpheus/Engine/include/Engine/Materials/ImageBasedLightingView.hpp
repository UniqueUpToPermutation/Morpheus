#pragma once

#include "EngineFactory.h"
#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "BasicMath.hpp"

#include <Engine/Materials/MaterialView.hpp>

namespace DG = Diligent;

namespace Morpheus {
class MaterialResource;
	class LightProbe;

	class ImageBasedLightingView {
	private:
		DG::IShaderResourceVariable* mIrradianceMapLoc;
		DG::IShaderResourceVariable* mPrefilteredEnvMapLoc;
		DG::IShaderResourceVariable* mIrradianceSHLoc;
	
	public:
		ImageBasedLightingView(
			DG::IShaderResourceVariable* irradianceMapLoc,
			DG::IShaderResourceVariable* irradianceMapSHLoc,
			DG::IShaderResourceVariable* prefilteredEnvMapLoc);

		void SetEnvironment(
			DG::ITextureView* irradiance,
			DG::IBufferView* irradianceMapSH,
			DG::ITextureView* prefilteredEnvMap);

		void SetEnvironment(
			LightProbe* lightProbe);
	};
}