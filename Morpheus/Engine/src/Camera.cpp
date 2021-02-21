#include <Engine/Camera.hpp>
#include <Engine/Components/Transform.hpp>

namespace Morpheus {
	void Camera::ComputeTransformations(
		EntityNode cameraNode,
		Engine* engine,
		DG::float3* eye,
		DG::float3* lookAt,
		DG::float4x4* view,
		DG::float4x4* proj,
		DG::float4x4* viewProj) {

		auto& camera = cameraNode.Get<Camera>();
		auto transform = cameraNode.TryGet<MatrixTransformCache>();

		*eye = camera.GetEye();
		*lookAt = camera.GetLookAt();
		*view = camera.GetView();
		*proj = camera.GetProjection(engine);

		if (transform) {
			auto camera_transform_mat = transform->mCache;
			auto camera_transform_inv = camera_transform_mat.Inverse();
			(*view) = camera_transform_inv * (*view);
			(*eye) = (*eye) * camera_transform_mat;
			(*lookAt) = (*lookAt) * camera_transform_mat;
		}

		*viewProj = (*view) * (*proj);
	}

	Camera::Camera(CameraType type) :
		mType(type) {	
	}
	Camera::Camera(const Camera& other) :
		mEye(other.mEye),
		mLookAt(other.mLookAt),
		mUp(other.mUp),
		mFieldOfView(other.mFieldOfView),
		mNearPlane(other.mNearPlane),
		mFarPlane(other.mFarPlane),
		mType(other.mType) {
	}

	DG::float4x4 Camera::GetView() const {
		auto translate = DG::float4x4::Translation(-mEye);
		
		auto zAxis = DG::normalize(mLookAt - mEye);
		auto xAxis = DG::normalize(DG::cross(mUp, zAxis));
		auto yAxis = DG::normalize(DG::cross(zAxis, xAxis));

		auto rotation = DG::float4x4::ViewFromBasis(xAxis, yAxis, zAxis);

		auto view = translate * rotation;
		return view;
	}

	DG::float4x4 Camera::GetProjection(Engine* engine) const {
		if (mType == CameraType::PERSPECTIVE) {
			// Get pretransform matrix that rotates the scene according the surface orientation
			auto srfPreTransform = engine->GetSurfacePretransformMatrix(DG::float3{0, 0, 1});

			// Get projection matrix adjusted to the current screen orientation
			auto proj = engine->GetAdjustedProjectionMatrix(mFieldOfView, mNearPlane, mFarPlane);
			return srfPreTransform * proj;
		} else if (mType == CameraType::ORTHOGRAPHIC) {
			// Get pretransform matrix that rotates the scene according the surface orientation
			auto srfPreTransform = engine->GetSurfacePretransformMatrix(DG::float3{0, 0, 1});

			// Get projection matrix adjusted to the current screen orientation
			auto proj = engine->GetAdjustedOrthoMatrix(mOrthoSize, mNearPlane, mFarPlane);
			return srfPreTransform * proj;
		} else {
			throw std::runtime_error("Invalid Camera Type!");
		}
	}

	DG::float3 Camera::GetEye() const {
		return mEye;
	}
}