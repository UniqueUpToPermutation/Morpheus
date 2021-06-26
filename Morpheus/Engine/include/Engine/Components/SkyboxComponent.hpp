#pragma once

#include <Engine/Resources/Texture.hpp>

namespace DG = Diligent;

namespace Morpheus {

	struct SkyboxComponent {
		Handle<Texture> mCubemap;

		inline SkyboxComponent() {
		}

		inline SkyboxComponent(const Handle<Texture>& cubemap) : mCubemap(cubemap) {
		}
	};
}