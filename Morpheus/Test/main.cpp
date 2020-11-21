#include <Engine/Engine.hpp>
#include <Engine/PipelineResource.hpp>
#include <Engine/StaticMeshComponent.hpp>
#include <Engine/Transform.hpp>
#include <Engine/CameraComponent.hpp>
#include <Engine/Skybox.hpp>
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
		DG::float3(15.0f * std::sin(rot), 15.0f, 15.0f * std::cos(rot)), 
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

	auto skybox_texture = en.GetResourceManager()->Load<TextureResource>("env.ktx");
	auto skybox = scene->CreateChild(root);
	skybox.AddComponent<SkyboxComponent>(skybox_texture);

	skybox_texture->Release();
	resource->Release();

	skybox_texture->SavePng("test.png");

	en.SetScene(scene);

	while (en.IsReady()) {
		en.Update();

		rot += 0.01f;
		camera->mEye = DG::float3(15.0f * std::sin(rot), 15.0f, 15.0f * std::cos(rot));

		en.Render();
		en.Present();
	}

	en.Shutdown();
}