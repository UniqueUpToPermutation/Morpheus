#ifdef USE_BULLET

#include <Engine/Systems/BulletPhysics.hpp>
#include <Engine/Frame.hpp>
#include <Engine/Components/Transform.hpp>

#include "btBulletDynamicsCommon.h"

namespace Morpheus::Bullet {

	TransformUpdateInjection::TransformUpdateInjection(
		entt::entity entity, const btTransform& transform) :
		mEntity(entity) {
		transform.getOpenGLMatrix(mTransform.Data());
	}

	Task PhysicsSystem::Startup(SystemCollection& systems) {
		mBroadphase = std::make_unique<btDbvtBroadphase>();
		mCollisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
		mDispatcher = std::make_unique<btCollisionDispatcher>(mCollisionConfig.get());
		mSolver = std::make_unique<btSequentialImpulseConstraintSolver>();

		bIsInitialized = true;

		return Task();
	}

	bool PhysicsSystem::IsInitialized() const {
		return bIsInitialized;
	}

	void PhysicsSystem::Shutdown() {
		mSolver.reset();
		mDispatcher.reset();
		mCollisionConfig.reset();
		mBroadphase.reset();

		bIsInitialized = false;
	}

	void PhysicsSystem::NewFrame(Frame* frame) {
		if (mCurrentFrame) {
			mTransformUpdateObs.clear();
			mTransformUpdateObs.disconnect();
		}

		// Set transforms of rigid bodies
		auto rbView = frame->mRegistry.view<RigidBody>();
		for (auto ent : rbView) {
			assert(frame->mRegistry.try_get<Transform>(ent) != nullptr);
			auto& transform = frame->mRegistry.get<Transform>(ent);
			auto& rb = rbView.get<RigidBody>(ent);
		
			btTransform bulletTransform;
			transform.ToBullet(bulletTransform);
			rb.Get()->getMotionState()->setWorldTransform(bulletTransform);
			rb.Get()->activate();
		}

		// Set transforms of collision objects
		auto colView = frame->mRegistry.view<CollisionObject>();
		for (auto ent : colView) {
			assert(frame->mRegistry.try_get<Transform>(ent) != nullptr);
			auto& transform = frame->mRegistry.get<Transform>(ent);
			auto& col = colView.get<CollisionObject>(ent);
		
			btTransform bulletTransform;
			transform.ToBullet(bulletTransform);
			col.Get()->setWorldTransform(bulletTransform);
		}

		auto worldView = frame->mRegistry.view<DynamicsWorld>();

		if (worldView.size() > 0) {
			auto& world = worldView.get<DynamicsWorld>(worldView.front());

			auto rbView = frame->mRegistry.view<RigidBody>();
			auto collisionView = frame->mRegistry.view<CollisionObject>();

			for (auto ent : rbView) {
				auto& rb = rbView.get<RigidBody>(ent);
				world.Get()->addRigidBody(rb.Get());
			}

			for (auto ent : collisionView) {
				auto& col = collisionView.get<CollisionObject>(ent);
				world.Get()->addCollisionObject(col.Get());
			}
		}

		mTransformUpdateObs.connect(frame->mRegistry,
			entt::collector.update<Transform>());

		mCurrentFrame = frame;
		mTransformUpdates.clear();
	}

	void PhysicsSystem::OnAddedTo(SystemCollection& collection) {
		collection.AddUpdateTask([this]
			(const TaskParams& e, const UpdateParams& params) {
			Update(e, params);
		});

		collection.AddInjector(entt::type_id<TransformUpdate>(),
			[this](Frame* frame) {
				InjectTransforms(frame);
		});
	}

	void PhysicsSystem::Update(const TaskParams& e, 
		const UpdateParams& params) {
		
		// Update bullet transforms from Morpheus transforms
		for (auto ent : mTransformUpdateObs) {
			auto& transform = params.mFrame->mRegistry.get<Transform>(ent);
			auto rbPtr = params.mFrame->mRegistry.try_get<RigidBody>(ent);
			auto objPtr = params.mFrame->mRegistry.try_get<CollisionObject>(ent);

			if (rbPtr) {
				btTransform bulletTransform;
				transform.ToBullet(bulletTransform);
				rbPtr->Get()->getMotionState()->setWorldTransform(bulletTransform);
				rbPtr->Get()->activate();
			}

			if (objPtr) {
				btTransform bulletTransform;
				transform.ToBullet(bulletTransform);
				objPtr->Get()->setWorldTransform(bulletTransform);
			}
		}

		mTransformUpdateObs.clear();

		// Find all physics worlds and time step!
		auto worlds = params.mFrame->mRegistry.view<DynamicsWorld>();

		for (auto entity : worlds) {
			auto& world = worlds.get<DynamicsWorld>(entity);
			world.Get()->stepSimulation(0.01f);
		}

		auto rigidBodyView = params.mFrame->mRegistry.view<RigidBody>();

		for (auto entity : rigidBodyView) {
			auto& rb = rigidBodyView.get<RigidBody>(entity);

			assert(params.mFrame->mRegistry.try_get<Transform>(entity) != nullptr);
			auto& transform = params.mFrame->mRegistry.get<Transform>(entity);

			auto bulletRb = rb.Get();

			if (bulletRb->isActive()) {
				btTransform trans;
				bulletRb->getMotionState()->getWorldTransform(trans);
				mTransformUpdates.emplace_back(TransformUpdateInjection(
					entity, trans));
			}
		}
	}

	void PhysicsSystem::InjectTransforms(Frame* frame) {
		frame->mRegistry.clear<TransformUpdate>();
		
		for (auto& transforms : mTransformUpdates) {
			frame->mRegistry.emplace<TransformUpdate>(
				transforms.mEntity, 
				TransformUpdate(transforms));	
		}

		mTransformUpdates.clear();
	}
}

#endif