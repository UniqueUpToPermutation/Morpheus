#pragma once

#include <Engine/Renderer.hpp>
#include <Engine/DynamicGlobalsBuffer.hpp>
#include <Engine/SpriteBatch.hpp>
#include <Engine/RendererGlobalsData.hpp>
#include <Engine/Components/Transform.hpp>
#include <Engine/Engine2D/Base2D.hpp>

namespace Morpheus {
	enum class LayerSorting2D {
		NO_SORTING,
		SORT_BY_TEXTURE,
		SORT_BY_Y_INCREASING,
		SORT_BY_Y_DECREASING
	};
	
	struct RenderLayer2DComponent {
		RenderLayerId mId;
		int mOrder;
		LayerSorting2D mSorting;
		std::string mName;
	};

	struct Transform2D {
		DG::float3 mPosition;
		DG::float2 mScale;
		float mRotation;

		void From(const DG::float4x4& matrix);
		void From(const Transform& transform);
		DG::float4 Apply(const DG::float4& vec) const;
		DG::float4 ApplyInverse(const DG::float4& vec) const;

		inline Transform2D() {
		}

		inline Transform2D(const DG::float4x4& matrix) {
			From(matrix);
		}

		inline Transform2D(const Transform& transform) {
			From(transform);
		}
	};

	struct SpriteComponent;
	struct TilemapComponent;

	struct SpriteRenderRequest {
		Transform2D mTransform;
		SpriteComponent* mSprite;
		RenderLayer2DComponent* mRenderLayer;
	};

	struct TilemapLayerRenderRequest {
		TilemapComponent* mTilemap;
		int mTilemapLayerId;
		RenderLayer2DComponent* mRenderLayer;
		Transform2D mTransform;
	};

	struct Layer2DRenderParams {
		std::vector<SpriteRenderRequest>::iterator mSpriteBegin;
		std::vector<SpriteRenderRequest>::iterator mSpriteEnd;
		std::vector<TilemapLayerRenderRequest>::iterator mTilemapLayerBegin;
		std::vector<TilemapLayerRenderRequest>::iterator mTilemapLayerEnd;
		RenderLayer2DComponent* mRenderLayer;
	};

	class Renderer2D : public IRendererOld {
	private:
		DynamicGlobalsBuffer<RendererGlobalData> mGlobals;
		Engine* mEngine;
		std::unique_ptr<SpriteBatch> mDefaultSpriteBatch;

		void RenderLayer(DG::IDeviceContext* context, const Layer2DRenderParams& params);
		void ConvertTilemapLayerToSpriteCalls(const TilemapLayerRenderRequest& layer, std::vector<SpriteBatchCall3D>& sbCalls);

	public:
		void RequestConfiguration(DG::EngineD3D11CreateInfo* info) override;
		void RequestConfiguration(DG::EngineD3D12CreateInfo* info) override;
		void RequestConfiguration(DG::EngineGLCreateInfo* info) override;
		void RequestConfiguration(DG::EngineVkCreateInfo* info) override;
		void RequestConfiguration(DG::EngineMtlCreateInfo* info) override;

		void Initialize(Engine* engine) override;
		void InitializeSystems(Scene* scene) override;

		void Render(Scene* scene, EntityNode cameraNode, const RenderPassTargets& targets) override;

		DG::IRenderDevice* GetDevice() override;
		DG::IDeviceContext* GetImmediateContext() override;

		// This buffer will be bound as a constant to all pipelines
		DG::IBuffer* GetGlobalsBuffer() override;
		DG::FILTER_TYPE GetDefaultFilter() const override;
		uint GetMaxAnisotropy() const override;
		uint GetMSAASamples() const override;
		uint GetMaxRenderThreadCount() const override;

		void OnWindowResized(uint width, uint height) override;

		DG::TEXTURE_FORMAT GetBackbufferColorFormat() const override;
		DG::TEXTURE_FORMAT GetBackbufferDepthFormat() const override;

		DG::TEXTURE_FORMAT GetIntermediateFramebufferFormat() const override;
		DG::TEXTURE_FORMAT GetIntermediateDepthbufferFormat() const override;

		DG::ITextureView* GetLUTShaderResourceView() override;
		bool GetUseSHIrradiance() const override;
		bool GetUseIBL() const override;
	};
}