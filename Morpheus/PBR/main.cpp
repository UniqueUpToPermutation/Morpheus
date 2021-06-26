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
	Handle<Geometry> gunGeometry;
	MaterialDesc gunMaterialDesc;
	LightProbe skyboxLightProbe;

	{
		// Create a material for the gun.
		MaterialDescFuture gunMaterialFuture;
		gunMaterialFuture.mType 		= MaterialType::LAMBERT;
		gunMaterialFuture.mAlbedo 		= systems.Load<Texture>("cerberus_A.png", &threadPool);
		gunMaterialFuture.mNormal 		= systems.Load<Texture>("cerberus_N.png", &threadPool);
		gunMaterialFuture.mMetallic 	= systems.Load<Texture>("cerberus_M.png", &threadPool);
		gunMaterialFuture.mRoughness 	= systems.Load<Texture>("cerberus_R.png", &threadPool);

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

		LoadParams<Geometry> geoParams;
		geoParams.mSource = "cerberus.obj";
		geoParams.mType = GeometryType::STATIC_MESH;
		auto gunGeoFuture = systems.Load<Geometry>(geoParams, &threadPool);

		TaskBarrier barrier;
		barrier.mIn.Lock()
			.Connect(gunMaterialFuture.Out())
			.Connect(skyboxHdri.Out())
			.Connect(hdriConvShaders.Out())
			.Connect(lightProbeShaders.Out())
			.Connect(gunGeoFuture.Out());
		
		BasicLoadingScreen(platform, graphics, imguiSystem->GetImGui(), &barrier, &threadPool);

		HDRIToCubemapConverter conv(graphics.Device(), 
			hdriConvShaders.Get(), 
			DG::TEX_FORMAT_RGBA16_FLOAT);

		auto skyboxPtr = conv.Convert(graphics.Device(), graphics.ImmediateContext(), 
			skyboxHdri.Get()->GetShaderView(), 2048, true);

		skyboxTexture.Adopt(new Texture(skyboxPtr));
		gunGeometry.Adopt(gunGeoFuture.Get());

		LightProbeProcessor processor(graphics.Device(), 
			lightProbeShaders.Get(), lightProbeConfig);
		skyboxLightProbe = processor.ComputeLightProbe(graphics.Device(), 
			graphics.ImmediateContext(), skyboxTexture->GetShaderView());

		gunMaterialDesc = gunMaterialFuture.Get();
	}

	Material gunMaterial = renderer->CreateMaterial(gunMaterialDesc);

	// Create camera and skybox
	Frame frame;
	frame.mCamera = frame.SpawnDefaultCamera();
	frame.Emplace<Transform>(frame.mCamera).SetTranslation(0.0f, 0.0f, -5.0f);
	frame.Emplace<SimpleFPSCameraController>(frame.mCamera);

	auto gunEntity = frame.CreateEntity();
	frame.Emplace<StaticMeshComponent>(gunEntity, 
		std::move(gunMaterial), std::move(gunGeometry));
	frame.Emplace<Transform>(gunEntity)
		.SetScale(4.0f);

	auto skyboxEntity = frame.CreateEntity();
	frame.Emplace<SkyboxComponent>(skyboxEntity, std::move(skyboxTexture));
	frame.Emplace<LightProbe>(skyboxEntity, std::move(skyboxLightProbe));

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
	gunMaterial = Material();
	
	systems.Shutdown();
	graphics.Shutdown();
	platform.Shutdown();
}