#include <Engine/Pipelines/PipelineFactory.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/ShaderResource.hpp>

namespace Morpheus {
	TaskId CreateWhitePipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides,
		const AsyncResourceParams* asyncParams) {

		LoadParams<ShaderResource> vsParams(
			"internal/StaticMesh.vsh",
			DG::SHADER_TYPE_VERTEX,
			"Static Mesh VS",
			overrides,
			"main"
		);

		LoadParams<ShaderResource> psParams(
			"internal/White.psh",
			DG::SHADER_TYPE_PIXEL,
			"Basic Textured PS",
			overrides,
			"main"
		);

		ShaderResource* whiteVSResource = nullptr;
		ShaderResource* whitePSResource = nullptr;

		TaskBarrier* preLoadBarrier = into->GetPreLoadBarrier();
		TaskBarrier* postLoadBarrier = into->GetLoadBarrier();
		TaskId loadVSTask;
		TaskId loadPSTask;

		if (!asyncParams->bUseAsync) {
			whiteVSResource = manager->Load<ShaderResource>(vsParams);
			whitePSResource = manager->Load<ShaderResource>(psParams);
		} else {
			preLoadBarrier->Increment(2);
			loadVSTask = manager->AsyncLoadDeferred<ShaderResource>(vsParams, 
				&whiteVSResource, [preLoadBarrier](ThreadPool* pool) {
				preLoadBarrier->Decrement(pool);
			});
			loadPSTask = manager->AsyncLoadDeferred<ShaderResource>(psParams, 
				&whitePSResource, [preLoadBarrier](ThreadPool* pool) {
				preLoadBarrier->Decrement(pool);
			});
		}

		std::function<void()> buildPipeline = [=]() {
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
			GraphicsPipeline.SmplDesc.Count = renderer->GetMSAASamples();

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

			VertexAttributeLayout layout;
			layout.mPosition = 0;
			layout.mNormal = 1;
			layout.mUV = 2;
			layout.mTangent = 3;
			layout.mStride = 12 * sizeof(float);

			into->SetAll(
				result,
				layoutElements,
				layout,
				InstancingType::INSTANCED_STATIC_TRANSFORMS);
		};

		if (!asyncParams->bUseAsync) {
			buildPipeline(); // Build pipeline on the current thread
			return 0;
		} else {
			auto queue = asyncParams->mThreadPool->GetQueue();

			// When all prerequisites have been loaded, create the pipeline on main thread
			preLoadBarrier->SetCallback([postLoadBarrier, buildPipeline](ThreadPool* pool) {
				pool->GetQueue().Immediate([buildPipeline](const TaskParams& params) {
					buildPipeline();
				}, postLoadBarrier, 0);
			});
			
			// When the pipeline has been created, call the callback
			postLoadBarrier->SetCallback(asyncParams->mCallback);

			// Create a deferred task to trigger the loading of the vertex and pixel shaders
			return queue.Defer([loadVSTask, loadPSTask](const TaskParams& params) {
				auto queue = params.mPool->GetQueue();

				// Load vertex and pixel shaders
				queue.MakeImmediate(loadVSTask);
				queue.MakeImmediate(loadPSTask);
			});
		}
	}
}