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

		// Inform other components that the transform was updated externally
		if (mOnUpdateDelegate)
			mOnUpdateDelegate(this);
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

	// Update the update the transform externally, update delegate is called to propagate the update to physics
	void Transform::SetTransform(EntityNode self, 
		const DG::float3& translation,
		const DG::float3& scale,
		const DG::Quaternion& rotation,
		bool bUpdateDescendants) {

		mTranslation = translation;
		mScale = scale;
		mRotation = rotation;

		auto parent = FindParent(self);

		if (bUpdateDescendants) {
			UpdateDescendantCaches(self, parent);
		} else {
			UpdateCache(parent);
		}
	}

	// Update the update the transform externally, update delegate is called to propagate the update to physics
	void Transform::SetTranslation(EntityNode self, const DG::float3& translation, bool bUpdateDescendants) {
		SetTransform(self, translation, mScale, mRotation, bUpdateDescendants);
	}

	// Update the update the transform externally, update delegate is called to propagate the update to physics
	void Transform::SetScale(EntityNode self, const DG::float3& scale, bool bUpdateDescendants) {
		SetTransform(self, mTranslation, scale, mRotation, bUpdateDescendants);
	}

	// Update the update the transform externally, update delegate is called to propagate the update to physics
	void Transform::SetRotation(EntityNode self, const DG::Quaternion& rotation, bool bUpdateDescendants) {
		SetTransform(self, mTranslation, mScale, rotation, bUpdateDescendants);
	}

	Transform* Transform::FindParent(EntityNode self) {
		for (EntityNode node = self.GetParent(); node.IsValid(); node = node.GetParent()) {
			auto transform = node.TryGetComponent<Transform>();
			if (transform) {
				return transform;
			}
		}

		return nullptr;
	}

	// Updates the transforms of all descendants of this node, including self
	void Transform::UpdateDescendantCaches(EntityNode self, Transform* parent) {
		DepthFirstNodeDoubleIterator iterator(self);
		
		std::stack<Transform*> transformStack;
		transformStack.emplace(parent);

		for (; iterator; ++iterator) {
			if (iterator.GetDirection() == IteratorDirection::DOWN) {
				auto child = iterator();
				auto transform = child.TryGetComponent<Transform>();
				if (transform) {
					transform->UpdateCache(transformStack.top());
					transformStack.emplace(transform);
				}
			}
			else {
				transformStack.pop();
			}
		}
	}
}