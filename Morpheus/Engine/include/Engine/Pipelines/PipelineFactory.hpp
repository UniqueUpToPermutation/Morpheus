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
	
	typedef std::function<TaskId(
		DG::IRenderDevice*, 
		ResourceManager*, 
		IRenderer*, 
		PipelineResource*,
		const ShaderPreprocessorConfig*,
		const AsyncResourceParams*)> factory_func_t;

	TaskId CreateBasicTexturedPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides = nullptr,
		const AsyncResourceParams* asyncParams = nullptr);

	TaskId CreateSkyboxPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides,
		const AsyncResourceParams* asyncParams = nullptr);

	TaskId CreateStaticMeshPBRPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides,
		const AsyncResourceParams* asyncParams = nullptr);

	TaskId CreateWhitePipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides,
		const AsyncResourceParams* asyncParams = nullptr);
}