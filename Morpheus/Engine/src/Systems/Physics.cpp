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

			auto registry = scene->GetRegistry();
			auto rbView = registry->view<RigidBodyComponent>();

			for (auto e : rbView) {
				auto& rb = rbView.get<RigidBodyComponent>(e);
				mWorld->addRigidBody(rb);

				auto transform = registry->try_get<Transform>(e);
				if (transform) {
					btTransform bulletTransform = transform->ToBullet();
					auto ms = rb->getMotionState();
					if (ms) {
						ms->setWorldTransform(bulletTransform);
					}
					rb->setWorldTransform(bulletTransform);
				}
			}
		}

		mWorld->setGravity(btVector3(0, -15, 0));
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
		auto registry = scene->GetRegistry();

		registry->on_destroy<RigidBodyComponent>().connect<&PhysicsSystem::OnDestroyRigidBody>(this);
		registry->on_construct<RigidBodyComponent>().connect<&PhysicsSystem::OnConstructRigidBody>(this);

		InitPhysics(scene);

		mPhysicsComponentTransformUpdateObs.connect(*registry, entt::collector.update<Transform>().where<RigidBodyComponent>());
		mPhysicsComponentTransformGroupObs.connect(*registry, entt::collector.group<Transform, RigidBodyComponent>());
	}

	void PhysicsSystem::CopyBulletTransformFromTransform(entt::registry& reg, entt::entity e) {
		auto& transform = reg.get<Transform>(e);
		auto& rb = reg.get<RigidBodyComponent>(e);

		btTransform btTrans = transform.ToBullet();

		auto motionState = rb->getMotionState();
		if (motionState) {
			motionState->setWorldTransform(btTrans);
		}
		rb->setWorldTransform(btTrans);
		rb->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
		rb->setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));
		rb->activate();
	}

	void PhysicsSystem::CopyBulletTransformToCache(RigidBodyComponent& rb, 
		entt::registry& registry, entt::entity e) {
		auto cache = registry.try_get<MatrixTransformCache>(e);
		if (cache) {
			auto ms = rb->getMotionState();
			if (ms) {
				btTransform transform;
				ms->getWorldTransform(transform);
				transform.getOpenGLMatrix(&cache->mCache[0][0]);
			}
		}
	}

	void PhysicsSystem::OnSceneUpdate(const UpdateEvent& e) {
		auto registry = e.mSender->GetRegistry();

		for (auto entity : mPhysicsComponentTransformGroupObs) {
			CopyBulletTransformFromTransform(*registry, entity);
		}

		for (auto entity : mPhysicsComponentTransformUpdateObs) {
			CopyBulletTransformFromTransform(*registry, entity);
		}

		mPhysicsComponentTransformGroupObs.clear();
		mPhysicsComponentTransformUpdateObs.clear();

		mWorld->stepSimulation(1.0f / 60.0f);

		auto view = registry->view<RigidBodyComponent>();

		for (auto ent : view) {
			auto& rb = view.get<RigidBodyComponent>(ent);

			if (rb->isActive()) {
				CopyBulletTransformToCache(rb, *registry, ent);
			}
		}
	}

	void PhysicsSystem::OnSceneBegin(const SceneBeginEvent& args) {
		
	}

	void PhysicsSystem::OnFrameBegin(const FrameBeginEvent& args) {
		
	}

	void PhysicsSystem::OnDestroyRigidBody(entt::registry& reg, entt::entity e) {
		auto component = reg.get<RigidBodyComponent>(e);
		mWorld->removeRigidBody(component);

		auto ms = component->getMotionState();

		if (ms) {
			delete ms;
		}

		delete component.RawPtr();
	}

	void PhysicsSystem::OnConstructRigidBody(entt::registry& reg, entt::entity e) {
		auto& rb = reg.get<RigidBodyComponent>(e);
		mWorld->addRigidBody(rb);
	}

	void PhysicsSystem::Shutdown(Scene* scene) {
		auto registry = scene->GetRegistry();

		mPhysicsComponentTransformGroupObs.disconnect();
		mPhysicsComponentTransformUpdateObs.disconnect();

		KillPhysics(scene);

		registry->on_destroy<RigidBodyComponent>().disconnect<&PhysicsSystem::OnDestroyRigidBody>(this);
		registry->on_construct<RigidBodyComponent>().disconnect<&PhysicsSystem::OnConstructRigidBody>(this);
	}
}