#pragma once

#include <Engine/Components/Transform.hpp>
#include <Engine/Frame.hpp>

namespace Morpheus {

	struct RendererTransformCache {
		DG::float4x4 mCache;

		inline void Set(const Transform& transform, const RendererTransformCache& parent) {
			mCache = transform.ToMatrix() * parent.mCache;
		}

		inline void Set(const Transform& transform, const DG::float4x4& parent) {
			mCache = transform.ToMatrix() * parent;
		}

		inline void Set(const Transform& transform) {
			mCache = transform.ToMatrix();
		}

		inline RendererTransformCache(const Transform& transform, const DG::float4x4& parent) :
			mCache(transform.ToMatrix() * parent) {
		}

		inline RendererTransformCache(const Transform& transform, const RendererTransformCache& parent) :
			mCache(transform.ToMatrix() * parent.mCache) {
		}

		inline RendererTransformCache(const Transform& transform) :
			mCache(transform.ToMatrix()) {
		}

		inline RendererTransformCache() :
			mCache(DG::float4x4::Identity()) {
			}
	};

	class TransformCacheUpdater {
	private:
		entt::observer mTransformUpdateObs;
		entt::observer mNewTransformObs;
		Frame* mFrame;
	
	public:
		inline void SetFrame(Frame* frame) {
			mTransformUpdateObs.clear();
			mNewTransformObs.clear();

			mTransformUpdateObs.connect(frame->mRegistry, 
				entt::collector.update<Transform>());
			mNewTransformObs.connect(frame->mRegistry, 
				entt::collector.group<Transform>(entt::exclude<RendererTransformCache>));
				
			mFrame = frame;
		}

		inline TransformCacheUpdater(Frame* frame) {
			SetFrame(frame);
		}

		inline TransformCacheUpdater() {
		}

		void UpdateDescendants(entt::entity node, const DG::float4x4& matrix);
		entt::entity FindTransformParent(entt::entity node);
		void UpdateAll();
		void Update(entt::entity node);
		void UpdateChanges();
	};
}