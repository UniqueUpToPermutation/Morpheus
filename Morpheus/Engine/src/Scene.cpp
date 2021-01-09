#include <Engine/Scene.hpp>
#include <Engine/Engine.hpp>
#include <Engine/Camera.hpp>
#include <Engine/Transform.hpp>

namespace Morpheus {
	Scene::Scene() :
		mCamera(EntityNode::Invalid()) {
		
		// Create root
		mRoot = CreateNode();

		// Create camera
		mCamera = mRoot.CreateChild();
		mCamera.Add<Camera>();
	}

	void Scene::Begin() {
		if (!bBeginCalled) {
			SceneBeginEvent e;
			e.mSender = this;
			mDispatcher.trigger<SceneBeginEvent>(e);
			bBeginCalled = true;
		}
	}

	void Scene::Shutdown() {
		for (auto sys : mSystems) {
			sys.second->Shutdown(this);
		}
	}

	Scene::~Scene() {
		Clear();
		Shutdown();

		for (auto sys : mSystems) {
			delete sys.second;
		}
	}

	void Scene::Clear() {
		mRegistry.clear();
	}

	Camera* Scene::GetCamera() {
		return mCamera.TryGet<Camera>();
	}

	EntityNode Scene::GetRoot() {
		return mRoot;
	}

	EntityNode Scene::CreateNode() {
		entt::entity e = mRegistry.create();
		mRegistry.emplace<HierarchyData>(e);
		return EntityNode(&mRegistry, e);
	}

	EntityNode Scene::CreateNode(entt::entity e) {
		mRegistry.emplace<HierarchyData>(e);
		return EntityNode(&mRegistry, e);
	}

	void Scene::Update(double currTime, double elapsedTime) {
		// Update everything else that needs updating
		UpdateEvent e;
		e.mSender = this;
		e.mCurrTime = currTime;
		e.mElapsedTime = elapsedTime;

		mDispatcher.trigger<UpdateEvent>(e);
	}
}