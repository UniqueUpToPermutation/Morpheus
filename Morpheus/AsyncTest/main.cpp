#include <Engine/Core.hpp>
#include <Engine/EmptyRenderer.hpp>
#include <Engine/Loading.hpp>

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

	// Submit tasks to the thread pool
	auto threadPool = en.GetThreadPool();
	threadPool->AdoptAndTrigger(std::move(pipelineTask));
	threadPool->AdoptAndTrigger(std::move(textureTask));
	threadPool->AdoptAndTrigger(std::move(materialTask));
	threadPool->AdoptAndTrigger(std::move(hdrTextureTask));
	threadPool->AdoptAndTrigger(std::move(geometryTask));

	std::vector<IResource*> resourcesToLoad = {
		pipeline,
		texture,
		material, 
		hdrTexture,
		geometry
	};

	LoadingScreen(&en, resourcesToLoad);

	std::cout << "Everything loaded!" << std::endl;

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