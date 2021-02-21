#include <Engine/Core.hpp>
#include <Engine/Im3d.hpp>

using namespace Morpheus;

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
	camera->SetType(CameraType::ORTHOGRAPHIC);
	camera->SetOrthoSize(2.0f, 2.0f);
	camera->SetClipPlanes(-1.0f, 1.0f);

	en.InitializeDefaultSystems(scene.get());
	scene->Begin();

	while (en.IsReady()) {
		en.Update(scene.get());
		
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