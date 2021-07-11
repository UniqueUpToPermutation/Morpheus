#include <Engine/Entity.hpp>
#include <Engine/Camera.hpp>
#include <Engine/Components/Transform.hpp>
#include <Engine/Graphics.hpp>
#include <Engine/RendererTransformCache.hpp>

#include "SwapChain.h"

namespace Morpheus {

	HLSL::CameraAttribs CreateCameraAttribs(const DG::float2& viewportSize,
		const DG::float4x4& view, const DG::float4x4& projection,
		const DG::float4& eye, float nearPlane, float farPlane) {

		HLSL::CameraAttribs attribs;
		attribs.fNearPlaneZ = nearPlane;
		attribs.fFarPlaneZ = farPlane;

		attribs.f4Position = eye;
		attribs.f4ViewportSize = DG::float4(
			viewportSize.x,
			viewportSize.y,
			1.0f / viewportSize.x,
			1.0f / viewportSize.y);

		attribs.mViewT = view.Transpose();
		attribs.mProjT = projection.Transpose();
		attribs.mViewProjT = (view * projection).Transpose();
		attribs.mProjInvT = attribs.mProjT.Inverse();
		attribs.mViewInvT = attribs.mViewT.Inverse();
		attribs.mViewProjInvT = attribs.mViewProjT.Inverse();

		return attribs;
	}

	DG::float4x4 GetSurfacePretransformMatrix(DG::ISwapChain* swapChain, 
		const DG::float3& f3CameraViewAxis)
	{
		const auto& SCDesc = swapChain->GetDesc();
		switch (SCDesc.PreTransform)
		{
			case  DG::SURFACE_TRANSFORM_ROTATE_90:
				// The image content is rotated 90 degrees clockwise.
				return DG::float4x4::RotationArbitrary(f3CameraViewAxis, -DG::PI_F / 2.f);

			case  DG::SURFACE_TRANSFORM_ROTATE_180:
				// The image content is rotated 180 degrees clockwise.
				return DG::float4x4::RotationArbitrary(f3CameraViewAxis, -DG::PI_F);

			case  DG::SURFACE_TRANSFORM_ROTATE_270:
				// The image content is rotated 270 degrees clockwise.
				return DG::float4x4::RotationArbitrary(f3CameraViewAxis, -DG::PI_F * 3.f / 2.f);

			case  DG::SURFACE_TRANSFORM_OPTIMAL:
				UNEXPECTED("SURFACE_TRANSFORM_OPTIMAL is only valid as parameter during swap chain initialization.");
				return DG::float4x4::Identity();

			case DG::SURFACE_TRANSFORM_HORIZONTAL_MIRROR:
			case DG::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90:
			case DG::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180:
			case DG::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270:
				UNEXPECTED("Mirror transforms are not supported");
				return DG::float4x4::Identity();

			default:
				return DG::float4x4::Identity();
		}
	}

	DG::float4x4 GetAdjustedProjectionMatrix(DG::ISwapChain* swapChain, 
		bool bIsGL, 
		float FOV, 
		float NearPlane, 
		float FarPlane)
	{
		const auto& SCDesc = swapChain->GetDesc();

		float AspectRatio = static_cast<float>(SCDesc.Width) / static_cast<float>(SCDesc.Height);
		float XScale, YScale;
		if (SCDesc.PreTransform == DG::SURFACE_TRANSFORM_ROTATE_90 ||
			SCDesc.PreTransform == DG::SURFACE_TRANSFORM_ROTATE_270 ||
			SCDesc.PreTransform == DG::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90 ||
			SCDesc.PreTransform == DG::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270)
		{
			// When the screen is rotated, vertical FOV becomes horizontal FOV
			XScale = 1.f / std::tan(FOV / 2.f);
			// Aspect ratio is inversed
			YScale = XScale * AspectRatio;
		}
		else
		{
			YScale = 1.f / std::tan(FOV / 2.f);
			XScale = YScale / AspectRatio;
		}

		DG::float4x4 Proj;
		Proj._11 = XScale;
		Proj._22 = YScale;
		Proj.SetNearFarClipPlanes(NearPlane, FarPlane, bIsGL);
		return Proj;
	}

	DG::float4x4 GetAdjustedOrthoMatrix(
		bool bIsGL,
		const DG::float2& fCameraSize,
		float zNear, float zFar) {
		float XScale = (float)(2.0 / fCameraSize.x);
		float YScale = (float)(2.0 / fCameraSize.y);

		DG::float4x4 Proj;
		Proj._11 = XScale;
		Proj._22 = YScale;

		if (bIsGL)
        {
            Proj._33 = -(-(zFar + zNear) / (zFar - zNear));
            Proj._43 = -2 * zNear * zFar / (zFar - zNear);
        }
        else
        {
            Proj._33 = zFar / (zFar - zNear);
            Proj._43 = -zNear * zFar / (zFar - zNear);
        }

		Proj._44 = 1;

		return Proj;
	}

	void Camera::ComputeTransformations(
		entt::entity entity,
		entt::registry* registry,
		DG::ISwapChain* swapChain,
		bool bIsGL,
		DG::float3* eye,
		DG::float3* lookAt,
		DG::float4x4* view,
		DG::float4x4* proj,
		DG::float4x4* viewProj) {

		assert(this == &registry->get<Camera>(entity));

		auto transform = registry->try_get<RendererTransformCache>(entity);

		*eye = GetEye();
		*lookAt = GetLookAt();
		*view = GetView();
		*proj = GetProjection(swapChain, bIsGL);

		if (transform) {
			auto camera_transform_mat = transform->mCache;
			auto camera_transform_inv = camera_transform_mat.Inverse();
			(*view) = camera_transform_inv * (*view);
			(*eye) = (*eye) * camera_transform_mat;
			(*lookAt) = (*lookAt) * camera_transform_mat;
		}

		*viewProj = (*view) * (*proj);
	}

	void Camera::ComputeTransformations(
		entt::entity entity,
		entt::registry* registry,
		RealtimeGraphics& graphics,
		DG::float3* eye,
		DG::float3* lookAt,
		DG::float4x4* view,
		DG::float4x4* proj,
		DG::float4x4* viewProj) {
		ComputeTransformations(entity, registry, graphics.SwapChain(), graphics.IsGL(),
			eye, lookAt, view, proj, viewProj);
	}

	HLSL::CameraAttribs Camera::GetLocalAttribs(DG::ISwapChain* swapChain, bool bIsGL) {
		auto eye = DG::float4(GetEye(), 1.0f);
		auto view = GetView();
		auto proj = GetProjection(swapChain, bIsGL);

		DG::float2 viewportSize;
		auto& scDesc = swapChain->GetDesc();
		viewportSize.x = scDesc.Width;
		viewportSize.y = scDesc.Height;

		return CreateCameraAttribs(viewportSize, view, proj, eye, mNearPlane, mFarPlane);
	}

	HLSL::CameraAttribs Camera::GetTransformedAttribs(entt::entity entity, entt::registry* registry, 
		DG::ISwapChain* swapChain, bool bIsGL) {
		auto eye3 = GetEye();
		auto view = GetView();
		auto proj = GetProjection(swapChain, bIsGL);

		RendererTransformCache* cameraTransform = nullptr;

		if (entity != entt::null)
			cameraTransform = registry->try_get<RendererTransformCache>(entity);

		if (cameraTransform) {
			auto camera_transform_inv = cameraTransform->mCache.Inverse();
			view = camera_transform_inv * view;
			eye3 = eye3 * (cameraTransform->mCache);
		}

		auto eye = DG::float4(eye3, 1.0f);

		DG::float2 viewportSize;
		auto& scDesc = swapChain->GetDesc();
		viewportSize.x = scDesc.Width;
		viewportSize.y = scDesc.Height;

		return CreateCameraAttribs(viewportSize, view, proj, eye, mNearPlane, mFarPlane);
	}

	HLSL::CameraAttribs Camera::GetLocalAttribs(RealtimeGraphics& graphics) {
		return GetLocalAttribs(graphics.SwapChain(), graphics.IsGL());
	}
	HLSL::CameraAttribs Camera::GetTransformedAttribs(entt::entity entity, entt::registry* registry, RealtimeGraphics& graphics) {
		return GetTransformedAttribs(entity, registry, graphics.SwapChain(), graphics.IsGL());
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

	DG::float4x4 Camera::GetProjection(DG::ISwapChain* swapChain, bool bIsGL) const {
		if (mType == CameraType::PERSPECTIVE) {
			// Get pretransform matrix that rotates the scene according the surface orientation
			auto srfPreTransform = GetSurfacePretransformMatrix(swapChain, DG::float3{0, 0, 1});

			// Get projection matrix adjusted to the current screen orientation
			auto proj = GetAdjustedProjectionMatrix(swapChain, bIsGL, mFieldOfView, mNearPlane, mFarPlane);
			return srfPreTransform * proj;
		} else if (mType == CameraType::ORTHOGRAPHIC) {
			// Get pretransform matrix that rotates the scene according the surface orientation
			auto srfPreTransform = GetSurfacePretransformMatrix(swapChain, DG::float3{0, 0, 1});

			// Get projection matrix adjusted to the current screen orientation
			auto proj = GetAdjustedOrthoMatrix(bIsGL, mOrthoSize, mNearPlane, mFarPlane);
			return srfPreTransform * proj;
		} else {
			throw std::runtime_error("Invalid Camera Type!");
		}
	}

	DG::float4x4 Camera::GetProjection(RealtimeGraphics& graphics) const {
		return GetProjection(graphics.SwapChain(), graphics.IsGL());
	}

	DG::float3 Camera::GetEye() const {
		return mEye;
	}
}