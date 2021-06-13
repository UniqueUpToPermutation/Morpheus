#pragma once

#include <functional>

#include <Engine/Resources/Resource.hpp>
#include <Engine/Resources/ShaderResource.hpp>
#include "RenderDevice.h"

namespace DG = Diligent;

namespace Morpheus {

	class IRendererOld;
	class ResourceManager;
	class PipelineResource;
	
	std::vector<DG::IShaderResourceBinding*> GenerateSRBs(
		DG::IPipelineState* pipelineState, IRendererOld* renderer);

	typedef std::function<Task(
		DG::IRenderDevice*, 
		ResourceManager*, 
		IRendererOld*, 
		PipelineResource*,
		const ShaderPreprocessorConfig*)> factory_func_t;

	Task CreateBasicTexturedPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRendererOld* renderer,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides = nullptr);

	Task CreateSkyboxPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRendererOld* renderer,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides);

	Task CreateStaticMeshPBRPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRendererOld* renderer,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides);

	Task CreateWhitePipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRendererOld* renderer,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides);
}