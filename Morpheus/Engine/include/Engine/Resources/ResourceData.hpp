#pragma once

#include "BasicTypes.h"
#include "GraphicsTypes.h"
#include <Engine/Defines.hpp>

namespace DG = Diligent;

namespace Morpheus {
	uint GetTypeSize(DG::VALUE_TYPE type);
	DG::VALUE_TYPE GetComponentType(DG::TEXTURE_FORMAT texFormat);
	int GetComponentCount(DG::TEXTURE_FORMAT texFormat);
	int GetPixelByteSize(DG::TEXTURE_FORMAT format);
	bool GetIsNormalized(DG::TEXTURE_FORMAT format);
	bool GetIsSRGB(DG::TEXTURE_FORMAT format);
}