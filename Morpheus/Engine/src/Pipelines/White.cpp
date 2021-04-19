#include <Engine/Pipelines/PipelineFactory.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/ShaderResource.hpp>
#include <Engine/Renderer.hpp>

namespace Morpheus {
	Task CreateWhitePipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides) {

		ShaderPreprocessorConfig overridesCopy;
		if (overrides)
			overridesCopy = *overrides;

		Task task;
		task.mType = TaskType::FILE_IO;
		task.mSyncPoint = into->GetLoadBarrier();
		task.mFunc = [device, manager, renderer, into, 
			overrides = std::move(overridesCopy)](const TaskParams& e) mutable {
			LoadParams<ShaderResource> vsParams(
				"internal/StaticMesh.vsh",
				DG::SHADER_TYPE_VERTEX,
				"Static Mesh VS",
				&overrides,
				"main"
			);

			LoadParams<ShaderResource> psParams(
				"internal/White.psh",
				DG::SHADER_TYPE_PIXEL,
				"Basic Textured PS",
				&overrides,
				"main"
			);

			ShaderResource* whiteVSResource = nullptr;
			ShaderResource* whitePSResource = nullptr;

			e.mQueue->Submit(manager->LoadTask<ShaderResource>(vsParams, &whiteVSResource));
			e.mQueue->Submit(manager->LoadTask<ShaderResource>(psParams, &whitePSResource));

			e.mQueue->YieldUntil(whiteVSResource->GetLoadBarrier());
			e.mQueue->YieldUntil(whitePSResource->GetLoadBarrier());

			e.mQueue->Submit([=](const TaskParams& e) {
				auto whiteVS = whiteVSResource->GetShader();
				auto whitePS = whitePSResource->GetShader();

				DG::IPipelineState* result = nullptr;

				// Create Irradiance Pipeline
				DG::GraphicsPipelineStateCreateInfo PSOCreateInfo;
				DG::PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
				DG::GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

				PSODesc.Name         = "Basic White Pipeline";
				PSODesc.PipelineType = DG::PIPELINE_TYPE_GRAPHICS;

				GraphicsPipeline.NumRenderTargets             = 1;
				GraphicsPipeline.RTVFormats[0]                = renderer->GetIntermediateFramebufferFormat();
				GraphicsPipeline.PrimitiveTopology            = DG::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				GraphicsPipeline.RasterizerDesc.CullMode      = DG::CULL_MODE_BACK;
				GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
				GraphicsPipeline.DSVFormat 					  = renderer->GetIntermediateDepthbufferFormat();

				// Number of MSAA samples
				GraphicsPipeline.SmplDesc.Count = (DG::Uint8)renderer->GetMSAASamples();

				uint stride = 12 * sizeof(float);

				std::vector<DG::LayoutElement> layoutElements = {
					DG::LayoutElement(0, 0, 3, DG::VT_FLOAT32, false, 
						DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
					DG::LayoutElement(1, 0, 3, DG::VT_FLOAT32, false, 
						DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
					DG::LayoutElement(2, 0, 2, DG::VT_FLOAT32, false, 
						DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
					DG::LayoutElement(3, 0, 3, DG::VT_FLOAT32, false, 
						DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),

					DG::LayoutElement(4, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
					DG::LayoutElement(5, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
					DG::LayoutElement(6, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
					DG::LayoutElement(7, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE)
				};

				GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
				GraphicsPipeline.InputLayout.LayoutElements = &layoutElements[0];

				PSOCreateInfo.pVS = whiteVS;
				PSOCreateInfo.pPS = whitePS;

				PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

				device->CreateGraphicsPipelineState(PSOCreateInfo, &result);
				result->GetStaticVariableByName(DG::SHADER_TYPE_VERTEX, "Globals")->Set(renderer->GetGlobalsBuffer());

				whiteVSResource->Release();
				whitePSResource->Release();

				VertexLayout layout;
				layout.mElements = std::move(layoutElements);
				layout.mPosition = 0;
				layout.mNormal = 1;
				layout.mUV = 2;
				layout.mTangent = 3;
				layout.mStride = 12 * sizeof(float);

				// Create shader resource bindings (one per render thread)
				renderer->GetMaxRenderThreadCount();

				into->SetAll(
					result,
					GenerateSRBs(result, renderer),
					layout,
					InstancingType::INSTANCED_STATIC_TRANSFORMS);
			}, TaskType::RENDER, into->GetLoadBarrier(), ASSIGN_THREAD_MAIN);
		};

		return task;
	}
}