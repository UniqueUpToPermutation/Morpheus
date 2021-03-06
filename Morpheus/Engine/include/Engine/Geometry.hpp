#pragma once

#include "BasicMath.hpp"
#include "InputLayout.h"

namespace DG = Diligent;

namespace Morpheus {
	struct BoundingBox {
		DG::float3 mLower;
		DG::float3 mUpper;
	};

	struct VertexLayout {
	public:
		std::vector<DG::LayoutElement> mElements;

		int mPosition	= -1;
		int mUV			= -1;
		int mNormal 	= -1;
		int mTangent 	= -1;
		int mBitangent	= -1;

		// If this is -1, then assume dense packing
		int mStride 	= -1; 
	};
}