#pragma once

#include <Engine/Engine.hpp>

namespace DG = Diligent;

namespace Morpheus {
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
		virtual void Render() = 0;
	};
}