#include <Engine/Core.hpp>
#include <Engine/HdriToCubemap.hpp>
#include <Engine/Loading.hpp>
#include <Engine/Systems/ImGuiSystem.hpp>
#include <Engine/Systems/SimpleFPSCameraSystem.hpp>
#include <Engine/LightProbeProcessor.hpp>

using namespace Morpheus;

MAIN() {

	Platform platform;
	platform.Startup();

	Graphics graphics(platform);
	graphics.Startup();

	ThreadPool threadPool;
	threadPool.Startup();

	SystemCollection systems;
	auto renderer = systems.Add<DefaultRenderer>(graphics);
	systems.Add<TextureCacheSystem>(graphics);
	systems.Add<GeometryCacheSystem>(graphics);
	systems.Add<SimpleFPSCameraSystem>(platform->GetInput());
	auto imguiSystem = systems.Add<ImGuiSystem>(graphics);
	systems.Startup();

	EmbeddedFileLoader embeddedFiles;

	Handle<Texture> skyboxTexture;
	LightProbe skyboxLightProbe;

	// Compute Skybox Stuff
	{
		auto skyboxHdriTask = Texture::LoadHandle(graphics.Device(), "environment.hdr");
		auto skyboxHdri = threadPool.AdoptAndTrigger(std::move(skyboxHdriTask));

		auto hdriConvShadersTask = HDRIToCubemapShaders::Load(
			graphics.Device(), false, &embeddedFiles);
		auto hdriConvShaders = threadPool.AdoptAndTrigger(std::move(hdriConvShadersTask));

		LightProbeProcessorConfig lightProbeConfig;
		lightProbeConfig.mPrefilteredEnvFormat = DG::TEX_FORMAT_RGBA16_FLOAT;
		auto lightProbeShadersTask = LightProbeProcessorShaders::Load(
			graphics.Device(), lightProbeConfig, &embeddedFiles);
		auto lightProbeShaders = threadPool.AdoptAndTrigger(std::move(lightProbeShadersTask));

		TaskBarrier barrier;
		barrier.mIn.Lock()
			.Connect(skyboxHdri.Out())
			.Connect(hdriConvShaders.Out())
			.Connect(lightProbeShaders.Out());
		
		BasicLoadingScreen(platform, graphics, imguiSystem->GetImGui(), &barrier, &threadPool);

		HDRIToCubemapConverter conv(graphics.Device(), 
			hdriConvShaders.Get(), 
			DG::TEX_FORMAT_RGBA16_FLOAT);

		auto skyboxPtr = conv.Convert(graphics.Device(), graphics.ImmediateContext(), 
			skyboxHdri.Get()->GetShaderView(), 2048, true);

		skyboxTexture.Adopt(new Texture(skyboxPtr));

		LightProbeProcessor processor(graphics.Device(), 
			lightProbeShaders.Get(), lightProbeConfig);
		skyboxLightProbe = processor.ComputeLightProbe(graphics.Device(), 
			graphics.ImmediateContext(), skyboxTexture->GetShaderView());
	}

	MaterialDesc whiteMaterial;
	whiteMaterial.mType = MaterialType::LAMBERT;
	auto material = renderer->CreateMaterial(whiteMaterial);

	auto staticMeshLayout = renderer->GetStaticMeshLayout();

	std::vector<Handle<Geometry>> testGeometries = {
		Geometry::Prefabs::Plane(graphics.Device(), staticMeshLayout),
		Geometry::Prefabs::Box(graphics.Device(), staticMeshLayout),
		Geometry::Prefabs::Sphere(graphics.Device(), staticMeshLayout),
		Geometry::Prefabs::Torus(graphics.Device(), staticMeshLayout),
		Geometry::Prefabs::BlenderMonkey(graphics.Device(), staticMeshLayout),
		Geometry::Prefabs::MaterialBall(graphics.Device(), staticMeshLayout),
		Geometry::Prefabs::StanfordBunny(graphics.Device(), staticMeshLayout),
		Geometry::Prefabs::UtahTeapot(graphics.Device(), staticMeshLayout)
	};

	// Create camera and skybox
	Frame frame;
	frame.mCamera = frame.SpawnDefaultCamera();
	frame.Emplace<Transform>(frame.mCamera).SetTranslation(16.0f, 0.0f, -10.0f);
	frame.Emplace<SimpleFPSCameraController>(frame.mCamera);

	auto skyboxEntity = frame.CreateEntity();
	frame.Emplace<SkyboxComponent>(skyboxEntity, std::move(skyboxTexture));
	frame.Emplace<LightProbe>(skyboxEntity, std::move(skyboxLightProbe));

	// Create geometrical objects
	int index = 0;
	for (auto& geo : testGeometries) {
		auto geoEntity = frame.CreateEntity();
		frame.Emplace<StaticMeshComponent>(geoEntity, material, geo);
		frame.Emplace<Transform>(geoEntity)
			.SetTranslation((index++) * 4.0f, 0.0f, 0.0f);
	}	

	systems.SetFrame(&frame);

	// Run game loop
	DG::Timer timer;
	FrameTime time(timer);

	while (platform.IsValid()) {
		time.UpdateFrom(timer);
		platform.MessagePump();

		systems.RunFrame(time, &threadPool);
		systems.WaitOnRender(&threadPool);
		graphics.Present(1);
		systems.WaitOnUpdate(&threadPool);
	}

	frame = Frame();
	material = Material();
	testGeometries.clear();
	
	systems.Shutdown();
	graphics.Shutdown();
	platform.Shutdown();
}