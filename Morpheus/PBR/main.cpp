#include <Engine/Engine.hpp>
#include <Engine/TextureResource.hpp>
#include <Engine/PipelineResource.hpp>
#include <Engine/MaterialResource.hpp>
#include <Engine/GeometryResource.hpp>
#include <Engine/Transform.hpp>
#include <Engine/HdriToCubemap.hpp>
#include <Engine/Skybox.hpp>
#include <Engine/Brdf.hpp>
#include <Engine/EditorCameraController.hpp>
#include <Engine/Camera.hpp>
#include <Engine/Systems/Physics.hpp>
#include <random>

using namespace Morpheus;

int main(int argc, char** argv) {
	Engine en;

	en.Startup(argc, argv);

	Scene* scene = new Scene();
	scene->AddSystem<PhysicsSystem>();
	
	auto root = scene->GetRoot();
	auto content = en.GetResourceManager();

	
	auto sphereMaterial = content->Load<MaterialResource>("testpbr.json");
	LoadParams<GeometryResource> sphereParams;
	sphereParams.mSource = "sphere.obj";
	sphereParams.mPipelineResource = sphereMaterial->GetPipeline();
	auto sphereMesh = content->Load<GeometryResource>(sphereParams);
	
	int gridRadius = 5;
	for (int x = -gridRadius; x <= gridRadius; ++x) {
		for (int y = -gridRadius; y <= gridRadius; ++y) {
			auto meshNode = root.CreateChild();
			meshNode.Add<GeometryComponent>(sphereMesh);
			meshNode.Add<MaterialComponent>(sphereMaterial);
			Transform& transform = meshNode.Add<Transform>();
			transform.SetTranslation(x * 3.0f, 0.0f, y * 3.0f);
		}
	}

	sphereMesh->Release();
	sphereMaterial->Release();

	auto gunMaterial = content->Load<MaterialResource>("cerberusmat.json");

	LoadParams<GeometryResource> gunParams;
	gunParams.mSource = "cerberus.obj";
	gunParams.mPipelineResource = gunMaterial->GetPipeline();
	auto gunMesh = content->Load<GeometryResource>(gunParams);
	
	auto gunNode = root.CreateChild();
	gunNode.Add<GeometryComponent>(gunMesh);
	gunNode.Add<MaterialComponent>(gunMaterial);

	Transform& transform = gunNode.Add<Transform>(
		DG::float3(0.0f, 8.0f, 0.0f),
		DG::Quaternion::RotationFromAxisAngle(DG::float3(0.0f, 1.0f, 0.0f), DG::PI),
		DG::float3(8.0f, 8.0f, 8.0f)
	);

	gunMaterial->Release();
	gunMesh->Release();

	auto skybox_hdri = en.GetResourceManager()->Load<TextureResource>("environment.hdr");
	HDRIToCubemapConverter conv(en.GetDevice());
	conv.Initialize(en.GetResourceManager(), DG::TEX_FORMAT_RGBA16_FLOAT);
	auto skybox_texture = conv.Convert(en.GetDevice(), en.GetImmediateContext(), skybox_hdri->GetShaderView(), 2048);
	skybox_hdri->Release();

	auto tex_res = new TextureResource(en.GetResourceManager(), skybox_texture);
	tex_res->AddRef();
	content->Add(tex_res, "SKYBOX");

	auto cameraNode = scene->GetCameraNode();
	Transform& t = cameraNode.Add<Transform>();
	t.SetTranslation(0.0f, 0.0f, -5.0f);
	cameraNode.Add<EditorCameraController>(cameraNode, scene);

	auto skybox = root.CreateChild();
	skybox.Add<SkyboxComponent>(tex_res);

	en.SetScene(scene);

	while (en.IsReady()) {
		en.Update();
		en.Render();
		en.Present();
	}

	en.Shutdown();
}