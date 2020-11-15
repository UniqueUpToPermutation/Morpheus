#pragma once

#include "BasicMath.hpp"

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

		Transform();

		inline DG::float4x4 GetCache() {
			return mCachedTransform;
		}
	};
}