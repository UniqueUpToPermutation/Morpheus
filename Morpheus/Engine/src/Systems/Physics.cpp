#include <Engine/Systems/Physics.hpp>
#include <Engine/Components/PhysicsComponents.hpp>
#include <Engine/Components/Transform.hpp>

#include "btBulletDynamicsCommon.h"

namespace Morpheus {

	void PhysicsSystem::InitPhysics(Scene* scene) {
		std::cout << "Initializing physics subsystem..." << std::endl;

		if (!mCollisionConfiguration)
			mCollisionConfiguration = new btDefaultCollisionConfiguration();
		if (!mCollisionDispatcher)
			mCollisionDispatcher = new btCollisionDispatcher(mCollisionConfiguration);
		if (!mBroadphaseInterface)
			mBroadphaseInterface = new btDbvtBroadphase();
		if (!mConstaintSolver)
			mConstaintSolver = new btSequentialImpulseConstraintSolver();
		if (!mWorld) {
			mWorld = new btDiscreteDynamicsWorld(mCollisionDispatcher, 
				mBroadphaseInterface,
				mConstaintSolver, 
				mCollisionConfiguration);

			auto rbView = scene->GetRegistry()->view<RigidBodyComponent>();

			for (auto e : rbView) {
				auto& rb = rbView.get<RigidBodyComponent>(e);
				mWorld->addRigidBody(rb);
			}
		}

		mWorld->setGravity(btVector3(0, -10, 0));
	}

	void PhysicsSystem::KillPhysics(Scene * scene) {

		std::cout << "Shutting down physics subsystem..." << std::endl;

		if (mWorld) {
			auto rbView = scene->GetRegistry()->view<RigidBodyComponent>();

			for (auto e : rbView) {
				auto& rb = rbView.get<RigidBodyComponent>(e);
				mWorld->removeRigidBody(rb);
			}

			delete mWorld;
			mWorld = nullptr;
		}

		if (mConstaintSolver) {
			delete mConstaintSolver;
			mConstaintSolver = nullptr;
		}

		if (mBroadphaseInterface) {
			delete mBroadphaseInterface;
			mBroadphaseInterface = nullptr;
		}

		if (mCollisionDispatcher) {
			delete mCollisionDispatcher;
			mCollisionDispatcher = nullptr;
		}

		if (mCollisionConfiguration) {
			delete mCollisionConfiguration;
			mCollisionConfiguration = nullptr;
		}
	}

	void PhysicsSystem::Startup(Scene* scene) {
		scene->GetRegistry()->on_construct<RigidBodyComponent>().connect<&PhysicsSystem::OnConstructRigidBody>(this);
		scene->GetRegistry()->on_update<RigidBodyComponent>().connect<&PhysicsSystem::OnUpdateRigidBody>(this);
		scene->GetRegistry()->on_destroy<RigidBodyComponent>().connect<&PhysicsSystem::OnDestroyRigidBody>(this);

		InitPhysics(scene);

		scene->GetDispatcher()->sink<UpdateEvent>().connect<&PhysicsSystem::OnSceneUpdate>(this);
	}

	void PhysicsSystem::OnSceneUpdate(const UpdateEvent& e) {
		mWorld->stepSimulation(1.0 / 60.0f);

		auto registry = e.mSender->GetRegistry();
		auto view = registry->view<RigidBodyComponent>();

		for (auto e : view) {
			auto& rb = view.get<RigidBodyComponent>(e);

			if (rb->isActive()) {
				auto cache = registry->try_get<MatrixTransformCache>(e);
				if (cache) {
					btTransform transform;
					rb->getMotionState()->getWorldTransform(transform);
					transform.getOpenGLMatrix(&cache->mCache[0][0]);
				}
			}
		}
	}

	void PhysicsSystem::OnConstructRigidBody(entt::registry& reg, entt::entity e) {
		mWorld->addRigidBody(reg.get<RigidBodyComponent>(e));
	}

	void PhysicsSystem::OnDestroyRigidBody(entt::registry& reg, entt::entity e) {
		auto component = reg.get<RigidBodyComponent>(e);
		mWorld->removeRigidBody(component);

		auto ms = component->getMotionState();

		if (ms)
			delete ms;

		delete component.RawPtr();
	}

	void PhysicsSystem::OnUpdateRigidBody(entt::registry& reg, entt::entity e) {
	}
	
	void PhysicsSystem::Shutdown(Scene* scene) {
		scene->GetDispatcher()->sink<UpdateEvent>().disconnect<&PhysicsSystem::OnSceneUpdate>(this);

		KillPhysics(scene);

		scene->GetRegistry()->on_construct<RigidBodyComponent>().disconnect<&PhysicsSystem::OnConstructRigidBody>(this);
		scene->GetRegistry()->on_update<RigidBodyComponent>().disconnect<&PhysicsSystem::OnUpdateRigidBody>(this);
		scene->GetRegistry()->on_destroy<RigidBodyComponent>().disconnect<&PhysicsSystem::OnDestroyRigidBody>(this);
	}
}