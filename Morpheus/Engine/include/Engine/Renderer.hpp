#pragma once

#include <Engine/Engine.hpp>

namespace DG = Diligent;

namespace Morpheus {

	class SceneHeirarchy;

	class RenderCache {
	public:
		virtual ~RenderCache() {
		}
	};

	class Camera;

	class Renderer {
	public:
		virtual ~Renderer() {
		}
		virtual void RequestConfiguration(DG::EngineD3D11CreateInfo* info) = 0;
		virtual void RequestConfiguration(DG::EngineD3D12CreateInfo* info) = 0;
		virtual void RequestConfiguration(DG::EngineGLCreateInfo* info) = 0;
		virtual void RequestConfiguration(DG::EngineVkCreateInfo* info) = 0;
		virtual void RequestConfiguration(DG::EngineMtlCreateInfo* info) = 0;
		virtual void Initialize() = 0;
		virtual void Render(RenderCache* cache, Camera* camera) = 0;

		virtual RenderCache* BuildRenderCache(SceneHeirarchy* scene) = 0;

		// This buffer will be bound as a constant to all pipelines
		virtual DG::IBuffer* GetGlobalsBuffer() = 0;
		virtual DG::FILTER_TYPE GetDefaultFilter() = 0;
		virtual uint GetMaxAnisotropy() = 0;
	};
}