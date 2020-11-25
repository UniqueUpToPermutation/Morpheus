#include <Engine/Engine.hpp>
#include <Engine/TextureResource.hpp>
#include <Engine/PipelineResource.hpp>
#include <Engine/StaticMeshComponent.hpp>
#include <Engine/Transform.hpp>
#include <Engine/CameraComponent.hpp>
#include <Engine/HdriToCubemap.hpp>
#include <Engine/Skybox.hpp>
#include <Engine/Brdf.hpp>
#include <random>

using namespace Morpheus;

int main(int argc, char** argv) {
	Engine en;

	en.Startup(argc, argv);

	SceneHeirarchy* scene = new SceneHeirarchy(&en);
	auto root = scene->CreateNode();
	auto cameraNode = scene->CreateChild(root);
	auto cameraComponent = cameraNode.AddComponent<CameraComponent>();
	float rot = 0.0f;
	cameraComponent->SetPerspectiveLookAt(
		DG::float3(15.0f * std::sin(rot), 5.0f, 15.0f * std::cos(rot)), 
		DG::float3(0.0f, 0.0f, 0.0f), 
		DG::float3(0.0f, 1.0f, 0.0f));
	auto camera = dynamic_cast<PerspectiveLookAtCamera*>(cameraComponent->GetCamera());
	scene->SetCurrentCamera(cameraComponent);

	auto resource = en.GetResourceManager()->Load<StaticMeshResource>("static_mesh.json");

	std::default_random_engine generator;
	std::uniform_real_distribution<double> distribution(0.0, 2 * DG::PI);

	for (int x = -5; x <= 5; ++x) {
		for (int y = -5; y <= 5; ++y) {
			auto meshNode = scene->CreateChild(root);
			StaticMeshComponent* component = meshNode.AddComponent<StaticMeshComponent>(resource);
			Transform* transform = meshNode.AddComponent<Transform>();
			transform->mTranslation.x = x * 4.0f;
			transform->mTranslation.z = y * 4.0f;

			transform->mRotation = DG::Quaternion::RotationFromAxisAngle(DG::float3(0.0f, 1.0f, 0.0f), 
				distribution(generator));
		}
	}

	auto skybox_hdri = en.GetResourceManager()->Load<TextureResource>("environment.hdr");

	HDRIToCubemapConverter conv(en.GetDevice());
	conv.Initialize(en.GetResourceManager(), DG::TEX_FORMAT_RGBA16_FLOAT);
	auto skybox_texture = conv.Convert(en.GetDevice(), en.GetImmediateContext(), skybox_hdri->GetShaderView(), 2048);

	skybox_hdri->Release();

	auto tex_res = new TextureResource(en.GetResourceManager(), skybox_texture);
	tex_res->AddRef();
	en.GetResourceManager()->Add(tex_res, "SKYBOX");

	/*LightProbeProcessor processor(en.GetDevice());
	processor.Initialize(en.GetResourceManager(), 
		DG::TEX_FORMAT_RGBA16_FLOAT, 
		DG::TEX_FORMAT_RGBA16_FLOAT, 
		64, 128);
	auto tex = processor.ComputeIrradiance(en.GetDevice(), en.GetImmediateContext(), skybox_texture->GetShaderView(), 256);
	auto tex2 = processor.ComputePrefilteredEnvironment(en.GetDevice(), en.GetImmediateContext(), skybox_texture->GetShaderView(), 256);

	auto tex_res = new TextureResource(en.GetResourceManager(), tex);
	tex_res->AddRef();
	en.GetResourceManager()->Add(tex_res, "PROCESSED RESOURCE");

	auto tex_res2 = new TextureResource(en.GetResourceManager(), tex2);
	tex_res2->AddRef();
	en.GetResourceManager()->Add(tex_res2, "PROCESSED RESOURCE 2");*/

	auto skybox = scene->CreateChild(root);
	skybox.AddComponent<SkyboxComponent>(tex_res);

	resource->Release();

	en.SetScene(scene);

	while (en.IsReady()) {
		en.Update();

		rot += 0.01f;
		camera->mEye = DG::float3(15.0f * std::sin(rot), 5.0f, 15.0f * std::cos(rot));

		en.Render();
		en.Present();
	}

	en.Shutdown();
}