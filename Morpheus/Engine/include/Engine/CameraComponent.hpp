#pragma once

#include <Engine/Camera.hpp>

namespace Morpheus {
	class CameraComponent {
	private:
		std::shared_ptr<Camera> mCamera;

	public:
		inline Camera* GetCamera() {
			return mCamera.get();
		}

		void SetPerspectiveLookAt(const DG::float3& eye, const DG::float3& lookAt, const DG::float3& up,
			float fieldOfView = DG::PI_F / 4.0f,
			float nearPlane = 0.1f,
			float farPlane = 100.0f) {

			auto newCamera = new PerspectiveLookAtCamera();
			newCamera->mEye = eye;
			newCamera->mLookAt = lookAt;
			newCamera->mUp = up;
			newCamera->mFieldOfView = fieldOfView;
			newCamera->mNearPlane = nearPlane;
			newCamera->mFarPlane = farPlane;

			mCamera.reset(newCamera);
		}
	};
}