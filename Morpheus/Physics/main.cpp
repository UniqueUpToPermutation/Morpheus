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

void CreateSphere(btSphereShape* sphere, 
	Scene* scene, 
	const DG::float3 position,
	GeometryResource* geo,
	MaterialResource* mat) {

	btMotionState* motionState = new btDefaultMotionState(btTransform(btMatrix3x3::getIdentity(), 
		btVector3(position.x, position.y, position.z)));

	btVector3 inertia;
	sphere->calculateLocalInertia(1.0f, inertia);
	btRigidBody* rb = new btRigidBody(1.0f, motionState, sphere, inertia);

	auto meshNode = scene->GetRoot().CreateChild();
	meshNode.Add<GeometryComponent>(geo);
	meshNode.Add<MaterialComponent>(mat);
	meshNode.Add<Transform>().SetTranslation(position);
	meshNode.Add<RigidBodyComponent>(rb);
}

int main(int argc, char** argv) {
	Engine en;
	en.Startup(argc, argv);

	btSphereShape* sphere = new btSphereShape(1.0f);
	btBoxShape* box = new btBoxShape(btVector3(10.0f, 0.1f, 10.0f));
	btRigidBody* ground_rb = new btRigidBody(0.0f, nullptr, box);

	

	Scene* scene = new Scene();
	scene->AddSystem<PhysicsSystem>();


	auto root = scene->GetRoot();
	auto content = en.GetResourceManager();



	GeometryResource* groundMesh;
	MaterialResource* groundMaterial;
	content->LoadMesh("ground.obj", "brick.json", &groundMesh, &groundMaterial);
	
	auto groundNode = root.CreateChild();
	groundNode.Add<GeometryComponent>(groundMesh);
	groundNode.Add<MaterialComponent>(groundMaterial);
	groundNode.Add<Transform>().SetTranslation(0.0f, -10.0f, 0.0f);
	ground_rb->setWorldTransform(btTransform(btMatrix3x3::getIdentity(), btVector3(0.0f, -10.0f, 0.0f)));
	groundNode.Add<RigidBodyComponent>(ground_rb);

	groundMesh->Release();
	groundMaterial->Release();

	// Create Grid of Spheres
	GeometryResource* sphereMesh;
	MaterialResource* sphereMaterial;
	content->LoadMesh("sphere.obj", "testpbr.json", &sphereMesh, &sphereMaterial);

	for (int i = 0; i < 10; ++i) {
		CreateSphere(sphere, scene, 
			DG::float3(0.0f, 2.5f * i, 0.0f),
			sphereMesh, sphereMaterial);
	}

	sphereMesh->Release();
	sphereMaterial->Release();


	// Load HDRI and convert it to a cubemap
	auto skybox_hdri = en.GetResourceManager()->Load<TextureResource>("environment.hdr");
	HDRIToCubemapConverter conv(en.GetDevice());
	conv.Initialize(content, DG::TEX_FORMAT_RGBA16_FLOAT);
	auto skybox_texture = conv.Convert(en.GetDevice(), en.GetImmediateContext(), skybox_hdri->GetShaderView(), 2048);
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


	en.SetScene(scene);

	while (en.IsReady()) {
		en.Update();
		en.Render();
		en.Present();
	}

	en.Shutdown();

	delete sphere;
	delete box;
}