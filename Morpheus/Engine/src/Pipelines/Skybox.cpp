#include <Engine/Pipelines/PipelineFactory.hpp>
#include <Engine/PipelineResource.hpp>

namespace Morpheus {
	void CreateSkyboxPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		ShaderLoader* shaderLoader,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides) {

		auto skyboxVS = LoadShader(device, 
			DG::SHADER_TYPE_VERTEX,
			"internal/Skybox.vsh",
			"Skybox VS",
			"main",
			overrides,
			shaderLoader);

		auto skyboxPS = LoadShader(device,
			DG::SHADER_TYPE_PIXEL,
			"internal/Skybox.psh",
			"Skybox PS",
			"main",
			overrides,
			shaderLoader);

		DG::SamplerDesc SamLinearClampDesc
		{
			DG::FILTER_TYPE_LINEAR, DG::FILTER_TYPE_LINEAR, DG::FILTER_TYPE_LINEAR, 
			DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP
		};

		SamLinearClampDesc.MaxAnisotropy = renderer->GetMaxAnisotropy();

		DG::IPipelineState* result = nullptr;

		// Create Irradiance Pipeline
		DG::GraphicsPipelineStateCreateInfo PSOCreateInfo;
		DG::PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
		DG::GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

		PSODesc.Name         = "Skybox Pipeline";
		PSODesc.PipelineType = DG::PIPELINE_TYPE_GRAPHICS;

		GraphicsPipeline.NumRenderTargets             = 1;
		GraphicsPipeline.RTVFormats[0]                = renderer->GetIntermediateFramebufferFormat();
		GraphicsPipeline.PrimitiveTopology            = DG::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		GraphicsPipeline.RasterizerDesc.CullMode      = DG::CULL_MODE_NONE;
		GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
		GraphicsPipeline.DepthStencilDesc.DepthFunc   = DG::COMPARISON_FUNC_LESS_EQUAL;
		GraphicsPipeline.DSVFormat 					  = renderer->GetIntermediateDepthbufferFormat();

		// Number of MSAA samples
		GraphicsPipeline.SmplDesc.Count = renderer->GetMSAASamples();

		GraphicsPipeline.InputLayout.NumElements = 0;

		PSOCreateInfo.pVS = skyboxVS;
		PSOCreateInfo.pPS = skyboxPS;

		PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
		// clang-format off
		DG::ShaderResourceVariableDesc Vars[] = 
		{
			{DG::SHADER_TYPE_PIXEL, "mTexture", DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
		};
		// clang-format on
		PSODesc.ResourceLayout.NumVariables = _countof(Vars);
		PSODesc.ResourceLayout.Variables    = Vars;

		// clang-format off
		DG::ImmutableSamplerDesc ImtblSamplers[] =
		{
			{DG::SHADER_TYPE_PIXEL, "mTexture_sampler", SamLinearClampDesc}
		};
		// clang-format on
		PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
		PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
		
		device->CreateGraphicsPipelineState(PSOCreateInfo, &result);
		result->GetStaticVariableByName(DG::SHADER_TYPE_VERTEX, "Globals")->Set(renderer->GetGlobalsBuffer());

		skyboxVS->Release();
		skyboxPS->Release();

		into->SetAll(
			result,
			std::vector<DG::LayoutElement>(),
			VertexAttributeLayout(),
			InstancingType::NONE);
	}
}