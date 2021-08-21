#pragma once

#ifdef USE_BULLET

#include <Engine/Defines.hpp>
#include <Engine/Systems/System.hpp>
#include <Engine/Resources/Resource.hpp>
#include <Engine/Entity.hpp>
#include <Engine/Components/Transform.hpp>

#include "btBulletDynamicsCommon.h"

namespace Morpheus::Bullet {

	class IShapeResource : public IResource {
	public:
		virtual btCollisionShape* GetShape() = 0;
	};

	template <typename T>
	class ShapeResource : public IShapeResource {
	private:
		T mShape;

	public:
		inline ShapeResource(T&& shape) : mShape(std::move(shape)) {
		}

		btCollisionShape* GetShape() override {
			return &mShape;
		}
	};

	class RigidBody {
	private:
		std::shared_ptr<btRigidBody> mRigidBody;
		std::shared_ptr<btMotionState> mMotionState;
		Handle<IShapeResource> mShape;

	public:
		inline RigidBody(btRigidBody* body, 
			btMotionState* motion, 
			Handle<IShapeResource> shape) :
			mRigidBody(body),
			mMotionState(motion),
			mShape(std::move(shape)) {
		}

		inline btRigidBody* Get() const {
			return mRigidBody.get();
		}
	};

	class CollisionObject {
	private:
		std::shared_ptr<btCollisionObject> mObject;
		Handle<IShapeResource> mShape;

	public:
		inline CollisionObject(btCollisionObject* obj, Handle<IShapeResource> shape) :
			mObject(obj), mShape(std::move(shape)) {
		}

		inline btCollisionObject* Get() const {
			return mObject.get();
		}
	};

	class DynamicsWorld {
	private:
		std::shared_ptr<btDynamicsWorld> mWorld;
	
	public:
		inline DynamicsWorld(btDynamicsWorld* world) :
			mWorld(world) {
		}

		inline btDynamicsWorld* Get() const {
			return mWorld.get();
		}
	};

	struct TransformUpdateInjection {
		entt::entity mEntity;
		DG::float4x4 mTransform; 

		TransformUpdateInjection() = default;
		TransformUpdateInjection(
			entt::entity entity, 
			const btTransform& transform);
	};

	struct TransformUpdate {
		DG::float4x4 mTransform;

		TransformUpdate() = default;
		inline TransformUpdate(const TransformUpdateInjection& inj) :
			mTransform(inj.mTransform) {
		}
	};

	class PhysicsSystem : public ISystem {
	private:
		std::unique_ptr<btBroadphaseInterface>		mBroadphase	= nullptr;
		std::unique_ptr<btCollisionConfiguration>	mCollisionConfig = nullptr;
		std::unique_ptr<btCollisionDispatcher>		mDispatcher = nullptr;
		std::unique_ptr<btConstraintSolver>    		mSolver = nullptr;
		bool										bIsInitialized = false;

		std::vector<TransformUpdateInjection> 		mTransformUpdates;

		entt::observer mTransformUpdateObs;
		Frame* mCurrentFrame = nullptr;

		void Update(const TaskParams& e, const UpdateParams& params);
		void InjectTransforms(Frame* frame);

	public:
		Task Startup(SystemCollection& systems) override;
		bool IsInitialized() const override;
		void Shutdown() override;
		void NewFrame(Frame* frame) override;
		void OnAddedTo(SystemCollection& collection) override;

		inline btConstraintSolver* GetConstraintSolver() const {
			return mSolver.get();
		}

		inline btCollisionDispatcher* GetCollisionDispatcher() const {
			return mDispatcher.get();
		}

		inline btBroadphaseInterface* GetBroadphase() const {
			return mBroadphase.get();
		}

		inline btCollisionConfiguration* GetConfig() const {
			return mCollisionConfig.get();
		}
	};
}

#endif