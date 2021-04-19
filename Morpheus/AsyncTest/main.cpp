#include <Engine/Core.hpp>
#include <Engine/EmptyRenderer.hpp>

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

	en.AddComponent<EmptyRenderer>();
	en.Startup();

	auto manager = en.GetResourceManager();

	PipelineResource* pipeline = nullptr;
	TextureResource* texture = nullptr;
	MaterialResource* material = nullptr;
	TextureResource* hdrTexture = nullptr;
	GeometryResource* geometry = nullptr;

	// Generate tasks for loading resources
	Task pipelineTask = manager->LoadTask<PipelineResource>("White", &pipeline);
	Task textureTask = manager->LoadTask<TextureResource>("brick_albedo.png", &texture);
	Task materialTask = manager->LoadTask<MaterialResource>("material.json", &material);
	Task hdrTextureTask = manager->LoadTask<TextureResource>("environment.hdr", &hdrTexture);

	LoadParams<GeometryResource> geoParams;
	geoParams.mMaterial = material;
	geoParams.mSource = "matBall.obj";
	Task geometryTask = manager->LoadTask<GeometryResource>(geoParams, &geometry);

	// Set callbacks executed when the resources are actually loaded
	pipeline->GetLoadBarrier()->SetCallback(Task([](const TaskParams& e) {
		std::cout << "Loaded Pipeline!" << std::endl;
	}));

	texture->GetLoadBarrier()->SetCallback(Task([](const TaskParams& e) {
		std::cout << "Loaded Texture!" << std::endl;
	}));

	material->GetLoadBarrier()->SetCallback(Task([](const TaskParams& e) {
		std::cout << "Loaded Material!" << std::endl;
	}));

	hdrTexture->GetLoadBarrier()->SetCallback(Task([](const TaskParams& e) {
		std::cout << "Loaded HDR Texture!" << std::endl;
	}));

	geometry->GetLoadBarrier()->SetCallback(Task([](const TaskParams& e) {
		std::cout << "Loaded Geometry!" << std::endl;
	}));

	// Submit tasks to the thread pool
	auto threadPool = en.GetThreadPool();
	threadPool->Submit(std::move(pipelineTask));
	threadPool->Submit(std::move(textureTask));
	threadPool->Submit(std::move(materialTask));
	threadPool->Submit(std::move(hdrTextureTask));
	threadPool->Submit(std::move(geometryTask));

	en.CollectGarbage();

	while (en.IsReady()) {
		en.YieldFor(std::chrono::milliseconds(10));
		en.Update([](double, double) { });
		en.Render(nullptr);
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