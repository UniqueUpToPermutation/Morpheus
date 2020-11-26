#include <Engine/Camera.hpp>

namespace Morpheus {
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
		assert(mType == CameraType::PERSPECTIVE);

		auto translate = DG::float4x4::Translation(-mEye);
		
		auto zAxis = DG::normalize(mLookAt - mEye);
		auto xAxis = DG::normalize(DG::cross(mUp, zAxis));
		auto yAxis = DG::normalize(DG::cross(zAxis, xAxis));

		auto rotation = DG::float4x4::ViewFromBasis(xAxis, yAxis, zAxis);

		auto view = translate * rotation;
		return view;
	}

	DG::float4x4 Camera::GetProjection(Engine* engine) const {
		// Get pretransform matrix that rotates the scene according the surface orientation
    	auto srfPreTransform = engine->GetSurfacePretransformMatrix(DG::float3{0, 0, 1});

    	// Get projection matrix adjusted to the current screen orientation
    	auto proj = engine->GetAdjustedProjectionMatrix(mFieldOfView, mNearPlane, mFarPlane);
		return srfPreTransform * proj;
	}

	DG::float3 Camera::GetEye() const {
		return mEye;
	}
}