#include <Engine/Core.hpp>
#include <Engine/Im3d.hpp>

using namespace Morpheus;

MAIN() {
	Meta::Initialize();

	Platform platform;
	platform.Startup();

	RealtimeGraphics graphics(platform);
	graphics.Startup();

	EmbeddedFileLoader fileSystem;

	{
		// Create the Im3d renderer and state
		Im3dGlobalsBuffer im3dGlobals(graphics);
		Im3dShaders im3dShaders = Im3dShaders::LoadDefault(graphics, &fileSystem).Evaluate();
		Im3dPipeline im3dPipeline(graphics, &im3dGlobals, im3dShaders);
		Im3dRenderer im3dRenderer(graphics);

		// Camera setup
		Camera camera;
		camera.SetType(CameraType::ORTHOGRAPHIC);
		camera.SetOrthoSize(2.0f, 2.0f);
		camera.SetClipPlanes(-1.0f, 1.0f);
		camera.SetEye(0.0, 0.0, 0.0);

		while (platform.IsValid()) {
			
			platform.MessagePump();
			
			auto context = graphics.ImmediateContext();
			auto swapChain = graphics.SwapChain();

			DG::ITextureView* pRTV = swapChain->GetCurrentBackBufferRTV();
			DG::ITextureView* pDSV = swapChain->GetDepthBufferDSV();

			// Clear screen
			float rgba[] = { 0.5f, 0.5f, 0.5f, 1.0f };
			context->SetRenderTargets(1, &pRTV, pDSV,
				DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context->ClearRenderTarget(pRTV, rgba, 
				DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context->ClearDepthStencil(pDSV, DG::CLEAR_DEPTH_FLAG,
				1.0f, 0, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			// Render Im3d Test
			Im3d::NewFrame();

			Im3d::BeginTriangles();
			Im3d::Vertex(-0.75f, -0.75f, 0.0f, Im3d::Color_Blue);
			Im3d::Vertex(-0.5f, -0.25f, 0.0f, Im3d::Color_Green);
			Im3d::Vertex(-0.25f, -0.75f, 0.0f, Im3d::Color_Red);
			Im3d::End();

			Im3d::BeginLineLoop();
			Im3d::Vertex(-0.75f, 0.25f, 0.0f, 4.0f, Im3d::Color_Blue);
			Im3d::Vertex(-0.5f, 0.75f, 0.0f, 4.0f, Im3d::Color_Green);
			Im3d::Vertex(-0.25f, 0.25f, 0.0f, 4.0f, Im3d::Color_Red);
			Im3d::End();

			Im3d::DrawPoint(Im3d::Vec3(0.5f, 0.5f, 0.0f), 
				50.0f, Im3d::Color_Black);

			Im3d::DrawCircleFilled(Im3d::Vec3(0.5f, -0.5f, 0.0f), 
				Im3d::Vec3(0.0f, 0.0f, -1.0f), 0.25f);

			Im3d::EndFrame();

			// Write the camera data to GPU
			im3dGlobals.WriteWithoutTransformCache(context, graphics, camera);
			// Draw everything submitted to the Im3d queue.
			im3dRenderer.Draw(context, im3dPipeline);

			graphics.Present(1);
		}
	}

	graphics.Shutdown();
	platform.Shutdown();
}