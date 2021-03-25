#include <Engine/Engine2D/Renderer2D.hpp>
#include <Engine/Engine2D/Sprite.hpp>
#include <Engine/Engine2D/Tilemap.hpp>
#include <Engine/Components/Transform.hpp>
#include <Engine/Camera.hpp>

#include <algorithm>

namespace Morpheus {
	struct Transform2D {
		DG::float3 mPosition;
		DG::float2 mScale;
		float mRotation;	
	};

	struct SpriteRenderRequest {
		Transform2D mTransform;
		SpriteComponent* mSprite;
		RenderLayer2DComponent* mLayer;
	};

	void Renderer2D::RequestConfiguration(DG::EngineD3D11CreateInfo* info) {
	}

	void Renderer2D::RequestConfiguration(DG::EngineD3D12CreateInfo* info) {
	}

	void Renderer2D::RequestConfiguration(DG::EngineGLCreateInfo* info) {
	}

	void Renderer2D::RequestConfiguration(DG::EngineVkCreateInfo* info) {
	}

	void Renderer2D::RequestConfiguration(DG::EngineMtlCreateInfo* info) {
	}

	void Renderer2D::Initialize(Engine* engine) {
		mEngine = engine;
		mGlobals.Initialize(engine->GetDevice());
		mDefaultSpriteBatch.reset(new SpriteBatch(engine->GetDevice(), engine->GetResourceManager()));
	}

	void Renderer2D::InitializeSystems(Scene* scene) {
	}

	void Renderer2D::Render(Scene* scene, EntityNode cameraNode, const RenderPassTargets& targets) {
		float rgba[4] = { 0.5f, 0.5f, 0.5f, 1.0f };

		auto context = mEngine->GetImmediateContext();

		DG::ITextureView* rtv = targets.mColorOutputs[0];
		context->SetRenderTargets(1, &rtv, targets.mDepthOutput, 
			DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		context->ClearRenderTarget(targets.mColorOutputs[0], rgba,
			DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		context->ClearDepthStencil(targets.mDepthOutput, 
			DG::CLEAR_DEPTH_FLAG, 1.f, 0, 
			DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		if (scene) {
			auto camera = cameraNode.TryGet<Camera>();
			auto cameraTransformCache = cameraNode.TryGet<MatrixTransformCache>();

			auto context = mEngine->GetImmediateContext();
			auto projection = camera->GetProjection(mEngine);
			auto swapChain = mEngine->GetSwapChain();
			auto& swapChainDesc = swapChain->GetDesc();
			DG::float2 viewportSize((float)swapChainDesc.Width, (float)swapChainDesc.Height);

			WriteRenderGlobalsData(&mGlobals, context, viewportSize, camera,
				projection,
				(cameraTransformCache == nullptr ? nullptr : &cameraTransformCache->mCache),
				nullptr);

			auto registry = scene->GetRegistry();

			auto layerView = registry->view<RenderLayer2DComponent>();

			RenderLayer2DComponent defaultLayer;
			defaultLayer.mOrder = 0;
			defaultLayer.mSorting = LayerSorting2D::NO_SORTING;
			defaultLayer.mId = -1;

			std::unordered_map<int, RenderLayer2DComponent*> renderLayers(layerView.size());
			for (auto en : layerView) {
				auto& renderLayer = layerView.get<RenderLayer2DComponent>(en);
				renderLayers.emplace(renderLayer.mId, &renderLayer);
			}

			auto spriteView = registry->view<SpriteComponent, MatrixTransformCache>();

			std::vector<SpriteRenderRequest> visibleSprites;
			visibleSprites.reserve(spriteView.size_hint());

			for (auto en : spriteView) {
				auto& sprite = spriteView.get<SpriteComponent>(en);
				auto& transformCache = spriteView.get<MatrixTransformCache>(en);
				auto& matrix = transformCache.mCache;

				// If it's not loaded, don't display it
				if (!sprite.mTextureResource->IsLoaded())
					continue;

				// Calculate Transform2D from Cached Matrix Transform
				Transform2D transform;
				transform.mPosition.x = matrix.m30;
				transform.mPosition.y = matrix.m31;
				transform.mPosition.z = matrix.m32;
				transform.mScale.x = sqrt(matrix.m00 * matrix.m00 + matrix.m01 * matrix.m01);
				transform.mScale.y = sqrt(matrix.m10 * matrix.m10 + matrix.m11 * matrix.m11);
	
				float costheta = matrix.m00 / transform.mScale.x;
				float sintheta = matrix.m01 / transform.mScale.y;
				transform.mRotation = atan2(sintheta, costheta);

				SpriteRenderRequest request;

				auto it = renderLayers.find(sprite.mRenderLayer);
				if (it != renderLayers.end()) {
					request.mLayer = it->second;
				} else {
					request.mLayer = &defaultLayer;
				}

				request.mTransform = transform;
				request.mSprite = &sprite;

				visibleSprites.emplace_back(request);
			}

			std::sort(visibleSprites.begin(), visibleSprites.end(), 
				[](const SpriteRenderRequest& r1, const SpriteRenderRequest& r2) {
					return r1.mLayer->mOrder < r2.mLayer->mOrder;
			});

			mDefaultSpriteBatch->Begin(context);

			for (auto sprite : visibleSprites) {
				DG::float2 spriteSize;
				spriteSize.x = sprite.mTransform.mScale.x * sprite.mSprite->mRect.mSize.x;
				spriteSize.y = sprite.mTransform.mScale.y * sprite.mSprite->mRect.mSize.y;

				mDefaultSpriteBatch->Draw(
					sprite.mSprite->mTextureResource, 
					sprite.mTransform.mPosition,
					spriteSize,
					sprite.mSprite->mRect,
					sprite.mSprite->mOrigin,
					sprite.mTransform.mRotation,
					sprite.mSprite->mColor);
			}

			mDefaultSpriteBatch->End();
		}
	}

	DG::IRenderDevice* Renderer2D::GetDevice() {
		return mEngine->GetDevice();
	}

	DG::IDeviceContext* Renderer2D::GetImmediateContext() {
		return mEngine->GetImmediateContext();
	}

	// This buffer will be bound as a constant to all pipelines
	DG::IBuffer* Renderer2D::GetGlobalsBuffer() {
		return mGlobals.Get();
	}

	DG::FILTER_TYPE Renderer2D::GetDefaultFilter() const {
		return DG::FILTER_TYPE_LINEAR;
	}

	uint Renderer2D::GetMaxAnisotropy() const {
		return 1;
	}

	uint Renderer2D::GetMSAASamples() const {
		return 1;
	}

	uint Renderer2D::GetMaxRenderThreadCount() const {
		return 1;
	}

	void Renderer2D::OnWindowResized(uint width, uint height) {
	}

	DG::TEXTURE_FORMAT Renderer2D::GetBackbufferColorFormat() const {
		return mEngine->GetSwapChain()->GetDesc().ColorBufferFormat;
	}

	DG::TEXTURE_FORMAT Renderer2D::GetBackbufferDepthFormat() const {
		return mEngine->GetSwapChain()->GetDesc().DepthBufferFormat;
	}

	DG::TEXTURE_FORMAT Renderer2D::GetIntermediateFramebufferFormat() const {
		return DG::TEX_FORMAT_UNKNOWN;
	}

	DG::TEXTURE_FORMAT Renderer2D::GetIntermediateDepthbufferFormat() const {
		return DG::TEX_FORMAT_UNKNOWN;
	}

	DG::ITextureView* Renderer2D::GetLUTShaderResourceView() {
		return nullptr;
	}

	bool Renderer2D::GetUseSHIrradiance() const {
		return false;
	}

	bool Renderer2D::GetUseIBL() const {
		return false;
	}
}