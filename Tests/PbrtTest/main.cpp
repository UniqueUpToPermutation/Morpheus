#include <Engine/Core.hpp>
#include <MorpheusPbrt/Interface.hpp>

using namespace Morpheus;

MAIN() {
	Raytrace::RaytracePlatform platform;

	SystemCollection systems;
	systems.AddFromFactory<Raytrace::RaytraceDeviceFactory>(platform);
	systems.Startup();

	auto frame = std::make_unique<Frame>();
	systems.SetFrame(frame.get());

	DG::Timer timer;
	FrameTime time(timer);

	ImmediateComputeQueue queue;

	time.UpdateFrom(timer);

	systems.RunFrame(time, &queue);
	systems.WaitUntilFrameFinished(&queue);

	frame.reset();
	systems.Shutdown();

	auto texture = platform.GetBackbuffer();
	texture->SavePng("output.png");
}