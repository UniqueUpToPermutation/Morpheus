#pragma once

#include <Engine/Scene.hpp>
#include <Engine/Components/Transform.hpp>

namespace Morpheus {
	
	// Makes sure that all transforms 
	class TransformSystem : public ISystem {
	private:
		entt::observer mTransformNoCacheObs;
		entt::observer mTransformUpdateObs;
		entt::observer mCacheNoTransformObs;

	public:
		void Startup(Scene* scene) override;
		void Shutdown(Scene* scene) override;

		void OnSceneBegin(const SceneBeginEvent& args) override;
		void OnFrameBegin(const FrameBeginEvent& args) override;
		void OnSceneUpdate(const UpdateEvent& args) override;

		void UpdateDescendants(EntityNode node, const DG::float4x4& matrix);
		MatrixTransformCache& UpdateCache(EntityNode node, 
			const Transform& transform,
			bool bUpdateDescendants);
		EntityNode FindTransformParent(EntityNode node);
	};
}