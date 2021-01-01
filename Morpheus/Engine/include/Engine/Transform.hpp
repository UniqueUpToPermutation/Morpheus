#pragma once

#include "BasicMath.hpp"

#include "btBulletDynamicsCommon.h"

namespace DG = Diligent;

namespace Morpheus {
	class Transform {
	public:
		DG::float3 mTranslation;
		DG::float3 mScale;
		DG::Quaternion mRotation;

	private:
		DG::float4x4 mCachedTransform;

	public:
		void UpdateCache(Transform* parent);
		void UpdateCache();
		void UpdateCacheFromMotionState(btMotionState* motionState);

		Transform();
		Transform(const DG::float3& translation,
			const DG::float3& scale,
			const DG::Quaternion& rotation);
			
		inline DG::float4x4 GetCache() {
			return mCachedTransform;
		}
	};
}