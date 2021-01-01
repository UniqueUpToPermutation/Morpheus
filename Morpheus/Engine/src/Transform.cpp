#include <Engine/Transform.hpp>

namespace Morpheus {
	void Transform::UpdateCache(Transform* parent) {
		UpdateCache();
		if (parent) {
			mCachedTransform = mCachedTransform * parent->GetCache();
		}
	}

	void Transform::UpdateCache() {
		mCachedTransform = mRotation.ToMatrix();
		mCachedTransform.m30 = mTranslation.x;
		mCachedTransform.m31 = mTranslation.y;
		mCachedTransform.m32 = mTranslation.z;

		mCachedTransform.m00 *= mScale.x;
		mCachedTransform.m01 *= mScale.x;
		mCachedTransform.m02 *= mScale.x;

		mCachedTransform.m10 *= mScale.y;
		mCachedTransform.m11 *= mScale.y;
		mCachedTransform.m12 *= mScale.y;

		mCachedTransform.m20 *= mScale.z;
		mCachedTransform.m21 *= mScale.z;
		mCachedTransform.m22 *= mScale.z;
	}

	void Transform::UpdateCacheFromMotionState(btMotionState* motionState) {
		btTransform transform;
		motionState->getWorldTransform(transform);
		transform.getOpenGLMatrix(&mCachedTransform.m00);
	}

	Transform::Transform() :
		mTranslation(0.0f, 0.0f, 0.0f),
		mScale(1.0f, 1.0f, 1.0f),
		mRotation(0.0f, 0.0f, 0.0f, 1.0f) {
	}

	Transform::Transform(const DG::float3& translation,
		const DG::float3& scale,
		const DG::Quaternion& rotation) :
			mTranslation(translation),
			mScale(scale),
			mRotation(rotation) {
	}
}