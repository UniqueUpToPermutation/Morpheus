#pragma once

#include <functional>

#include <Engine/Resources/Resource.hpp>
#include <Engine/Resources/ShaderResource.hpp>
#include "RenderDevice.h"

namespace DG = Diligent;

namespace Morpheus {

	class IRenderer;
	class ResourceManager;
	class PipelineResource;
	
	std::vector<DG::IShaderResourceBinding*> GenerateSRBs(
		DG::IPipelineState* pipelineState, IRenderer* renderer);

	typedef std::function<Task(
		DG::IRenderDevice*, 
		ResourceManager*, 
		IRenderer*, 
		PipelineResource*,
		const ShaderPreprocessorConfig*)> factory_func_t;

	Task CreateBasicTexturedPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides = nullptr);

	Task CreateSkyboxPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides);

	Task CreateStaticMeshPBRPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides);

	Task CreateWhitePipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides);
}