#pragma once

#include <Engine/SceneHeirarchy.hpp>
#include <Engine/Engine.hpp>

namespace DG = Diligent;

namespace Morpheus {

	class SceneHeirarchy;
	class Camera;

	class IRenderCache {
	public:
		virtual ~IRenderCache() {
		}
	};

	class IRenderer {
	public:
		virtual ~IRenderer() {
		}
		virtual void RequestConfiguration(DG::EngineD3D11CreateInfo* info) = 0;
		virtual void RequestConfiguration(DG::EngineD3D12CreateInfo* info) = 0;
		virtual void RequestConfiguration(DG::EngineGLCreateInfo* info) = 0;
		virtual void RequestConfiguration(DG::EngineVkCreateInfo* info) = 0;
		virtual void RequestConfiguration(DG::EngineMtlCreateInfo* info) = 0;
		virtual void Initialize() = 0;
		virtual void Render(IRenderCache* cache, EntityNode cameraNode) = 0;

		virtual IRenderCache* BuildRenderCache(SceneHeirarchy* scene) = 0;

		// This buffer will be bound as a constant to all pipelines
		virtual DG::IBuffer* GetGlobalsBuffer() = 0;
		virtual DG::FILTER_TYPE GetDefaultFilter() const = 0;
		virtual uint GetMaxAnisotropy() const = 0;
		virtual uint GetMSAASamples() const = 0;

		virtual void OnWindowResized(uint width, uint height) = 0;

		virtual DG::TEXTURE_FORMAT GetIntermediateFramebufferFormat() const = 0;
		virtual DG::TEXTURE_FORMAT GetIntermediateDepthbufferFormat() const = 0;
		virtual DG::ITextureView* GetLUTShaderResourceView() = 0;
		virtual bool GetUseSHIrradiance() const = 0;
		virtual bool GetUseIBL() const = 0;
	};
}