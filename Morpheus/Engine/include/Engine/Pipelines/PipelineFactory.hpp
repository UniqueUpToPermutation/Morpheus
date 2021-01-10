#pragma once

#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Renderer.hpp>

#include <functional>

namespace Morpheus {
	typedef std::function<void(
		DG::IRenderDevice*, 
		ResourceManager*, 
		IRenderer*, 
		ShaderLoader*,
		PipelineResource*,
		const ShaderPreprocessorConfig*)> factory_func_t;

	DG::IShader* LoadShader(DG::IRenderDevice* device,
		DG::SHADER_TYPE shaderType,
		const std::string& path,
		const std::string& name,
		const std::string& entryPoint,
		const ShaderPreprocessorConfig* config,
		ShaderLoader* loader);

	void CreateBasicTexturedPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		ShaderLoader* shaderLoader,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides = nullptr);

	void CreateSkyboxPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		ShaderLoader* shaderLoader,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides);

	void CreateStaticMeshPBRPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		ShaderLoader* shaderLoader,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides);

	void CreateWhitePipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		ShaderLoader* shaderLoader,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides);
}