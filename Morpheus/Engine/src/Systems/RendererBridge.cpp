#include <Engine/Systems/RendererBridge.hpp>
#include <Engine/Transform.hpp>
#include <Engine/Skybox.hpp>
#include <Engine/LightProbe.hpp>
#include <Engine/Brdf.hpp>

#include <Engine/MaterialResource.hpp>
#include <Engine/GeometryResource.hpp>

namespace Morpheus {
	void RendererBridge::Startup(Scene* scene) {
		scene->GetDispatcher()->sink<SceneBeginEvent>().connect<&RendererBridge::OnSceneBegin>(this);
		scene->GetDispatcher()->sink<FrameBeginEvent>().connect<&RendererBridge::OnFrameBegin>(this);

		mRenderableGroup.reset(new StaticMeshGroupType(
			scene->GetRegistry()->group<
			MatrixTransformCache,
			MaterialComponent,
			GeometryComponent>()));
	}

	void RendererBridge::OnSceneBegin(const SceneBeginEvent& args) {
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

		auto skybox_view = registry->view<SkyboxComponent>();

		auto textureCache = mResourceManager->GetCache<TextureResource>();
		auto device = mRenderer->GetDevice();
		auto immediateContext = mRenderer->GetImmediateContext();

		if (skybox_view.size() > 1) {
			std::cout << "Warning: multiple skyboxes detected!" << std::endl;
		}

		for (auto entity : skybox_view) {
			auto& skybox = skybox_view.get<SkyboxComponent>(entity);

			// Build light probe if this skybox has none
			auto lightProbe = registry->try_get<LightProbe>(entity);
			if (!lightProbe) {

				std::cout << "Warning: skybox detected without light probe! Running light probe processor..." << std::endl;

				LightProbeProcessor processor(device);
				processor.Initialize(mResourceManager, 
					DG::TEX_FORMAT_RGBA16_FLOAT,
					DG::TEX_FORMAT_RGBA16_FLOAT);

				LightProbe newProbe;
				
				if (mRenderer->GetUseSHIrradiance()) {
					newProbe = processor.ComputeLightProbeSH(device,
						immediateContext, textureCache,
						skybox.GetCubemap()->GetShaderView());
				} else {
					newProbe = processor.ComputeLightProbe(device,
						immediateContext, textureCache, 
						skybox.GetCubemap()->GetShaderView());
				}
				
				// Add light probe to skybox
				registry->emplace<LightProbe>(entity, newProbe);
			}
		}
	}

	EntityNode RendererBridge::FindTransformParent(EntityNode node) {
		auto result = node.GetParent();
		for (; result.IsValid(); result = result.GetParent()) {
			if (result.Has<Transform>()) {
				return result;
			}
		}
		return result;
	}

	void RendererBridge::UpdateDescendants(EntityNode node, 
		const DG::float4x4& matrix) {
		
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

	void RendererBridge::OnFrameBegin(const FrameBeginEvent& args) {
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

		auto group = mRenderableGroup.get();
		mRenderableGroup->sort([&group](
			const entt::entity lhs, 
			const entt::entity rhs) {
			
			auto lhsMaterial = group->get<MaterialComponent>(lhs).RawPtr();
			auto rhsMaterial = group->get<MaterialComponent>(rhs).RawPtr();

			auto lhsGeometry = group->get<GeometryComponent>(lhs).RawPtr();
			auto rhsGeometry = group->get<GeometryComponent>(rhs).RawPtr();

			if (lhsMaterial == rhsMaterial) {
				return lhsGeometry < rhsGeometry;
			} else {
				return lhsMaterial < rhsMaterial;
			}
		});
	}

	MatrixTransformCache& RendererBridge::UpdateCache(EntityNode node, 
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

	void RendererBridge::Shutdown(Scene* scene) {
		auto registry = scene->GetRegistry();

		// Remove all cache stuff if there's anything there
		registry->clear<MatrixTransformCache>();

		// Unsubscribe to updates
		mTransformNoCacheObs.disconnect();
		mCacheNoTransformObs.disconnect();
		mTransformUpdateObs.disconnect();

		scene->GetDispatcher()->sink<SceneBeginEvent>()
			.disconnect<&RendererBridge::OnSceneBegin>(this);
		scene->GetDispatcher()->sink<FrameBeginEvent>()
			.disconnect<&RendererBridge::OnFrameBegin>(this);

		mRenderableGroup.release();
	}
}