#include <Engine/Engine2D/Physics2D.hpp>
#include <Engine/Components/Transform.hpp>
#include <Engine/Engine2D/Renderer2D.hpp>

#include <box2d/b2_body.h>

namespace Morpheus {
	void PhysicsSystem2D::Startup(Scene* scene) {

		auto registry = scene->GetRegistry();

		mWorld.SetGravity(b2Vec2(0.0f, -10.0f));

		registry->on_destroy<PhysicsBody2DComponent>().connect<&PhysicsSystem2D::OnDestroyRigidBody>(this);

		mPhysicsComponentTransformUpdateObs.connect(*registry, entt::collector.update<Transform>().where<PhysicsBody2DComponent>());
		mPhysicsComponentTransformGroupObs.connect(*registry, entt::collector.group<Transform, PhysicsBody2DComponent>());
	}

	void PhysicsSystem2D::Shutdown(Scene* scene) {
		auto registry = scene->GetRegistry();

		mPhysicsComponentTransformGroupObs.disconnect();
		mPhysicsComponentTransformUpdateObs.disconnect();

		registry->on_destroy<PhysicsBody2DComponent>().disconnect<&PhysicsSystem2D::OnDestroyRigidBody>(this);
	}

	void PhysicsSystem2D::CopyBox2DTransformFromTransform(entt::registry& reg, entt::entity e) {
		auto& transform = reg.get<Transform>(e);
		auto& rb = reg.get<PhysicsBody2DComponent>(e);

		Transform2D transform2D(transform);

		rb->SetTransform(b2Vec2(transform2D.mPosition.x, transform2D.mPosition.y), transform2D.mRotation);
		rb->SetAwake(true);
	}

	void PhysicsSystem2D::CopyBox2DTransformToCache(PhysicsBody2DComponent& rb, entt::registry& reg, entt::entity e) {
		auto cache = reg.try_get<MatrixTransformCache>(e);

		if (cache) {
			auto& transform = reg.get<Transform>(e);
			auto& rb = reg.get<PhysicsBody2DComponent>(e);

			auto scale = transform.GetScale();
			auto transform2d = rb->GetTransform();

			DG::float4x4 matrix = DG::float4x4::Identity();

			matrix.m00 = transform2d.q.c * scale.x;
			matrix.m11 = transform2d.q.c * scale.y;
			matrix.m01 = transform2d.q.s * scale.x;
			matrix.m10 = -transform2d.q.s * scale.y;

			matrix.m30 = transform2d.p.x;
			matrix.m31 = transform2d.p.y;
			matrix.m32 = transform.GetTranslation().z;

			cache->mCache = matrix;
		}
	}

	void PhysicsSystem2D::OnDestroyRigidBody(entt::registry& reg, entt::entity e) {
		auto component = reg.get<PhysicsBody2DComponent>(e);
		mWorld.DestroyBody(component.RawPtr());
	}

	void PhysicsSystem2D::OnSceneBegin(const SceneBeginEvent& args) {
	}

	void PhysicsSystem2D::OnFrameBegin(const FrameBeginEvent& args) {
	}

	void PhysicsSystem2D::OnSceneUpdate(const UpdateEvent& e) {

		auto registry = e.mSender->GetRegistry();

		for (auto entity : mPhysicsComponentTransformGroupObs) {
			CopyBox2DTransformFromTransform(*registry, entity);
		}

		for (auto entity : mPhysicsComponentTransformUpdateObs) {
			CopyBox2DTransformFromTransform(*registry, entity);
		}

		mPhysicsComponentTransformGroupObs.clear();
		mPhysicsComponentTransformUpdateObs.clear();

		int32 velocityIterations = 6;
		int32 positionIterations = 2;
		mWorld.Step(1.0f / 60.0f, velocityIterations, positionIterations);

		auto view = registry->view<PhysicsBody2DComponent>();

		for (auto ent : view) {
			auto& rb = view.get<PhysicsBody2DComponent>(ent);

			if (rb->IsAwake()) {
				CopyBox2DTransformToCache(rb, *registry, ent);
			}
		}
	}
}