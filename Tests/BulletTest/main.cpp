#include <Engine/Core.hpp>
#include <Engine/Systems/BulletPhysics.hpp>
#include <Engine/HdriToCubemap.hpp>
#include <Engine/Loading.hpp>
#include <Engine/Systems/SimpleFPSCameraSystem.hpp>
#include <Engine/LightProbeProcessor.hpp>

using namespace Morpheus;
using namespace Morpheus::Bullet;

MAIN() {
	EmbeddedFileLoader embeddedFiles;

	ThreadPool threadPool;
	threadPool.Startup();

	Platform platform;
	platform.Startup();

	RealtimeGraphics graphics(platform);
	graphics.Startup();

	SystemCollection systems;
	auto physics = systems.Add<Bullet::PhysicsSystem>();
	auto renderer = systems.Add<DefaultRenderer>(graphics);
	systems.Add<SimpleFPSCameraSystem>(platform.GetInput());
	systems.Startup(&threadPool);

	Handle<Texture> skyboxTexture;
	LightProbe skyboxLightProbe;

	{
		auto skyboxHdriTask = Texture::Load(graphics.Device(), "environment.hdr");
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

		threadPool.YieldUntilFinished(&barrier);

		HDRIToCubemapConverter conv(graphics.Device(), 
			hdriConvShaders.Get(), DG::TEX_FORMAT_RGBA16_FLOAT);

		auto skyboxPtr = conv.Convert(graphics.Device(), graphics.ImmediateContext(), 
			skyboxHdri.Get()->GetShaderView(), 2048, true);

		skyboxTexture.Adopt(new Texture(skyboxPtr));

		LightProbeProcessor processor(graphics.Device(), 
			lightProbeShaders.Get(), lightProbeConfig);

		skyboxLightProbe = processor.ComputeLightProbe(graphics.Device(), 
			graphics.ImmediateContext(), skyboxTexture->GetShaderView());
	}

	Handle<IShapeResource> planeShape;
	Handle<IShapeResource> sphereShape;

	btStaticPlaneShape plane(btVector3(0.0f, 1.0f, 0.0f), 0.0f);
	planeShape.Adopt(new Bullet::ShapeResource<btStaticPlaneShape>(std::move(plane)));

	btSphereShape sphere(1.0);
	sphereShape.Adopt(new Bullet::ShapeResource<btSphereShape>(std::move(sphere)));

	auto planeGeo = Geometry::Prefabs::Plane(graphics.Device(), 
		renderer->GetStaticMeshLayout());
	auto sphereGeo = Geometry::Prefabs::Sphere(graphics.Device(),
		renderer->GetStaticMeshLayout());
	Material defaultMaterial = renderer->CreateMaterial(MaterialDesc());

	Frame frame;
	frame.mCamera = frame.SpawnDefaultCamera();
	frame.Emplace<Transform>(frame.mCamera).SetTranslation(0.0f, 5.0f, -5.0f);
	frame.Emplace<SimpleFPSCameraController>(frame.mCamera);

	btDynamicsWorld* world = new btDiscreteDynamicsWorld(
		physics->GetCollisionDispatcher(),
		physics->GetBroadphase(),
		physics->GetConstraintSolver(),
		physics->GetConfig());
	world->setGravity(btVector3(0.0f, -0.05f, 0.0f));

	btMotionState* groundMs = new btDefaultMotionState();
	btRigidBody* groundRb = new btRigidBody(0.0, groundMs, planeShape->GetShape());

	auto physicsWorldEntity = frame.CreateEntity();
	frame.Emplace<DynamicsWorld>(physicsWorldEntity, DynamicsWorld(world));

	auto groundEntity = frame.CreateEntity();
	frame.Emplace<StaticMeshComponent>(groundEntity, 
		StaticMeshComponent{defaultMaterial, &planeGeo});
	frame.Emplace<Transform>(groundEntity);
	frame.Emplace<RigidBody>(groundEntity, groundRb, groundMs, planeShape);

	auto skyboxEntity = frame.CreateEntity();
	frame.Emplace<SkyboxComponent>(skyboxEntity, std::move(skyboxTexture));
	frame.Emplace<LightProbe>(skyboxEntity, std::move(skyboxLightProbe));

	for (int i = 0; i < 10; ++i) {
		auto sphereEntity = frame.CreateEntity();
		frame.Emplace<StaticMeshComponent>(sphereEntity,
			StaticMeshComponent{defaultMaterial, &sphereGeo});
		frame.Emplace<Transform>(sphereEntity).SetTranslation(0.0f, 5.0f + 3.0f * i, 6.0f + 3.0f * i);

		btMotionState* sphereMs = new btDefaultMotionState();
		btRigidBody* sphereRb = new btRigidBody(1.0f, sphereMs, sphereShape->GetShape());
		frame.Emplace<RigidBody>(sphereEntity, sphereRb, sphereMs, sphereShape);
	}

	systems.SetFrame(&frame);

	DG::Timer timer;
	FrameTime time(timer);

	while (platform.IsValid()) {
		platform.MessagePump();
		time.UpdateFrom(timer);

		systems.RunFrame(time, &threadPool);
		systems.WaitOnRender(&threadPool);
		graphics.Present(1);
		systems.WaitOnUpdate(&threadPool);
	}

	sphereGeo = Geometry();
	planeGeo = Geometry();
	frame = Frame();
	systems.Shutdown();
	graphics.Shutdown();
	platform.Shutdown();
}