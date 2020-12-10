#include <Engine/Pipelines/PipelineFactory.hpp>
#include <Engine/Pipelines/PBRStaticMesh.hpp>
#include <Engine/PipelineResource.hpp>

namespace Morpheus {
	DG::IShader* LoadShader(DG::IRenderDevice* device,
		DG::SHADER_TYPE shaderType,
		const std::string& path,
		const std::string& name,
		const std::string& entryPoint,
		const ShaderPreprocessorConfig* config,
		ShaderLoader* loader) {

		DG::ShaderCreateInfo info;
		info.Desc.ShaderType = shaderType;
		info.Desc.Name = name.c_str();
		info.EntryPoint = entryPoint.c_str();

		std::cout << "Loading " << path << "..." << std::endl;

		ShaderPreprocessorOutput output;
		loader->Load(path, &output, config);

		info.Source = output.mContent.c_str();
		info.SourceLanguage = DG::SHADER_SOURCE_LANGUAGE_HLSL;

		DG::IShader* shader = nullptr;
		device->CreateShader(info, &shader);

		return shader;
	}

	void ResourceCache<PipelineResource>::InitFactories() {
		mPipelineFactories["BasicTextured"] = &CreateBasicTexturedPipeline;
		mPipelineFactories["PBRStaticMesh"] = &CreatePBRStaticMeshPipeline<true, false, false>;
		mPipelineFactories["PBRStaticMesh_AO"] = &CreatePBRStaticMeshPipeline<true, true, false>;
		mPipelineFactories["PBRStaticMesh_AO_Emissive"] = &CreatePBRStaticMeshPipeline<true, true, true>;
		mPipelineFactories["PBRStaticMesh_Emissive"] = &CreatePBRStaticMeshPipeline<true, false, true>;
		mPipelineFactories["Skybox"] = &CreateSkyboxPipeline;
	}
}