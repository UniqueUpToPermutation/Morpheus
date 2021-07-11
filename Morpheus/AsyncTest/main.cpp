#include <Engine/Core.hpp>
#include <Engine/Loading.hpp>
#include <Engine/Systems/EmptyRenderer.hpp>
#include <Engine/Systems/ImGuiSystem.hpp>

using namespace Morpheus;

std::mutex mOutput;

MAIN() {
	Platform platform;
	platform.Startup();

	Graphics graphics(platform);
	graphics.Startup();

	ThreadPool taskQueue;
	taskQueue.Startup();

	SystemCollection systems;
	systems.Add<TextureCacheSystem>(graphics);
	systems.Add<GeometryCacheSystem>(graphics);
	systems.Add<EmptyRenderer>(graphics);
	auto imguiSystem = systems.Add<ImGuiSystem>(graphics);

	systems.Startup(&taskQueue);

	{
		auto geometryLayout = systems
			.QueryInterface<IVertexFormatProvider>()
			->GetStaticMeshLayout();

		// Using Texture::Load or Geometry::Load means the resulting object will not be
		// managed by the texture and geometry caches.
		ResourceTask<Handle<Texture>> unmanagedTextureTask = 
			Texture::Load(graphics.Device(), "brick_albedo.png");
		LoadParams<Geometry> geoLoad("matBall.obj", geometryLayout);
		ResourceTask<Handle<Geometry>> unmanagedGeoTask = 
			Geometry::Load(graphics.Device(), geoLoad);
		ResourceTask<Handle<Texture>> unmanagedHdrTask = 
			Texture::Load(graphics.Device(), "environment.hdr");

		// You can submit a load task to the task queue by using AdoptAndTrigger,
		// this converts a resource task into a resource future
		Future<Handle<Texture>> unmanagedTextureFuture = 
			taskQueue.AdoptAndTrigger(std::move(unmanagedTextureTask));
		Future<Handle<Geometry>> unmanagedGeometryFuture = 
			taskQueue.AdoptAndTrigger(std::move(unmanagedGeoTask));
		Future<Handle<Texture>> unmanagedHdrFuture = 
			taskQueue.AdoptAndTrigger(std::move(unmanagedHdrTask));

		// Alternatively, you can use the resource caches to load textures / geometry.
		// These textures are not gauranteed to be stable, and may move around in memory
		// or be disposed of to make room for new textures.
		Future<Texture*> managedTextureFuture = 
			systems.Load<Texture>("brick_albedo.png", &taskQueue);
		Future<Geometry*> managedGeoFuture = 
			systems.Load<Geometry>(geoLoad, &taskQueue);
		Future<Texture*> managedHdrFuture = 
			systems.Load<Texture>("environment.hdr", &taskQueue);

		// Create a barrier that is invoked after all resources are loaded
		TaskBarrier barrier;
		barrier.mIn.Lock()
			.Connect(unmanagedTextureFuture.Out())
			.Connect(unmanagedGeometryFuture.Out())
			.Connect(unmanagedHdrFuture.Out())
			.Connect(managedTextureFuture.Out())
			.Connect(managedGeoFuture.Out())
			.Connect(managedHdrFuture.Out());

		BasicLoadingScreen(platform, graphics, imguiSystem->GetImGui(), &barrier, &taskQueue);

		std::cout << "Everything loaded!" << std::endl;

		DG::Timer timer;
		FrameTime time(timer);

		while (platform.IsValid()) {
			time.UpdateFrom(timer);
			platform.MessagePump();

			systems.RunFrame(time, &taskQueue);
			systems.WaitOnRender(&taskQueue);
			graphics.Present(1);
			systems.WaitOnUpdate(&taskQueue);
		}
	}

	systems.Shutdown();
	taskQueue.Shutdown();
	graphics.Shutdown();
	platform.Shutdown();
}