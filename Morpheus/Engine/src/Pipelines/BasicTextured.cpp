#include <Engine/Pipelines/PipelineFactory.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/ShaderResource.hpp>
#include <Engine/Materials/BasicTexturedMaterial.hpp>
#include <Engine/Renderer.hpp>

namespace Morpheus {
	Task CreateBasicTexturedPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRendererOld* renderer,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides) {

		ShaderPreprocessorConfig overridesCopy;
		if (overrides)
			overridesCopy = *overrides;

		struct Data {
			ShaderResource* mBasicTexturedVSResource = nullptr;
			ShaderResource* mBasicTexturedPSResource = nullptr;

			~Data() {
				if (mBasicTexturedVSResource)
					mBasicTexturedVSResource->Release();
				if (mBasicTexturedPSResource)
					mBasicTexturedPSResource->Release();
			}
		};

		Task task([device, manager, renderer, into, 
			overrides = std::move(overridesCopy), data = Data()](const TaskParams& e) mutable {

			if (e.mTask->SubTask()) {
				LoadParams<ShaderResource> vsShaderParams("internal/BasicTextured.vsh", 
					DG::SHADER_TYPE_VERTEX,
					"Basic Textured VS",
					&overrides,
					"main");

				LoadParams<ShaderResource> psShaderParams("internal/BasicTextured.psh",
					DG::SHADER_TYPE_PIXEL,
					"Basic Textured PS",
					&overrides,
					"main");

				Task vsLoad = manager->LoadTask<ShaderResource>(vsShaderParams, &data.mBasicTexturedVSResource);
				Task psLoad = manager->LoadTask<ShaderResource>(psShaderParams, &data.mBasicTexturedPSResource);

				e.mQueue->AdoptAndTrigger(std::move(vsLoad));
				e.mQueue->AdoptAndTrigger(std::move(psLoad));

				if (e.mTask->In().Lock()
					.Connect(&data.mBasicTexturedVSResource->GetLoadBarrier()->mOut)
					.Connect(&data.mBasicTexturedPSResource->GetLoadBarrier()->mOut)
					.ShouldWait())
					return TaskResult::WAITING;
			}

			// Build pipeline on main thread
			if (e.mTask->SubTask()) {
				auto basicTexturedVS = data.mBasicTexturedVSResource->GetShader();
				auto basicTexturedPS = data.mBasicTexturedPSResource->GetShader();

				auto anisotropyFactor = renderer->GetMaxAnisotropy();
				auto filterType = anisotropyFactor > 1 ? DG::FILTER_TYPE_ANISOTROPIC : DG::FILTER_TYPE_LINEAR;

				DG::SamplerDesc SamLinearClampDesc
				{
					filterType, filterType, filterType, 
					DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP
				};

				SamLinearClampDesc.MaxAnisotropy = anisotropyFactor;

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

				// Number of MSAA samples
				GraphicsPipeline.SmplDesc.Count = (DG::Uint8)renderer->GetMSAASamples();

				std::vector<DG::LayoutElement> layoutElements = {
					DG::LayoutElement(0, 0, 3, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
					DG::LayoutElement(1, 0, 3, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),

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

				VertexLayout layout;
				layout.mElements = std::move(layoutElements);
				layout.mPosition = 0;
				layout.mUV = 1;

				auto srbs = GenerateSRBs(result, renderer);

				into->SetAll(
					result,
					srbs,
					layout,
					InstancingType::INSTANCED_STATIC_TRANSFORMS);

				into->AddView<BasicTexturedPipelineView>(into);
			}

			return TaskResult::FINISHED;
		}, 
		"Upload Basic Textured Pipeline", 
		TaskType::UNSPECIFIED, 
		ASSIGN_THREAD_MAIN);

		return task;
	}
}