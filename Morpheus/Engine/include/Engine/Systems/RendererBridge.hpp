/*
*	Description: Provides an interface between a scene and a renderer.
*/

#pragma once

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"

#include <Engine/Entity.hpp>
#include <Engine/Scene.hpp>
#include <Engine/Components/Transform.hpp>
#include <Engine/Components/ResourceComponents.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/Resource.hpp>

namespace DG = Diligent;

namespace Morpheus {
	struct FrameBeginEvent {
		Scene* mScene;
		IRenderer* mRenderer;
	};

	class RendererBridge : public ISystem {
	public:
	typedef entt::basic_group<
		entt::entity,
		entt::exclude_t<>,
		entt::get_t<>,
		MatrixTransformCache,
		MaterialComponent,
		GeometryComponent> StaticMeshGroupType;

	private:
		IRenderer* mRenderer;
		ResourceManager* mResourceManager;

		entt::observer mTransformNoCacheObs;
		entt::observer mTransformUpdateObs;
		entt::observer mCacheNoTransformObs;

		std::unique_ptr<StaticMeshGroupType> mRenderableGroup;

	public:
		inline RendererBridge(
			IRenderer* renderer,
			ResourceManager* resources) : 
			mRenderer(renderer), 
			mResourceManager(resources) {
		}

		void Startup(Scene* scene) override;
		void Shutdown(Scene* scene) override;

		void OnSceneBegin(const SceneBeginEvent& args);
		void OnFrameBegin(const FrameBeginEvent& args);

		void UpdateDescendants(EntityNode node, const DG::float4x4& matrix);
		MatrixTransformCache& UpdateCache(EntityNode node, 
			const Transform& transform,
			bool bUpdateDescendants);
		EntityNode FindTransformParent(EntityNode node);

		inline decltype(RendererBridge::mRenderableGroup)& GetRenderableGroup() {
			return mRenderableGroup;
		}
	};
}