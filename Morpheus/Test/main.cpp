#include <Engine/Engine.hpp>
#include <Engine/TextureResource.hpp>
#include <Engine/PipelineResource.hpp>
#include <Engine/StaticMeshComponent.hpp>
#include <Engine/Transform.hpp>
#include <Engine/HdriToCubemap.hpp>
#include <Engine/Skybox.hpp>
#include <Engine/Brdf.hpp>
#include <Engine/EditorCameraController.hpp>
#include <Engine/Camera.hpp>
#include <random>

using namespace Morpheus;

int main(int argc, char** argv) {
	Engine en;

	en.Startup(argc, argv);

	SceneHeirarchy* scene = new SceneHeirarchy();
	auto root = scene->GetRoot();

	auto sphereMesh = en.GetResourceManager()->Load<StaticMeshResource>("static_mesh.json");

	std::default_random_engine generator;
	std::uniform_real_distribution<double> distribution(0.0, 2 * DG::PI);

	for (int x = -5; x <= 5; ++x) {
		for (int y = -5; y <= 5; ++y) {
			auto meshNode = scene->CreateChild(root);
			StaticMeshComponent* component = meshNode.AddComponent<StaticMeshComponent>(sphereMesh);
			Transform* transform = meshNode.AddComponent<Transform>();
			transform->mTranslation.x = x * 4.0f;
			transform->mTranslation.z = y * 4.0f;

			transform->mRotation = DG::Quaternion::RotationFromAxisAngle(DG::float3(0.0f, 1.0f, 0.0f), 
				distribution(generator));
		}
	}

	auto gunMesh = en.GetResourceManager()->Load<StaticMeshResource>("static_mesh2.json");
	auto meshNode = scene->CreateChild(root);
	StaticMeshComponent* component = meshNode.AddComponent<StaticMeshComponent>(gunMesh);
	Transform* transform = meshNode.AddComponent<Transform>();
	transform->mTranslation.y = 3.0f;
	transform->mScale = DG::float3(4.0f, 4.0f, 4.0f);

	auto skybox_hdri = en.GetResourceManager()->Load<TextureResource>("environment.hdr");

	HDRIToCubemapConverter conv(en.GetDevice());
	conv.Initialize(en.GetResourceManager(), DG::TEX_FORMAT_RGBA16_FLOAT);
	auto skybox_texture = conv.Convert(en.GetDevice(), en.GetImmediateContext(), skybox_hdri->GetShaderView(), 2048);

	skybox_hdri->Release();

	auto tex_res = new TextureResource(en.GetResourceManager(), skybox_texture);
	tex_res->AddRef();
	en.GetResourceManager()->Add(tex_res, "SKYBOX");

	Transform* t = scene->GetCameraNode().AddComponent<Transform>();
	t->mTranslation = DG::float3(0.0f, 5.0f, 0.0f);

	scene->GetCameraNode().AddComponent<EditorCameraController>(scene);

	auto skybox = scene->CreateChild(root);
	skybox.AddComponent<SkyboxComponent>(tex_res);

	sphereMesh->Release();

	en.SetScene(scene);

	while (en.IsReady()) {
		en.Update();

		auto camera = scene->GetCameraNode();
		auto transform = camera.GetComponent<Transform>();
		transform->UpdateCache(nullptr);

		en.Render();
		en.Present();
	}

	en.Shutdown();
}