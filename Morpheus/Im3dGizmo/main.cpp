#include <Engine/Core.hpp>
#include <Engine/Im3d.hpp>

using namespace Morpheus;

Im3d::Vec3 ToIm3d(const DG::float3& f) {
	return Im3d::Vec3(f.x, f.y, f.z);
}

Im3d::Vec2 ToIm3d(const DG::float2& f) {
	return Im3d::Vec2(f.x, f.y);
}

Im3d::Mat4 ToIm3d(const DG::float4x4& f) {
	return Im3d::Mat4(
		f.m00, f.m01, f.m02, f.m03,
		f.m10, f.m11, f.m12, f.m13,
		f.m20, f.m21, f.m22, f.m23,
		f.m30, f.m31, f.m32, f.m33);
}

#if PLATFORM_WIN32
int __stdcall WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd) {
#endif

#if PLATFORM_LINUX
int main(int argc, char** argv) {
#endif

	Engine en;
	en.Startup();

	std::unique_ptr<Im3dRenderer> im3drenderer;
	std::unique_ptr<Im3dGlobalsBuffer> im3dglobals(
		new Im3dGlobalsBuffer(en.GetDevice()));

	{
		Im3dRendererFactory factory;
		factory.Initialize(en.GetDevice(),
			im3dglobals.get(),
			en.GetSwapChain()->GetDesc().ColorBufferFormat,
			en.GetSwapChain()->GetDesc().DepthBufferFormat);
		im3drenderer.reset(new Im3dRenderer(en.GetDevice(), &factory));
	}	
	
	std::unique_ptr<Scene> scene(new Scene());

	auto camera = scene->GetCamera();
	camera->SetEye(1.0, 1.0, 1.0);
	camera->LookAt(0.0, 0.0, 0.0);
	camera->SetClipPlanes(0.1f, 20.0f);

	en.InitializeDefaultSystems(scene.get());
	scene->Begin();

	float translation[] = { 0.0, 0.0, 0.0 };

	while (en.IsReady()) {
		double dt = 0.0;

		en.Update([&dt, &scene](double currTime, double elapsedTime) {
			scene->Update(currTime, elapsedTime);
			dt = elapsedTime;
		});
		
		auto context = en.GetImmediateContext();

		auto swapChain = en.GetSwapChain();
		DG::ITextureView* pRTV = swapChain->GetCurrentBackBufferRTV();
		DG::ITextureView* pDSV = swapChain->GetDepthBufferDSV();

		float rgba[] = { 0.5f, 0.5f, 0.5f, 1.0f };
		context->SetRenderTargets(1, &pRTV, pDSV,
			DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		context->ClearRenderTarget(pRTV, rgba, 
			DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		context->ClearDepthStencil(pDSV, DG::CLEAR_DEPTH_FLAG,
			1.0f, 0, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		DG::float3 eye;
		DG::float3 lookAt;
		DG::float4x4 view;
		DG::float4x4 proj;
		DG::float4x4 viewProj;
		DG::float4x4 viewProjInv;

		Camera::ComputeTransformations(scene->GetCameraNode(),
			&en, &eye, &lookAt, &view, &proj, &viewProj);
		viewProjInv = viewProj.Inverse();

		auto& ad = Im3d::GetAppData();

		auto mouseState = en.GetInputController().GetMouseState();
		auto& scDesc = swapChain->GetDesc();

		DG::float2 viewportSize((float)scDesc.Width, (float)scDesc.Height);

		ad.m_deltaTime     = dt;
		ad.m_viewportSize  = ToIm3d(viewportSize);
		ad.m_viewOrigin    = ToIm3d(eye); // for VR use the head position
		ad.m_viewDirection = ToIm3d(lookAt - eye);
		ad.m_worldUp       = Im3d::Vec3(0.0f, 1.0f, 0.0f); // used internally for generating orthonormal bases
		ad.m_projOrtho     = false;

		// m_projScaleY controls how gizmos are scaled in world space to maintain a constant screen height
		ad.m_projScaleY = ad.m_projOrtho 
			? 2.0f / proj.m11 :
			tanf(camera->GetFieldOfView() * 0.5f) * 2.0f; // or vertical fov for a perspective projection

		// World space cursor ray from mouse position; for VR this might be the position/orientation of the HMD or a tracked controller.
		DG::float2 cursorPos(mouseState.PosX, mouseState.PosY);
		cursorPos = 2.0f * cursorPos / viewportSize - DG::float2(1.0f, 1.0f);
		cursorPos.y = -cursorPos.y; // window origin is top-left, ndc is bottom-left

		DG::float4 rayDir = DG::float4(cursorPos.x, cursorPos.y, -1.0f, 1.0f) * viewProjInv; 
		rayDir = rayDir / rayDir.w;
		DG::float3 rayDirection = DG::float3(rayDir.x, rayDir.y, rayDir.z) - eye;
		rayDirection = DG::normalize(rayDirection);

		ad.m_cursorRayOrigin = ToIm3d(eye);
		ad.m_cursorRayDirection = ToIm3d(rayDirection);

		// Set cull frustum planes. This is only required if IM3D_CULL_GIZMOS or IM3D_CULL_PRIMTIIVES is enable via
		// im3d_config.h, or if any of the IsVisible() functions are called.
		ad.setCullFrustum(ToIm3d(viewProj), true);

		// Fill the key state array; using GetAsyncKeyState here but this could equally well be done via the window proc.
		// All key states have an equivalent (and more descriptive) 'Action_' enum.
		ad.m_keyDown[Im3d::Mouse_Left] = mouseState.ButtonFlags & mouseState.BUTTON_FLAG_LEFT;

		// Enable gizmo snapping by setting the translation/rotation/scale increments to be > 0
		ad.m_snapTranslation = 0.0f;
		ad.m_snapRotation    = 0.0f;
		ad.m_snapScale       = 0.0f;

		Im3d::NewFrame();
		Im3d::GizmoTranslation("Gizmo", translation);
		Im3d::EndFrame();

		im3dglobals->Write(context, scene->GetCameraNode(), &en);
		im3drenderer->Draw(context);

		en.RenderUI();
		en.Present();
	}

	im3drenderer.reset();
	im3dglobals.reset();
	scene.reset();

	en.Shutdown();
}