#pragma once

#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Renderer.hpp>

#include <functional>

namespace Morpheus {
	
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