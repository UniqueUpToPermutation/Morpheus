#include <Engine/Camera.hpp>

namespace Morpheus {
	DG::float4x4 PerspectiveLookAtCamera::GetView() const {
		auto translate = DG::float4x4::Translation(-mEye);
		
		auto zAxis = DG::normalize(mLookAt - mEye);
		auto xAxis = DG::normalize(DG::cross(mUp, zAxis));
		auto yAxis = DG::normalize(DG::cross(zAxis, xAxis));

		auto rotation = DG::float4x4::ViewFromBasis(xAxis, yAxis, zAxis);

		auto view = translate * rotation;
		return view;
	}

	DG::float4x4 PerspectiveLookAtCamera::GetProjection(Engine* engine) const {
		// Get pretransform matrix that rotates the scene according the surface orientation
    	auto srfPreTransform = engine->GetSurfacePretransformMatrix(DG::float3{0, 0, 1});

    	// Get projection matrix adjusted to the current screen orientation
    	auto proj = engine->GetAdjustedProjectionMatrix(mFieldOfView, mNearPlane, mFarPlane);
		return srfPreTransform * proj;
	}

	DG::float3 PerspectiveLookAtCamera::GetEye() const {
		return mEye;
	}
}