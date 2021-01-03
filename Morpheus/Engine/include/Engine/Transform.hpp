#pragma once

#include "BasicMath.hpp"

#include "btBulletDynamicsCommon.h"

#include <entt/entt.hpp>

#include <Engine/SceneHeirarchy.hpp>

namespace DG = Diligent;

namespace Morpheus {
	class Transform {
	private:
		DG::float3 mTranslation;
		DG::float3 mScale;
		DG::Quaternion mRotation;
		entt::delegate<void(Transform*)> mOnUpdateDelegate;
		DG::float4x4 mCachedTransform;

	public:
		// Used to update the cache externally, update delegate is called to propagate the update to physics
		void UpdateCache(Transform* parent);
		// Used to update the cache externally, update delegate is called to propagate the update to physics
		void UpdateCache();

		// Used to update the cache internally from physics, update delegate is not called
		void UpdateCacheFromMotionState(btMotionState* motionState);

		// Finds the parent transform of this entity in the scene tree
		static Transform* FindParent(EntityNode self);

		// Update the update the transform externally, update delegate is called to propagate the update to physics
		void SetTransform(EntityNode self, 
			const DG::float3& translation,
			const DG::float3& scale,
			const DG::Quaternion& rotation,
			bool bUpdateDescendants = true);

		// Update the update the transform externally, update delegate is called to propagate the update to physics
		void SetTranslation(EntityNode self, const DG::float3& translation, bool bUpdateDescendants = true);
		inline void SetTranslation(EntityNode self, float x, float y, float z, bool bUpdateDescendants = true) {
			SetTranslation(self, DG::float3(x, y, z), bUpdateDescendants);
		}

		// Update the update the transform externally, update delegate is called to propagate the update to physics
		void SetScale(EntityNode self, const DG::float3& scale, bool bUpdateDescendants = true);
		inline void SetScale(EntityNode self, float x, float y, float z, bool bUpdateDescendants = true) {
			SetScale(self, DG::float3(x, y, z), bUpdateDescendants);
		}
		inline void SetScale(EntityNode self, float s, bool bUpdateDescendants = true) {
			SetScale(self, DG::float3(s, s, s), bUpdateDescendants);
		}

		// Update the update the transform externally, update delegate is called to propagate the update to physics
		void SetRotation(EntityNode self, const DG::Quaternion& rotation, bool bUpdateDescendants = true);
		inline void SetRotation(EntityNode self, const DG::float3& rotationAxis, float angle, bool bUpdateDescendants) {
			SetRotation(self, DG::Quaternion::RotationFromAxisAngle(rotationAxis, angle), bUpdateDescendants);
		}

		// Updates the transforms of all descendants of this node, including self
		void UpdateDescendantCaches(EntityNode self, Transform* parent = nullptr);

		inline DG::float3 GetTranslation() const {
			return mTranslation;
		}

		inline DG::float3 GetScale() const {
			return mScale;
		}

		inline DG::Quaternion GetRotation() const {
			return mRotation;
		}

		Transform();
		Transform(const DG::float3& translation,
			const DG::float3& scale,
			const DG::Quaternion& rotation);
			
		inline DG::float4x4 GetCache() {
			return mCachedTransform;
		}

		template <auto Func, typename T>
		inline void AttachToUpdateDelegate(T* t) {
			mOnUpdateDelegate.connect<Func>(*t);
		}

		inline void ResetUpdateDelegate() {
			mOnUpdateDelegate.reset();
		}
	};
}