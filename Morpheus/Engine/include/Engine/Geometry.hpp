#pragma once

#include "BasicMath.hpp"

namespace DG = Diligent;

namespace Morpheus {
	struct BoundingBox {
		DG::float3 mLower;
		DG::float3 mUpper;
	};
}