#include <Engine/Core.hpp>
#include <Engine/Raytrace/DefaultRaytracer.hpp>

#include <random>

using namespace Morpheus;

MAIN() {
	DG::TextureDesc outputDesc;
	outputDesc.Width = 1024;
	outputDesc.Height = 756;
	outputDesc.Format = DG::TEX_FORMAT_RGBA8_UNORM;
	outputDesc.Type = DG::RESOURCE_DIM_TEX_2D;
	Texture outputTexture(outputDesc);

	ThreadPool pool;
	pool.Startup();

	SystemCollection systems;
	auto raytracer = systems.Add<Raytrace::DefaultRaytracer>();
	systems.Startup(&pool);

	Frame frame;
	frame.SpawnDefaultCamera();

	systems.SetFrame(&frame);
	raytracer->SetOutput(&outputTexture);

	DG::Timer timer;
	FrameTime frameTime(timer);

	systems.RenderFrame(frameTime, &pool);
	pool.Shutdown();

	outputTexture.SavePng("output.png");
}