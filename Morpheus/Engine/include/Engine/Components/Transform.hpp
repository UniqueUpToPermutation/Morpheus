#pragma once

#include "BasicMath.hpp"

#include "btBulletDynamicsCommon.h"

#include <Engine/Entity.hpp>
#include <Engine/Scene.hpp>

namespace DG = Diligent;

namespace Morpheus {
	class MatrixTransformCache {
	public:
		DG::float4x4 mCache;

		MatrixTransformCache(const DG::float4x4& cache) :
			mCache(cache) {
		}
	};

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

		inline void SetTranslation(const DG::float3& t) {
			mTranslation = t;
		}

		inline void SetTranslation(const float x, const float y, const float z) {
			SetTranslation(DG::float3(x, y, z));
		}

		inline void SetRotation(const DG::Quaternion& q) {
			mRotation = q;
		}

		inline void SetScale(const DG::float3& s) {
			mScale = s;
		}

		inline void SetScale(const float x, const float y, const float z) {
			SetScale(DG::float3(x, y, z));
		}

		inline void SetScale(const float s) {
			SetScale(DG::float3(s, s, s));
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
	};
}