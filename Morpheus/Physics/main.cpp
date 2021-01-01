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
#include <Engine/PhysicsComponents.hpp>
#include <random>

using namespace Morpheus;

class PhysicsTestSphere : public IEntityPrototype {
public:
	std::unique_ptr<btCollisionShape> mPhysicsShape;
	btVector3 mInertia;
	float mMass;

	PhysicsTestSphere(Engine* engine) : mPhysicsShape(new btSphereShape(1.0f)), mMass(1.0f) {
		mPhysicsShape->calculateLocalInertia(mMass, mInertia);
	}

	entt::entity Spawn(Engine* en, SceneHeirarchy* scene) const override {
		auto registry = scene->GetRegistry();
		auto contentManager = en->GetResourceManager();
		auto dynamicsWorld = scene->GetDynamicsWorld();

		auto staticMesh = contentManager->Load<StaticMeshResource>("static_mesh.json");

		auto motionState = new btDefaultMotionState();
		auto rigidBody = new btRigidBody(1.0f, motionState, mPhysicsShape.get(), mInertia);

		auto entity = registry->create();

		dynamicsWorld->addRigidBody(rigidBody);

		registry->emplace<StaticMeshComponent>(entity, staticMesh);
		registry->emplace<Transform>(entity);
		registry->emplace<RigidBodyComponent>(entity, dynamicsWorld, rigidBody, motionState);

		rigidBody->setUserIndex((int)entity);

		return entity;
	}

	entt::entity Clone(entt::entity ent) const override {
		throw std::runtime_error("Not implemented!");
	}
};

int main(int argc, char** argv) {
	Engine en;

	en.Startup(argc, argv);

	SceneHeirarchy* scene = new SceneHeirarchy(&en);
	auto root = scene->GetRoot();

	// Create skybox from environment HDRI
	auto skybox_hdri = en.GetResourceManager()->Load<TextureResource>("environment.hdr");

	HDRIToCubemapConverter conv(en.GetDevice());
	conv.Initialize(en.GetResourceManager(), DG::TEX_FORMAT_RGBA16_FLOAT);
	auto skybox_texture = conv.Convert(en.GetDevice(), en.GetImmediateContext(), skybox_hdri->GetShaderView(), 2048);

	skybox_hdri->Release();

	auto tex_res = new TextureResource(en.GetResourceManager(), skybox_texture);
	en.GetResourceManager()->Add(tex_res, "SKYBOX");

	auto skybox = scene->CreateChild(root);
	skybox.AddComponent<SkyboxComponent>(tex_res);

	// Initialize camera controller
	Transform* t = scene->GetCameraNode().AddComponent<Transform>();
	t->mTranslation = DG::float3(0.0f, 0.0f, -5.0f);
	scene->GetCameraNode().AddComponent<EditorCameraController>(scene);

	// Spawn a physics test sphere at the root
	IEntityPrototype* spherePrototype = new PhysicsTestSphere(&en);
	scene->Spawn(spherePrototype);

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