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

	btTransform Transform::ToBullet() const {
		DG::float4x4 mat = ToMatrix();

		btMatrix3x3 btMat(
			mat.m00, mat.m01, mat.m02,
			mat.m10, mat.m11, mat.m12,
			mat.m20, mat.m21, mat.m22);

		btVector3 btTransl(
			mat.m30, 
			mat.m31, 
			mat.m32);

		return btTransform(btMat, btTransl);
	}
}