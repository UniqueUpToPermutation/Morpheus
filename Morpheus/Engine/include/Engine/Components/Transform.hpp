#pragma once

#include "BasicMath.hpp"

#include <Engine/Entity.hpp>

namespace DG = Diligent;

struct btTransform;

namespace Morpheus {
	class Transform {
	private:
		DG::float3 mTranslation;
		DG::float3 mScale;
		DG::Quaternion mRotation;

	public:
		inline Transform() : 
			mTranslation(0.0f, 0.0f, 0.0f),
			mScale(1.0f, 1.0f, 1.0f),
			mRotation(0.0f, 0.0f, 0.0f, 1.0f) {
		}

		Transform(DG::float3 translation) :
			mTranslation(translation),
			mScale(1.0f, 1.0f, 1.0f),
			mRotation(0.0f, 0.0f, 0.0f, 1.0f) {
		}
			
		Transform(DG::float3 translation,
			DG::Quaternion rotation) : 
			mTranslation(translation),
			mScale(1.0f, 1.0f, 1.0f),
			mRotation(0.0f, 0.0f, 0.0f, 1.0f) {
		}

		Transform(DG::float3 translation,
			DG::Quaternion rotation,
			DG::float3 scale) :
			mTranslation(translation),
			mScale(scale),
			mRotation(rotation) {
		}

		inline Transform& SetTranslation(const DG::float3& t) {
			mTranslation = t;
			return *this;
		}

		inline Transform& SetTranslation(const float x, const float y, const float z) {
			SetTranslation(DG::float3(x, y, z));
			return *this;
		}

		inline Transform& SetRotation(const DG::Quaternion& q) {
			mRotation = q;
			return *this;
		}

		inline Transform& SetScale(const DG::float3& s) {
			mScale = s;
			return *this;
		}

		inline Transform& SetScale(const float x, const float y, const float z) {
			SetScale(DG::float3(x, y, z));
			return *this;
		}

		inline Transform& SetScale(const float s) {
			SetScale(DG::float3(s, s, s));
			return *this;
		}

		inline DG::float3 GetTranslation() const {
			return mTranslation;
		}

		inline DG::float3 GetScale() const {
			return mScale;
		}

		inline DG::Quaternion GetRotation() const {
			return mRotation;
		}

		DG::float4x4 ToMatrix() const;

		void ToBullet(btTransform& output) const;
	};
}