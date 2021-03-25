#pragma once

#include <Engine/Renderer.hpp>
#include <Engine/DynamicGlobalsBuffer.hpp>
#include <Engine/SpriteBatch.hpp>
#include <Engine/RendererGlobalsData.hpp>

namespace Morpheus {
	enum class LayerSorting2D {
		NO_SORTING,
		SORT_BY_TEXTURE
	};
	
	struct RenderLayer2DComponent {
		int mId;
		int mOrder;
		LayerSorting2D mSorting;
		std::string mName;
	};

	class Renderer2D : public IRenderer {
	private:
		DynamicGlobalsBuffer<RendererGlobalData> mGlobals;
		Engine* mEngine;
		std::unique_ptr<SpriteBatch> mDefaultSpriteBatch;

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