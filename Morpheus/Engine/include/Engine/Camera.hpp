#pragma once

#include <Engine/Engine.hpp>

namespace DG = Diligent;

namespace Morpheus {

	enum class CameraType {
		PERSPECTIVE,
		ORTHOGRAPHIC
	};

	class Camera {
	private:
		DG::float3 mEye = DG::float3{0.0f, 0.0f, 0.0f};
		DG::float3 mLookAt = DG::float3{0.0f, 0.0f, 1.0f};
		DG::float3 mUp = DG::float3{0.0f, 1.0f, 0.0f};

		float mFieldOfView = DG::PI_F / 4.0f;
		float mNearPlane = 0.1f;
		float mFarPlane = 100.0f;

		DG::float2 mOrthoSize = DG::float2{0.0f, 0.0f};

		CameraType mType;

	public:
		Camera(CameraType type = CameraType::PERSPECTIVE);
		Camera(const Camera& other);

		DG::float4x4 GetView() const;
		DG::float4x4 GetProjection(Engine* engine) const;
		DG::float3 GetEye() const;

		inline void SetEye(const DG::float3& eye) {
			mEye = eye;
		}

		inline void SetEye(float x, float y, float z) {
			mEye.x = x;
			mEye.y = y;
			mEye.z = z;
		}

		inline void LookAt(const DG::float3& target) { 
			mLookAt = target;
		}

		inline void LookAt(float x, float y, float z) {
			mEye.x = x;
			mEye.y = y;
			mEye.z = z;
		}

		inline float GetFieldOfView() const {
			return mFieldOfView;
		}

		inline void SetFieldOfView(float value) {
			mFieldOfView = value;
		}

		inline CameraType GetType() const {
			return mType;
		}

		inline void SetType(CameraType type) {
			mType = type;
		}

		inline void SetOrthoSize(const DG::float2& size) {
			mOrthoSize = size;
		}

		inline void SetOrthoSize(float width, float height) {
			mOrthoSize.x = width;
			mOrthoSize.y = height;
		}

		inline void SetClipPlanes(float fNear, float fFar) {
			mNearPlane = fNear;
			mFarPlane = fFar;
		}

		inline DG::float3 GetLookAt() const {
			return mLookAt;
		}

		inline float GetFarZ() const { 
			return mFarPlane;
		}

		inline float GetNearZ() const {
			return mNearPlane;
		}

		inline void SetFarPlane(const float z) {
			mFarPlane = z;
		}

		inline void SetNearPlane(const float z) {
			mNearPlane = z;
		}
	};
}