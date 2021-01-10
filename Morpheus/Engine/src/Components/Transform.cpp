#include <Engine/Components/Transform.hpp>

namespace Morpheus {
	DG::float4x4 Transform::ToMatrix() const {
		DG::float4x4 result = mRotation.ToMatrix();

		result.m30 = mTranslation.x;
		result.m31 = mTranslation.y;
		result.m32 = mTranslation.z;

		result.m00 *= mScale.x;
		result.m01 *= mScale.x;
		result.m02 *= mScale.x;

		result.m10 *= mScale.y;
		result.m11 *= mScale.y;
		result.m12 *= mScale.y;

		result.m20 *= mScale.z;
		result.m21 *= mScale.z;
		result.m22 *= mScale.z;

		return result;
	}
}