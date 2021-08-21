#include <Engine/RendererTransformCache.hpp>
#include <Engine/Systems/BulletPhysics.hpp>

namespace Morpheus {

	void TransformCacheUpdater::SetFrame(Frame* frame) {
		if (mFrame) {
			mTransformUpdateObs.clear();
			mNewTransformObs.clear();

			mTransformUpdateObs.disconnect();
			mNewTransformObs.disconnect();
		}

		mTransformUpdateObs.connect(frame->Registry(), 
			entt::collector.update<Transform>());
		mNewTransformObs.connect(frame->Registry(), 
			entt::collector.group<Transform>(entt::exclude<RendererTransformCache>));
			
		mFrame = frame;
	}

	void TransformCacheUpdater::UpdateDescendants(entt::entity node, const DG::float4x4& matrix) {
		for (auto child = mFrame->GetFirstChild(node); child != entt::null; child = mFrame->GetNext(child)) {
			auto transform = mFrame->Registry().try_get<Transform>(child);
			if (transform) {
				auto result = mFrame->Registry().emplace_or_replace<RendererTransformCache>(child, *transform, matrix);
				UpdateDescendants(child, result.mCache);
			} else {
				UpdateDescendants(child, matrix);
			}
		}
	}

	entt::entity TransformCacheUpdater::FindTransformParent(entt::entity node) {
		auto result = mFrame->GetParent(node);
		for (; result != entt::null; result = mFrame->GetParent(result)) {
			if (mFrame->Registry().has<Transform>(result)) {
				return result;
			}
		}
		return entt::null;
	}

	void TransformCacheUpdater::UpdateAll() {
		auto transform = mFrame->Registry().try_get<Transform>(mFrame->GetRoot());
		if (transform) {
			auto cache = mFrame->Registry()
				.emplace_or_replace<RendererTransformCache>(mFrame->GetRoot(), *transform);
			UpdateDescendants(mFrame->GetRoot(), cache.mCache);
		} else {
			DG::float4x4 id(DG::float4x4::Identity());
			UpdateDescendants(mFrame->GetRoot(), id);
		}

		mTransformUpdateObs.clear();
		mNewTransformObs.clear();
	}

	void TransformCacheUpdater::Update(entt::entity node) {
		auto parent = FindTransformParent(node);

		if (parent == entt::null) {
			auto& transform = mFrame->Registry().get<Transform>(node);
			auto cache = mFrame->Registry().emplace_or_replace<RendererTransformCache>(node, transform);
			UpdateDescendants(node, cache.mCache);
		} else {
			auto parentCache = mFrame->Registry().try_get<RendererTransformCache>(parent);

			// If parent doesn't have transform cache, it will get caught eventually and descendents will be updated
			if (parentCache) {
				auto& transform = mFrame->Registry().get<Transform>(node);
				auto cache = mFrame->Registry().emplace_or_replace<RendererTransformCache>(node, transform, parentCache->mCache);
				UpdateDescendants(node, cache.mCache);
			}
		}
	}

	void TransformCacheUpdater::UpdateChanges() {
		for (auto e : mTransformUpdateObs) {
			Update(e);
		}

		for (auto e : mNewTransformObs) {
			Update(e);
		}

		mTransformUpdateObs.clear();
		mNewTransformObs.clear();

#ifdef USE_BULLET
		// Copy updates from Bullet physics
		auto bulletView = mFrame->Registry().view<Bullet::TransformUpdate>();

		for (auto entity : bulletView) {
			auto& transUpdate = bulletView.get<Bullet::TransformUpdate>(entity);
			mFrame->Registry().emplace_or_replace<RendererTransformCache>(entity,
				RendererTransformCache(transUpdate.mTransform));
		}
#endif
	}
}