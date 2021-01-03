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

	SceneHeirarchy* scene = new SceneHeirarchy(&en);
	auto root = scene->GetRoot();

	auto sphereMesh = en.GetResourceManager()->Load<StaticMeshResource>("static_mesh.json");

	for (int x = -5; x <= 5; ++x) {
		for (int y = -5; y <= 5; ++y) {
			auto meshNode = scene->CreateChild(root);
			StaticMeshComponent* component = meshNode.AddComponent<StaticMeshComponent>(sphereMesh);
			Transform* transform = meshNode.AddComponent<Transform>();
			transform->SetTranslation(meshNode, x * 3.0f, 0.0f, y * 3.0f);
		}
	}

	auto gunMesh = en.GetResourceManager()->Load<StaticMeshResource>("cerberus.json");
	auto meshNode = scene->CreateChild(root);
	StaticMeshComponent* component = meshNode.AddComponent<StaticMeshComponent>(gunMesh);
	Transform* transform = meshNode.AddComponent<Transform>(
		DG::float3(0.0f, 8.0f, 0.0f),
		DG::float3(8.0f, 8.0f, 8.0f),
		DG::Quaternion::RotationFromAxisAngle(DG::float3(0.0f, 1.0f, 0.0f), DG::PI)
	);

	auto skybox_hdri = en.GetResourceManager()->Load<TextureResource>("environment.hdr");

	HDRIToCubemapConverter conv(en.GetDevice());
	conv.Initialize(en.GetResourceManager(), DG::TEX_FORMAT_RGBA16_FLOAT);
	auto skybox_texture = conv.Convert(en.GetDevice(), en.GetImmediateContext(), skybox_hdri->GetShaderView(), 2048);

	skybox_hdri->Release();

	auto tex_res = new TextureResource(en.GetResourceManager(), skybox_texture);
	tex_res->AddRef();
	en.GetResourceManager()->Add(tex_res, "SKYBOX");

	auto cameraNode = scene->GetCameraNode();
	Transform* t = cameraNode.AddComponent<Transform>();
	t->SetTranslation(cameraNode, 0.0f, 5.0f, 0.0f);
	scene->GetCameraNode().AddComponent<EditorCameraController>(cameraNode);

	auto skybox = scene->CreateChild(root);
	skybox.AddComponent<SkyboxComponent>(tex_res);

	sphereMesh->Release();

	en.SetScene(scene);

	while (en.IsReady()) {
		en.Update();
		en.Render();
		en.Present();
	}

	en.Shutdown();
}