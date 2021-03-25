#include <Engine/Systems/TransformSystem.hpp>

namespace Morpheus {
	void TransformSystem::Startup(Scene* scene) {
	}

	void TransformSystem::Shutdown(Scene* scene) {
		std::cout << "Shutting down transform system..." << std::endl;
		auto registry = scene->GetRegistry();

		// Remove all cache stuff if there's anything there
		registry->clear<MatrixTransformCache>();

		// Unsubscribe to updates
		mTransformNoCacheObs.disconnect();
		mCacheNoTransformObs.disconnect();
		mTransformUpdateObs.disconnect();
	}

	void TransformSystem::OnSceneBegin(const SceneBeginEvent& args) {
		auto registry = args.mSender->GetRegistry();

		// Create matrix caches for all transforms
		auto transformView = registry->view<Transform>();
		for (auto e : transformView) {
			UpdateCache(EntityNode(registry, e), 
				transformView.get<Transform>(e),
				false); 
		}

		// Observe future changes
		mTransformNoCacheObs.connect(*registry,
			entt::collector.group<Transform>(entt::exclude<MatrixTransformCache>));
		mCacheNoTransformObs.connect(*registry,
			entt::collector.group<MatrixTransformCache>(entt::exclude<Transform>));
		mTransformUpdateObs.connect(*registry,
			entt::collector.update<Transform>());
	}

	void TransformSystem::OnFrameBegin(const FrameBeginEvent& args) {
		auto registry = args.mScene->GetRegistry();

		// Update everything that has changed
		for (auto e : mTransformNoCacheObs) {
			EntityNode node(registry, e);
			UpdateCache(node, registry->get<Transform>(e), true);
		}

		// Update everything that has changed
		for (auto e : mTransformUpdateObs) {
			EntityNode node(registry, e);
			UpdateCache(node, registry->get<Transform>(e), true);
		}

		// Remove any caches with no transforms (for whatever reason)
		for (auto e : mCacheNoTransformObs) {
			registry->remove<MatrixTransformCache>(e);
		}

		// Clear observers
		mTransformNoCacheObs.clear();
		mTransformUpdateObs.clear();
		mCacheNoTransformObs.clear();
	}

	void TransformSystem::OnSceneUpdate(const UpdateEvent& args) {
	}

	void TransformSystem::UpdateDescendants(EntityNode node, const DG::float4x4& matrix) {
		for (auto child = node.GetFirstChild(); child.IsValid(); child = child.GetNext()) {
			auto transform = child.TryGet<Transform>();
			if (transform) {
				DG::float4x4 newMatrix = transform->ToMatrix() * matrix;
				child.AddOrReplace<MatrixTransformCache>(newMatrix);
				UpdateDescendants(child, newMatrix);
			} else {
				UpdateDescendants(child, matrix);
			}
		}
	}

	MatrixTransformCache& TransformSystem::UpdateCache(EntityNode node, 
		const Transform& transform,
		bool bUpdateDescendants) {
		auto parentTransform = FindTransformParent(node);

		auto matrix = transform.ToMatrix();

		if (parentTransform.IsValid() && 
			!parentTransform.Has<MatrixTransformCache>()) {
			auto& cache = UpdateCache(parentTransform, 
				parentTransform.Get<Transform>(),
				false);
			matrix = matrix * cache.mCache;
		}

		if (bUpdateDescendants)
			UpdateDescendants(node, matrix);

		return node.AddOrReplace<MatrixTransformCache>(matrix);
	}

	EntityNode TransformSystem::FindTransformParent(EntityNode node) {
		auto result = node.GetParent();
		for (; result.IsValid(); result = result.GetParent()) {
			if (result.Has<Transform>()) {
				return result;
			}
		}
		return result;
	}
}