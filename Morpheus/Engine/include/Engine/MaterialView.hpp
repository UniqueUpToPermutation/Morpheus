#pragma once

#include "EngineFactory.h"
#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "InputController.hpp"
#include "BasicMath.hpp"

#include <nlohmann/json.hpp>

namespace DG = Diligent;

namespace Morpheus {
	class MaterialResource;

	class ImageBasedLightingView {
	private:
		DG::IShaderResourceVariable* mIrradianceMapLoc;
		DG::IShaderResourceVariable* mPrefilteredEnvMapLoc;
	
	public:
		ImageBasedLightingView(MaterialResource* material,
			const nlohmann::json& config);

		void SetEnvironment(
			DG::ITextureView* irradiance,
			DG::ITextureView* prefilteredEnvMap);
	};
}