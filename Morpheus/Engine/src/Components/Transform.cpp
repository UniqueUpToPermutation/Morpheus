#include <Engine/Components/Transform.hpp>

#include <cereal/archives/portable_binary.hpp>

#ifdef USE_BULLET
#include "btBulletDynamicsCommon.h"
#endif

using namespace entt;

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

#ifdef USE_BULLET
	void Transform::ToBullet(btTransform& output) const {
		output.setFromOpenGLMatrix(ToMatrix().Data());
	}

	btTransform Transform::ToBullet() const {
		btTransform output;
		output.setFromOpenGLMatrix(ToMatrix().Data());
		return output;
	}

#endif

	void Transform::RegisterMetaData() {
		meta<Transform>()
			.type("Transform"_hs);
	}

	void Transform::Serialize(Transform& transform,
		cereal::PortableBinaryOutputArchive& archive,
		IDependencyResolver* dependencies) {
		transform.serialize(archive);
	}

	Transform Transform::Deserialize(
		cereal::PortableBinaryInputArchive& archive,
		const IDependencyResolver* dependices) {
		Transform transform;
		transform.serialize(archive);
		return transform;
	}
}