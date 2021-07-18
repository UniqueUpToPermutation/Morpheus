#include <Engine/TransformCache.hpp>
#include <Engine/Frame.hpp>

namespace Morpheus {

	void TransformCacheUpdater::SetFrame(Frame* frame) {
		mTransformUpdateObs.clear();
		mNewTransformObs.clear();

		mTransformUpdateObs.connect(frame->mRegistry, 
			entt::collector.update<Transform>());
		mNewTransformObs.connect(frame->mRegistry, 
			entt::collector.group<Transform>(entt::exclude<TransformCache>));
			
		mFrame = frame;
	}

	void TransformCacheUpdater::UpdateDescendants(entt::entity node, const DG::float4x4& matrix) {
		for (auto child = mFrame->GetFirstChild(node); child != entt::null; child = mFrame->GetNext(child)) {
			auto transform = mFrame->mRegistry.try_get<Transform>(child);
			if (transform) {
				auto result = mFrame->mRegistry.emplace_or_replace<TransformCache>(child, *transform, matrix);
				UpdateDescendants(child, result.mCache);
			} else {
				UpdateDescendants(child, matrix);
			}
		}
	}

	entt::entity TransformCacheUpdater::FindTransformParent(entt::entity node) {
		auto result = mFrame->GetParent(node);
		for (; result != entt::null; result = mFrame->GetParent(result)) {
			if (mFrame->mRegistry.has<Transform>(result)) {
				return result;
			}
		}
		return entt::null;
	}

	void TransformCacheUpdater::UpdateAll() {
		auto transform = mFrame->mRegistry.try_get<Transform>(mFrame->mRoot);
		if (transform) {
			auto cache = mFrame->mRegistry.emplace_or_replace<TransformCache>(mFrame->mRoot, *transform);
			UpdateDescendants(mFrame->mRoot, cache.mCache);
		} else {
			DG::float4x4 id(DG::float4x4::Identity());
			UpdateDescendants(mFrame->mRoot, id);
		}

		mTransformUpdateObs.clear();
		mNewTransformObs.clear();
	}

	void TransformCacheUpdater::Update(entt::entity node) {
		auto parent = FindTransformParent(node);

		if (parent == entt::null) {
			auto& transform = mFrame->mRegistry.get<Transform>(node);
			auto cache = mFrame->mRegistry.emplace_or_replace<TransformCache>(node, transform);
			UpdateDescendants(node, cache.mCache);
		} else {
			auto parentCache = mFrame->mRegistry.try_get<TransformCache>(parent);

			// If parent doesn't have transform cache, it will get caught eventually and descendents will be updated
			if (parentCache) {
				auto& transform = mFrame->mRegistry.get<Transform>(node);
				auto cache = mFrame->mRegistry.emplace_or_replace<TransformCache>(node, transform, parentCache->mCache);
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
	}
}