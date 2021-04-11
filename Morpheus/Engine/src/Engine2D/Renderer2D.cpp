#include <Engine/Engine2D/Renderer2D.hpp>
#include <Engine/Engine2D/Sprite.hpp>
#include <Engine/Engine2D/Tilemap.hpp>
#include <Engine/Components/Transform.hpp>
#include <Engine/Camera.hpp>

#include <algorithm>

namespace Morpheus {	

	void Transform2D::From(const DG::float4x4& matrix) {
		mPosition.x = matrix.m30;
		mPosition.y = matrix.m31;
		mPosition.z = matrix.m32;
		mScale.x = sqrt(matrix.m00 * matrix.m00 + matrix.m01 * matrix.m01);
		mScale.y = sqrt(matrix.m10 * matrix.m10 + matrix.m11 * matrix.m11);

		float costheta = matrix.m00 / mScale.x;
		float sintheta = matrix.m01 / mScale.y;
		mRotation = atan2(sintheta, costheta);
	}

	void Transform2D::From(const Transform& transform) {
		From(transform.ToMatrix());
	}

	DG::float4 Transform2D::Apply(const DG::float4& vec) const {
		DG::float4 result = vec;
		result.x *= mScale.x;
		result.y *= mScale.y;

		float cosRotation = cos(mRotation);
		float sinRotation = sin(mRotation);

		float tempX = cosRotation * result.x - sinRotation * result.y;
		float tempY = sinRotation * result.x + cosRotation * result.y;

		result.x = tempX;
		result.y = tempY;

		result.x += result.w * mPosition.x;
		result.y += result.w * mPosition.y;
		result.z += result.w * mPosition.z;

		return result;
	}

	DG::float4 Transform2D::ApplyInverse(const DG::float4& vec) const {
		DG::float4 result = vec;

		result.x -= result.w * mPosition.x;
		result.y -= result.w * mPosition.y;
		result.z -= result.w * mPosition.z;

		float cosRotation = cos(mRotation);
		float sinRotation = -sin(mRotation);

		float tempX = cosRotation * result.x - sinRotation * result.y;
		float tempY = sinRotation * result.x + cosRotation * result.y;

		result.x /= mScale.x;
		result.y /= mScale.y;

		return result;
	}

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

	void Renderer2D::ConvertTilemapLayerToSpriteCalls(const TilemapLayerRenderRequest& layer, std::vector<SpriteBatchCall3D>& sbCalls) {
		auto& tilemapLayer = layer.mTilemap->mLayers[layer.mTilemapLayerId];

		DG::float4 origin(tilemapLayer.mLayerOffset.x, tilemapLayer.mLayerOffset.y, tilemapLayer.mLayerOffset.z, 1.0f);
		DG::float4 axisX(tilemapLayer.mTileAxisX.x, tilemapLayer.mTileAxisX.y, 0.0f, 0.0f);
		DG::float4 axisY(tilemapLayer.mTileAxisY.x, tilemapLayer.mTileAxisY.y, 0.0f, 0.0f);

		origin = layer.mTransform.Apply(origin);
		axisX = layer.mTransform.Apply(axisX);
		axisY = layer.mTransform.Apply(axisY);

		uint width = layer.mTilemap->mData.mLayerWidth;
		uint height = layer.mTilemap->mData.mLayerHeight;

		auto tileIdsPtr = &layer.mTilemap->mData.mTileIds[layer.mTilemapLayerId * width * height];
		
		DG::float4 color = DG::float4(1.0f, 1.0f, 1.0f, 1.0f);

		if (layer.mTilemap->mData.bHasMultipleTilesets) {
			auto tilesetIdPtr = &layer.mTilemap->mData.mTileTilesets[layer.mTilemapLayerId * width * height];
			if (layer.mTilemap->mData.bHasZOffsets) {
				auto zOffsetPtr = &layer.mTilemap->mData.mTileZOffsets[layer.mTilemapLayerId * width * height];
				for (uint y = 0; y < height; ++y) {
					for (uint x = 0; x < width; ++x, ++tileIdsPtr, ++tilesetIdPtr, ++zOffsetPtr) {
						if (*tileIdsPtr >= 0) {
							DG::float4 position = origin + (float)x * axisX + (float)y * axisY;
							position.z += *zOffsetPtr;

							auto& tileset = layer.mTilemap->mTilesets[*tilesetIdPtr];
							auto& subtile = tileset.mSubTiles[*tileIdsPtr];
							
							sbCalls.emplace_back(SpriteBatchCall3D{
								tileset.mTexture->GetTexture(), 
								position,
								tilemapLayer.mTileRenderSize,
								subtile,
								tileset.mTileOrigin,
								layer.mTransform.mRotation,
								color
							});
						}
					}
				}
			} else {
				for (uint y = 0; y < height; ++y) {
					for (uint x = 0; x < width; ++x, ++tileIdsPtr, ++tilesetIdPtr) {
						if (*tileIdsPtr >= 0) {
							DG::float4 position = origin + (float)x * axisX + (float)y * axisY;

							auto& tileset = layer.mTilemap->mTilesets[*tilesetIdPtr];
							auto& subtile = tileset.mSubTiles[*tileIdsPtr];
							
							sbCalls.emplace_back(SpriteBatchCall3D{
								tileset.mTexture->GetTexture(), 
								position,
								tilemapLayer.mTileRenderSize,
								subtile,
								tileset.mTileOrigin,
								layer.mTransform.mRotation,
								color
							});
						}
					}
				}
			}
		} else if (layer.mTilemap->mTilesets.size() > 0) {

			auto& tileset = layer.mTilemap->mTilesets[0];

			if (layer.mTilemap->mData.bHasZOffsets) {
				auto zOffsetPtr = &layer.mTilemap->mData.mTileZOffsets[layer.mTilemapLayerId * width * height];
				for (uint y = 0; y < height; ++y) {
					for (uint x = 0; x < width; ++x, ++tileIdsPtr, ++zOffsetPtr) {
						if (*tileIdsPtr >= 0) {
							DG::float4 position = origin + (float)x * axisX + (float)y * axisY;
							position.z += *zOffsetPtr;

							auto& subtile = tileset.mSubTiles[*tileIdsPtr];
							
							sbCalls.emplace_back(SpriteBatchCall3D{
								tileset.mTexture->GetTexture(), 
								position,
								tilemapLayer.mTileRenderSize,
								subtile,
								tileset.mTileOrigin,
								layer.mTransform.mRotation,
								color
							});
						}
					}
				}
			} else {
				for (uint y = 0; y < height; ++y) {
					for (uint x = 0; x < width; ++x, ++tileIdsPtr) {
						if (*tileIdsPtr >= 0) {
							DG::float4 position = origin + (float)x * axisX + (float)y * axisY;
							auto& subtile = tileset.mSubTiles[*tileIdsPtr];
							
							sbCalls.emplace_back(SpriteBatchCall3D{
								tileset.mTexture->GetTexture(), 
								position,
								tilemapLayer.mTileRenderSize,
								subtile,
								tileset.mTileOrigin,
								layer.mTransform.mRotation,
								color
							});
						}
					}
				}
			}
		}
	}

	void Renderer2D::RenderLayer(DG::IDeviceContext* context, const Layer2DRenderParams& params) {

		auto sortingType = params.mRenderLayer->mSorting;

		std::vector<SpriteBatchCall3D> sbCalls;
		sbCalls.reserve(params.mSpriteEnd - params.mSpriteBegin);

		// Render individual sprites
		for (auto currentSprite = params.mSpriteBegin; currentSprite != params.mSpriteEnd; ++currentSprite) {
			auto& sprite = *currentSprite;

			DG::float2 spriteSize;
			spriteSize.x = sprite.mTransform.mScale.x * sprite.mSprite->mRect.mSize.x;
			spriteSize.y = sprite.mTransform.mScale.y * sprite.mSprite->mRect.mSize.y;

			sbCalls.emplace_back(SpriteBatchCall3D{
				sprite.mSprite->mTextureResource->GetTexture(), 
				sprite.mTransform.mPosition,
				spriteSize,
				sprite.mSprite->mRect,
				sprite.mSprite->mOrigin,
				sprite.mTransform.mRotation,
				sprite.mSprite->mColor
			});
		}

		// Render tilemaps
		for (auto currentTilemapLayer = params.mTilemapLayerBegin; 
			currentTilemapLayer != params.mTilemapLayerEnd; 
			++currentTilemapLayer) {
			ConvertTilemapLayerToSpriteCalls(*currentTilemapLayer, sbCalls);
		}

		mDefaultSpriteBatch->Begin(context);

		switch (sortingType) {
		case LayerSorting2D::SORT_BY_TEXTURE:
			std::sort(sbCalls.begin(), sbCalls.end(), 
				[](const SpriteBatchCall3D& call1, const SpriteBatchCall3D& call2) {
					return call1.mTexture < call2.mTexture;
			});
			break;
		case LayerSorting2D::SORT_BY_Y_DECREASING:
			std::sort(sbCalls.begin(), sbCalls.end(), 
				[](const SpriteBatchCall3D& call1, const SpriteBatchCall3D& call2) {
					return call1.mPosition.y > call2.mPosition.y;
			});
			break;
		case LayerSorting2D::SORT_BY_Y_INCREASING:
			std::sort(sbCalls.begin(), sbCalls.end(), 
				[](const SpriteBatchCall3D& call1, const SpriteBatchCall3D& call2) {
					return call1.mPosition.y < call2.mPosition.y;
			});
			break;
		}

		mDefaultSpriteBatch->Draw(&sbCalls[0], sbCalls.size());

		mDefaultSpriteBatch->End();
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
			FrameBeginEvent frameEvt;
			frameEvt.mRenderer = this;
			frameEvt.mScene = scene;
			scene->BeginFrame(frameEvt);

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
			std::vector<RenderLayer2DComponent*> renderLayersByOrder;
			renderLayersByOrder.emplace_back(&defaultLayer);

			for (auto en : layerView) {
				auto& renderLayer = layerView.get<RenderLayer2DComponent>(en);
				renderLayers.emplace(renderLayer.mId, &renderLayer);
				renderLayersByOrder.emplace_back(&renderLayer);
			}

			auto spriteView = registry->view<SpriteComponent, MatrixTransformCache>();
			auto tilemapView = registry->view<TilemapComponent, MatrixTransformCache>();

			std::vector<SpriteRenderRequest> visibleSprites;
			std::vector<TilemapLayerRenderRequest> tilemapLayers;
			visibleSprites.reserve(spriteView.size_hint());

			std::vector<std::vector<SpriteRenderRequest>::iterator> visibleSpriteLayerBins;
			visibleSpriteLayerBins.reserve(renderLayers.size());
			std::vector<std::vector<TilemapLayerRenderRequest>::iterator> tilemapLayerBins;
			tilemapLayerBins.reserve(renderLayers.size());

			// Collect sprites
			for (auto en : spriteView) {
				auto& sprite = spriteView.get<SpriteComponent>(en);
				auto& matrix = spriteView.get<MatrixTransformCache>(en).mCache;

				// Calculate transform2D
				Transform2D transform(matrix);
				SpriteRenderRequest request;

				// Retrieve the correct render layer
				auto it = renderLayers.find(sprite.mRenderLayerId);
				if (it != renderLayers.end()) {
					request.mRenderLayer = it->second;
				} else {
					request.mRenderLayer = &defaultLayer;
				}

				request.mTransform = transform;
				request.mSprite = &sprite;

				visibleSprites.emplace_back(request);
			}

			// Collect tilemaps
			for (auto en : tilemapView) {
				auto& tilemap = tilemapView.get<TilemapComponent>(en);
				auto& matrix = tilemapView.get<MatrixTransformCache>(en).mCache;

				// Calculate transform2D
				Transform2D transform(matrix);
				TilemapLayerRenderRequest request;

				request.mTilemap = &tilemap;
				request.mTransform = transform;

				for (int i = 0; i < tilemap.mLayers.size(); ++i) {
					request.mTilemapLayerId = i;

					// Retrieve the correct render layer
					auto it = renderLayers.find(tilemap.mLayers[i].mRenderLayerId);
					if (it != renderLayers.end()) {
						request.mRenderLayer = it->second;
					} else {
						request.mRenderLayer = &defaultLayer;
					}

					tilemapLayers.emplace_back(request);
				}
			}

			// Sort sprites and tilemap layers by render layer
			std::sort(visibleSprites.begin(), visibleSprites.end(), 
				[](const SpriteRenderRequest& r1, const SpriteRenderRequest& r2) {
					return (r1.mRenderLayer->mOrder != r1.mRenderLayer->mOrder) ? 
						r1.mRenderLayer->mOrder < r2.mRenderLayer->mOrder :
						r1.mRenderLayer < r2.mRenderLayer;
			});
			std::sort(tilemapLayers.begin(), tilemapLayers.end(), 
				[](const TilemapLayerRenderRequest& r1, const TilemapLayerRenderRequest& r2) {
					return (r1.mRenderLayer->mOrder != r1.mRenderLayer->mOrder) ? 
						r1.mRenderLayer->mOrder < r2.mRenderLayer->mOrder :
						r1.mRenderLayer < r2.mRenderLayer;
			});
			std::sort(renderLayersByOrder.begin(), renderLayersByOrder.end(),
				[](RenderLayer2DComponent* layer1, RenderLayer2DComponent* layer2) {
					return (layer1->mOrder != layer2->mOrder) ? 
						layer1->mOrder < layer2->mOrder :
						layer1 < layer2;
			});

			std::vector<SpriteRenderRequest>::iterator currentSpriteRequest = visibleSprites.begin();
			std::vector<TilemapLayerRenderRequest>::iterator currentTilemapRequest = tilemapLayers.begin();
			
			// Compute layer starts / ends
			visibleSpriteLayerBins.emplace_back(currentSpriteRequest);
			tilemapLayerBins.emplace_back(currentTilemapRequest);
			for (auto layer : renderLayersByOrder) {
				while (currentSpriteRequest != visibleSprites.end() && currentSpriteRequest->mRenderLayer == layer)
					++currentSpriteRequest;
				while (currentTilemapRequest != tilemapLayers.end() && currentTilemapRequest->mRenderLayer == layer)
					++currentTilemapRequest;
				
				visibleSpriteLayerBins.emplace_back(currentSpriteRequest);
				tilemapLayerBins.emplace_back(currentTilemapRequest);
			}

			// Actually draw all of the layers
			for (int layerIdx = 0; layerIdx < renderLayersByOrder.size(); ++layerIdx) {
				Layer2DRenderParams params;
				params.mRenderLayer = renderLayersByOrder[layerIdx];
				params.mSpriteBegin = visibleSpriteLayerBins[layerIdx];
				params.mSpriteEnd = visibleSpriteLayerBins[layerIdx + 1];
				params.mTilemapLayerBegin = tilemapLayerBins[layerIdx];
				params.mTilemapLayerEnd = tilemapLayerBins[layerIdx + 1];

				RenderLayer(context, params);
			}
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