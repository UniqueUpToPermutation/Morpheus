#include <Engine/Core.hpp>
#include <Engine/DefaultRenderer.hpp>

using namespace Morpheus;

std::mutex mOutput;

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

	en.AddComponent<DefaultRenderer>();
	en.Startup();

	Scene* scene = new Scene();

	auto manager = en.GetResourceManager();

	PipelineResource* pipeline = manager->AsyncLoad<PipelineResource>("White", [](ThreadPool* pool) {
		std::cout << "Loaded pipeline!" << std::endl;
	});

	TextureResource* texture = manager->AsyncLoad<TextureResource>("brick_albedo.png", [](ThreadPool* pool) {
		std::cout << "Loaded texture!" << std::endl;
	});

	MaterialResource* material = manager->AsyncLoad<MaterialResource>("material.json", [](ThreadPool* pool) {
		std::cout << "Loaded material!" << std::endl;
	});

	TextureResource* hdrTexture = manager->AsyncLoad<TextureResource>("environment.hdr", [](ThreadPool* pool) {
		std::cout << "Loaded HDR texture!" << std::endl;
	});

	LoadParams<GeometryResource> geoParams;
	geoParams.mMaterial = material;
	geoParams.mSource = "matBall.obj";

	GeometryResource* geometry = manager->AsyncLoad<GeometryResource>(geoParams, [](ThreadPool* pool) {
		std::cout << "Loaded geometry!" << std::endl;
	});

	en.InitializeDefaultSystems(scene);
	scene->Begin();

	en.CollectGarbage();

	while (en.IsReady()) {
		en.YieldFor(std::chrono::milliseconds(10));
		en.Update(scene);
		
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

		en.RenderUI();
		en.Present();
	}

	geometry->Release();
	texture->Release();
	pipeline->Release();
	material->Release();
	hdrTexture->Release();

	en.Shutdown();
}