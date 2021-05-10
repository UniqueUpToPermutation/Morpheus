#include <Engine/Pipelines/PipelineFactory.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/ShaderResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Renderer.hpp>

namespace Morpheus {
	Task CreateSkyboxPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides) {

		ShaderPreprocessorConfig overridesCopy;
		if (overrides)
			overridesCopy = *overrides;

		struct Data {
			ShaderResource* mSkyboxVSResource = nullptr;
			ShaderResource* mSkyboxPSResource = nullptr;

			~Data() {
				if (mSkyboxVSResource)
					mSkyboxVSResource->Release();
				if (mSkyboxPSResource)
					mSkyboxPSResource->Release();
			}
		};

		Task task([device, manager, renderer, into,
			overrides = std::move(overridesCopy), data = Data()](const TaskParams& e) mutable {
			if (e.mTask->SubTask()) {
				LoadParams<ShaderResource> vsParams(
					"internal/Skybox.vsh",
					DG::SHADER_TYPE_VERTEX,
					"Skybox VS",
					&overrides,
					"main"
				);

				LoadParams<ShaderResource> psParams(
					"internal/Skybox.psh",
					DG::SHADER_TYPE_PIXEL,
					"Skybox PS",
					&overrides,
					"main"
				);

				e.mQueue->AdoptAndTrigger(manager->LoadTask<ShaderResource>(vsParams, &data.mSkyboxVSResource));
				e.mQueue->AdoptAndTrigger(manager->LoadTask<ShaderResource>(psParams, &data.mSkyboxPSResource));

				if (e.mTask->In().Lock()
					.Connect(&data.mSkyboxVSResource->GetLoadBarrier()->mOut)
					.Connect(&data.mSkyboxPSResource->GetLoadBarrier()->mOut)
					.ShouldWait())
					return TaskResult::WAITING;
			}

			if (e.mTask->SubTask()) {
				auto skyboxVS = data.mSkyboxVSResource->GetShader();
				auto skyboxPS = data.mSkyboxPSResource->GetShader();

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
				GraphicsPipeline.SmplDesc.Count = (DG::Uint8)renderer->GetMSAASamples();

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

				into->SetAll(
					result,
					GenerateSRBs(result, renderer),
					VertexLayout(),
					InstancingType::NONE);
			}

			return TaskResult::FINISHED;
		}, 
		"Load Skybox Pipeline", 
		TaskType::UNSPECIFIED, 
		ASSIGN_THREAD_MAIN);

		return task;
	}
}