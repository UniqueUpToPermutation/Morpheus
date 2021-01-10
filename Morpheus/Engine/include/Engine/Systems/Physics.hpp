#pragma once

#include <Engine/Scene.hpp>
#include <Engine/Components/PhysicsComponents.hpp>
namespace Morpheus {
	class PhysicsSystem : public ISystem {
	private:
		btDynamicsWorld* mWorld = nullptr;
		btBroadphaseInterface* mBroadphaseInterface = nullptr;
		btCollisionConfiguration* mCollisionConfiguration = nullptr;
		btCollisionDispatcher* mCollisionDispatcher = nullptr;
		btConstraintSolver* mConstaintSolver = nullptr;

		void InitPhysics(Scene* scene);
		void KillPhysics(Scene* scene);

	public:
		void Startup(Scene* scene) override;
		void Shutdown(Scene* scene) override;

		void OnUpdateRigidBody(entt::registry& reg, entt::entity e);
		void OnConstructRigidBody(entt::registry& reg, entt::entity e);
		void OnDestroyRigidBody(entt::registry& reg, entt::entity e);

		void OnSceneUpdate(const UpdateEvent& e);

		inline btDynamicsWorld* GetWorld() {
			return mWorld;
		}

		inline const btDynamicsWorld* GetWorld() const {
			return mWorld;
		}

		inline btBroadphaseInterface* GetBroadphase() {
			return mBroadphaseInterface;
		}

		inline const btBroadphaseInterface* GetBroadphase() const {
			return mBroadphaseInterface;
		}

		inline btCollisionConfiguration* GetCollisionConfig() {
			return mCollisionConfiguration;
		}

		inline const btCollisionConfiguration* GetCollisionConfig() const {
			return mCollisionConfiguration;
		}

		inline btCollisionDispatcher* GetCollisionDispatcher() {
			return mCollisionDispatcher;
		}

		inline const btCollisionDispatcher* GetCollisionDispatcher() const {
			return mCollisionDispatcher;
		}

		inline btConstraintSolver* GetConstraintSolver() {
			return mConstaintSolver;
		}

		inline const btConstraintSolver* GetConstraintSolver() const {
			return mConstaintSolver;
		}
	};
}