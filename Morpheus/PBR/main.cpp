#include <Engine/Engine.hpp>
#include <Engine/Resources/TextureResource.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/MaterialResource.hpp>
#include <Engine/Resources/GeometryResource.hpp>
#include <Engine/Components/ResourceComponents.hpp>
#include <Engine/Components/Transform.hpp>
#include <Engine/HdriToCubemap.hpp>
#include <Engine/Components/SkyboxComponent.hpp>
#include <Engine/Brdf.hpp>
#include <Engine/EditorCameraController.hpp>
#include <Engine/Camera.hpp>
#include <Engine/Systems/Physics.hpp>

using namespace Morpheus;


int main(int argc, char** argv) {
	
	Engine en;
	en.Startup(argc, argv);

	Scene* scene = new Scene();


	auto root = scene->GetRoot();
	auto content = en.GetResourceManager();


	// Create Grid of Spheres
	GeometryResource* sphereMesh;
	MaterialResource* sphereMaterial;
	content->LoadMesh("sphere.obj", "brick.json", &sphereMesh, &sphereMaterial);
	
	int gridRadius = 0;
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


	// Create Gun
	/*GeometryResource* gunMesh;
	MaterialResource* gunMaterial;
	content->LoadMesh("cerberus.obj", "cerberusmat.json", &gunMesh, &gunMaterial);
	
	auto gunNode = root.CreateChild();
	gunNode.Add<GeometryComponent>(gunMesh);
	gunNode.Add<MaterialComponent>(gunMaterial);

	Transform& transform = gunNode.Add<Transform>(
		DG::float3(0.0f, 8.0f, 0.0f),
		DG::Quaternion::RotationFromAxisAngle(DG::float3(0.0f, 1.0f, 0.0f), DG::PI),
		DG::float3(8.0f, 8.0f, 8.0f)
	);

	gunMaterial->Release();
	gunMesh->Release();*/


	// Load HDRI and convert it to a cubemap
	auto skybox_hdri = en.GetResourceManager()->Load<TextureResource>("environment.hdr");
	HDRIToCubemapConverter conv(en.GetDevice());
	conv.Initialize(content, DG::TEX_FORMAT_RGBA16_FLOAT);
	auto skybox_texture = conv.Convert(en.GetDevice(), en.GetImmediateContext(), skybox_hdri->GetShaderView(), 2048, true);
	skybox_hdri->Release();


	// Create skybox from HDRI cubemap
	auto tex_res = new TextureResource(content, skybox_texture);
	tex_res->AddRef();
	auto skybox = root.CreateChild();
	skybox.Add<SkyboxComponent>(tex_res);
	tex_res->Release();


	// Create a controller 
	auto cameraNode = scene->GetCameraNode();
	cameraNode.Add<Transform>().SetTranslation(0.0f, 0.0f, -5.0f);
	cameraNode.Add<EditorCameraController>(cameraNode, scene);

	en.InitializeDefaultSystems(scene);
	scene->Begin();

	while (en.IsReady()) {
		en.Update(scene);
		en.Render(scene);
		en.RenderUI();
		en.Present();
	}

	delete scene;
	en.Shutdown();
}