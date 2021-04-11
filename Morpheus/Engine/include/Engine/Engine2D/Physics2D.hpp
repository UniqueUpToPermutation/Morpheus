#pragma once

#include <Engine/Scene.hpp>
#include <box2d/b2_world.h>

namespace Morpheus {
	typedef UniquePointerComponent<b2Body> PhysicsBody2DComponent;

	class PhysicsSystem2D : public ISystem {
	private:
		b2World mWorld;

		entt::observer mPhysicsComponentTransformUpdateObs;
		entt::observer mPhysicsComponentTransformGroupObs;

		void OnDestroyRigidBody(entt::registry& reg, entt::entity e);

		void CopyBox2DTransformFromTransform(entt::registry& reg, entt::entity e);
		void CopyBox2DTransformToCache(PhysicsBody2DComponent& body, entt::registry& reg, entt::entity e);

	public:
		void Startup(Scene* scene) override;
		void Shutdown(Scene* scene) override;
		void OnSceneBegin(const SceneBeginEvent& args) override;
		void OnFrameBegin(const FrameBeginEvent& args) override;
		void OnSceneUpdate(const UpdateEvent& e) override;

		inline b2World& GetWorld() {
			return mWorld;
		}

		inline const b2World& GetWorld() const {
			return mWorld;
		}
	};
}