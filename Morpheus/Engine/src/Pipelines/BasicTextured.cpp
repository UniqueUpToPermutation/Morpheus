#include <Engine/Pipelines/PipelineFactory.hpp>
#include <Engine/PipelineResource.hpp>

namespace Morpheus {
	void CreateBasicTexturedPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		Renderer* renderer,
		ShaderLoader* shaderLoader,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides) {

		auto basicTexturedVS = LoadShader(device, 
			DG::SHADER_TYPE_VERTEX,
			"internal/BasicTextured.vsh",
			"Basic Textured VS",
			"main",
			overrides,
			shaderLoader);

		auto basicTexturedPS = LoadShader(device,
			DG::SHADER_TYPE_PIXEL,
			"internal/BasicTextured.psh",
			"Basic Textured PS",
			"main",
			overrides,
			shaderLoader);

		DG::SamplerDesc SamLinearClampDesc
		{
			DG::FILTER_TYPE_LINEAR, DG::FILTER_TYPE_LINEAR, DG::FILTER_TYPE_LINEAR, 
			DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP
		};

		DG::IPipelineState* result = nullptr;

		// Create Irradiance Pipeline
		DG::GraphicsPipelineStateCreateInfo PSOCreateInfo;
		DG::PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
		DG::GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

		PSODesc.Name         = "Basic Textured Pipeline";
		PSODesc.PipelineType = DG::PIPELINE_TYPE_GRAPHICS;

		GraphicsPipeline.NumRenderTargets             = 1;
		GraphicsPipeline.RTVFormats[0]                = renderer->GetIntermediateFramebufferFormat();
		GraphicsPipeline.PrimitiveTopology            = DG::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		GraphicsPipeline.RasterizerDesc.CullMode      = DG::CULL_MODE_BACK;
		GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
		GraphicsPipeline.DSVFormat 					  = renderer->GetIntermediateDepthbufferFormat();

		std::vector<DG::LayoutElement> layoutElements = {
			DG::LayoutElement(0, 0, 3, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(1, 0, 2, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),

			DG::LayoutElement(2, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
			DG::LayoutElement(3, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
			DG::LayoutElement(4, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
			DG::LayoutElement(5, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE)
		};

		GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
		GraphicsPipeline.InputLayout.LayoutElements = &layoutElements[0];

		PSOCreateInfo.pVS = basicTexturedVS;
		PSOCreateInfo.pPS = basicTexturedPS;

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

		basicTexturedVS->Release();
		basicTexturedPS->Release();

		VertexAttributeIndices indx;
		indx.mPosition = 0;
		indx.mUV = 1;

		into->SetAll(
			result,
			layoutElements,
			indx,
			InstancingType::INSTANCED_STATIC_TRANSFORMS);
	}
}