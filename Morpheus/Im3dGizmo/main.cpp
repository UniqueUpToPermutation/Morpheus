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

MAIN() {

	Platform platform;
	platform.Startup();

	Graphics graphics(platform);
	graphics.Startup();

	EmbeddedFileLoader fileSystem;

	{
		// Create the Im3d renderer and state
		Im3dGlobalsBuffer im3dGlobals(graphics);
		Im3dShaders im3dShaders = Im3dShaders::LoadDefault(graphics, &fileSystem)();
		Im3dPipeline im3dPipeline(graphics, &im3dGlobals, im3dShaders);
		Im3dRenderer im3dRenderer(graphics);
	
		Camera camera;
		camera.SetEye(1.0, 1.0, 1.0);
		camera.LookAt(0.0, 0.0, 0.0);
		camera.SetClipPlanes(0.1f, 20.0f);

		float translation[] = { 0.0, 0.0, 0.0 };

		DG::Timer timer;
		FrameTime time(timer);

		while (platform.IsValid()) {

			platform.MessagePump();
			time.UpdateFrom(timer);

			auto context = graphics.ImmediateContext();

			auto swapChain = graphics.SwapChain();
			DG::ITextureView* pRTV = swapChain->GetCurrentBackBufferRTV();
			DG::ITextureView* pDSV = swapChain->GetDepthBufferDSV();

			float rgba[] = { 0.3f, 0.3f, 0.3f, 1.0f };
			context->SetRenderTargets(1, &pRTV, pDSV,
				DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context->ClearRenderTarget(pRTV, rgba, 
				DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context->ClearDepthStencil(pDSV, DG::CLEAR_DEPTH_FLAG,
				1.0f, 0, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			DG::float3 eye = camera.GetEye();
			DG::float3 lookAt = camera.GetLookAt();
			DG::float4x4 view = camera.GetView();
			DG::float4x4 proj = camera.GetProjection(graphics);
			DG::float4x4 viewProj = view * proj;
			DG::float4x4 viewProjInv = viewProj.Inverse();

			auto& ad = Im3d::GetAppData();

			auto mouseState = platform.GetInput().GetMouseState();
			auto& scDesc = swapChain->GetDesc();

			DG::float2 viewportSize((float)scDesc.Width, (float)scDesc.Height);

			ad.m_deltaTime     = time.mElapsedTime;
			ad.m_viewportSize  = ToIm3d(viewportSize);
			ad.m_viewOrigin    = ToIm3d(eye); // for VR use the head position
			ad.m_viewDirection = ToIm3d(lookAt - eye);
			ad.m_worldUp       = Im3d::Vec3(0.0f, 1.0f, 0.0f); // used internally for generating orthonormal bases
			ad.m_projOrtho     = false;

			// m_projScaleY controls how gizmos are scaled in world space to maintain a constant screen height
			ad.m_projScaleY = ad.m_projOrtho 
				? 2.0f / proj.m11 :
				tanf(camera.GetFieldOfView() * 0.5f) * 2.0f; // or vertical fov for a perspective projection

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

			im3dGlobals.WriteWithoutTransformCache(context, graphics, camera);
			im3dRenderer.Draw(context, im3dPipeline);

			graphics.Present(1);
		}
	}

	graphics.Shutdown();
	platform.Shutdown();
}