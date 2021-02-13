#include <Engine/Scene.hpp>
#include <Engine/Engine.hpp>
#include <Engine/Camera.hpp>

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
		if (!IsInitializedByEngine()) {
			throw std::runtime_error("Scene has not been initialized by engine!"
				"Please call Engine::InitializeDefaultSystems on this scene!");
		}

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

		for (auto gui : mImGuiObjects) {
			gui->OnDestroy(this);
			delete gui;
		}

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
		if (!bBeginCalled) {
			throw std::runtime_error("Scene::Begin has not yet been called!");
		}

		// Update everything else that needs updating
		UpdateEvent e;
		e.mSender = this;
		e.mCurrTime = currTime;
		e.mElapsedTime = elapsedTime;

		mDispatcher.trigger<UpdateEvent>(e);

		for (auto gui : mImGuiObjects) {
			gui->OnUpdate(this, currTime, elapsedTime);
		}
	}
}