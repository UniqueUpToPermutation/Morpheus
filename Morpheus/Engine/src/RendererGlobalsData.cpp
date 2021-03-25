#include <Engine/Camera.hpp>
#include <Engine/LightProbe.hpp>
#include <Engine/RendererGlobalsData.hpp>

namespace Morpheus {
	void WriteRenderGlobalsData(
		DynamicGlobalsBuffer<RendererGlobalData>* globals,
		DG::IDeviceContext* context,
		const DG::float2& viewportSize,
		Camera* camera,
		const DG::float4x4& projectionMatrix,
		const DG::float4x4* cameraTransform,
		LightProbe* globalLightProbe) {

		float4x4 view = camera->GetView();
		float4x4 projection = projectionMatrix;
		float3 eye = camera->GetEye();

		if (cameraTransform) {
			auto camera_transform_inv = cameraTransform->Inverse();
			view = camera_transform_inv * view;
			eye = eye * (*cameraTransform);
		}

		RendererGlobalData data;
		data.mCamera.fNearPlaneZ = camera->GetNearZ();
		data.mCamera.fFarPlaneZ = camera->GetFarZ();
		data.mCamera.f4Position = DG::float4(eye, 1.0f);
		data.mCamera.f4ViewportSize = DG::float4(
			viewportSize.x,
			viewportSize.y,
			1.0f / viewportSize.x,
			1.0f / viewportSize.y);
		data.mCamera.mViewT = view.Transpose();
		data.mCamera.mProjT = projection.Transpose();
		data.mCamera.mViewProjT = (view * projection).Transpose();
		data.mCamera.mProjInvT = data.mCamera.mProjT.Inverse();
		data.mCamera.mViewInvT = data.mCamera.mViewT.Inverse();
		data.mCamera.mViewProjInvT = data.mCamera.mViewProjT.Inverse();

		if (globalLightProbe)
			data.mGlobalLighting.fGlobalEnvMapLevels = 
				(float)globalLightProbe->GetPrefilteredEnvMap()->GetTexture()->GetDesc().MipLevels;
		else 
			data.mGlobalLighting.fGlobalEnvMapLevels = 0;

		globals->Write(context, data);
	}
}