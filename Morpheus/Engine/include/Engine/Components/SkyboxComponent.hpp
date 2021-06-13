#pragma once

#include <Engine/Resources/Texture.hpp>

namespace DG = Diligent;

namespace Morpheus {

	struct SkyboxComponent {
		Handle<Texture> mCubemap;
	};
}