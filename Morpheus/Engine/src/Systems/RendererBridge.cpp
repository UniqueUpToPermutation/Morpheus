#include <Engine/Systems/RendererBridge.hpp>
#include <Engine/Components/Transform.hpp>
#include <Engine/Components/SkyboxComponent.hpp>
#include <Engine/Components/ResourceComponents.hpp>
#include <Engine/LightProbe.hpp>
#include <Engine/Brdf.hpp>
#include <Engine/Renderer.hpp>

#include <Engine/Resources/MaterialResource.hpp>
#include <Engine/Resources/GeometryResource.hpp>

namespace Morpheus {
	void DefaultRendererBridge::Startup(Scene* scene) {
		std::cout << "Initializing renderer-scene bridge..." << std::endl;

		mRenderableGroup.reset(new StaticMeshGroupType(
			scene->GetRegistry()->group<
			MatrixTransformCache,
			MaterialComponent,
			GeometryComponent>()));
	}

	void DefaultRendererBridge::OnSceneBegin(const SceneBeginEvent& args) {
		auto registry = args.mSender->GetRegistry();
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
				processor.Initialize(device,
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
				registry->emplace<LightProbe>(entity, std::move(newProbe));
			}
		}
	}

	void DefaultRendererBridge::OnSceneUpdate(const UpdateEvent& args) {
	}

	void DefaultRendererBridge::OnFrameBegin(const FrameBeginEvent& args) {
		auto registry = args.mScene->GetRegistry();

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

	void DefaultRendererBridge::Shutdown(Scene* scene) {

		std::cout << "Shutting down renderer-scene bridge..." << std::endl;
		auto registry = scene->GetRegistry();

		// Remove all cache stuff if there's anything there
		registry->clear<MatrixTransformCache>();

		// Unsubscribe to updates
		mTransformNoCacheObs.disconnect();
		mCacheNoTransformObs.disconnect();
		mTransformUpdateObs.disconnect();

		mRenderableGroup.release();
	}
}