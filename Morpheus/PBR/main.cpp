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

	Scene* scene = new Scene();
	auto root = scene->GetRoot();

	auto sphereMesh = en.GetResourceManager()
		->Load<StaticMeshResource>("static_mesh.json");

	int gridRadius = 5;

	for (int x = -gridRadius; x <= gridRadius; ++x) {
		for (int y = -gridRadius; y <= gridRadius; ++y) {
			auto meshNode = root.CreateChild();
			StaticMeshComponent& component = meshNode.Add<StaticMeshComponent>(sphereMesh);
			Transform& transform = meshNode.Add<Transform>();
			transform.SetTranslation(x * 3.0f, 0.0f, y * 3.0f);
		}
	}

	auto gunMesh = en.GetResourceManager()->Load<StaticMeshResource>("cerberus.json");
	auto meshNode = root.CreateChild();
	StaticMeshComponent& component = meshNode.Add<StaticMeshComponent>(gunMesh);
	Transform& transform = meshNode.Add<Transform>(
		DG::float3(0.0f, 8.0f, 0.0f),
		DG::Quaternion::RotationFromAxisAngle(DG::float3(0.0f, 1.0f, 0.0f), DG::PI),
		DG::float3(8.0f, 8.0f, 8.0f)
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
	Transform& t = cameraNode.Add<Transform>();
	t.SetTranslation(0.0f, 0.0f, -5.0f);
	scene->GetCameraNode().Add<EditorCameraController>(cameraNode, scene);

	auto skybox = root.CreateChild();
	skybox.Add<SkyboxComponent>(tex_res);

	sphereMesh->Release();

	en.SetScene(scene);

	while (en.IsReady()) {
		en.Update();
		en.Render();
		en.Present();
	}

	en.Shutdown();
}