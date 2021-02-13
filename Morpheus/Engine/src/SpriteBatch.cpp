#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Pipelines/PipelineFactory.hpp>
#include <Engine/SpriteBatch.hpp>
#include <Engine/Engine.hpp>

using namespace DG;

#include <shaders/SpriteBatchStructures.hlsl>

namespace Morpheus {
	SpriteBatch::SpriteBatch(DG::IRenderDevice* device, PipelineResource* pipeline, uint batchSize) {

		mBatchSize = batchSize;
		mBatchSizeBytes = batchSize * sizeof(SpriteBatchVSInput);

		DG::BufferDesc desc;
		desc.Name = "Sprite Batch Buffer";
		desc.BindFlags = DG::BIND_VERTEX_BUFFER;
		desc.uiSizeInBytes = mBatchSizeBytes;
		desc.Usage = DG::USAGE_DYNAMIC;
		desc.CPUAccessFlags = DG::CPU_ACCESS_WRITE;

		mBuffer = nullptr;
		device->CreateBuffer(desc, nullptr, &mBuffer);

		mDefaultState = CreateState(pipeline);
	}

	SpriteBatch::SpriteBatch(DG::IRenderDevice* device, 
		ResourceManager* resourceManager, 
		DG::FILTER_TYPE filterType,
		uint batchSize) {

		mBatchSize = batchSize;
		mBatchSizeBytes = batchSize * sizeof(SpriteBatchVSInput);

		DG::BufferDesc desc;
		desc.Name = "Sprite Batch Buffer";
		desc.BindFlags = DG::BIND_VERTEX_BUFFER;
		desc.uiSizeInBytes = mBatchSizeBytes;
		desc.Usage = DG::USAGE_DYNAMIC;
		desc.CPUAccessFlags = DG::CPU_ACCESS_WRITE;

		mBuffer = nullptr;
		device->CreateBuffer(desc, nullptr, &mBuffer);

		PipelineResource* pipeline = LoadPipeline(resourceManager, filterType);
		mDefaultState = CreateState(pipeline);
		pipeline->Release();
	}

	SpriteBatch::SpriteBatch(DG::IRenderDevice* device, uint batchSize) {

		mBatchSize = batchSize;
		mBatchSizeBytes = batchSize * sizeof(SpriteBatchVSInput);

		DG::BufferDesc desc;
		desc.Name = "Sprite Batch Buffer";
		desc.BindFlags = DG::BIND_VERTEX_BUFFER;
		desc.uiSizeInBytes = mBatchSizeBytes;
		desc.Usage = DG::USAGE_DYNAMIC;
		desc.CPUAccessFlags = DG::CPU_ACCESS_WRITE;

		mBuffer = nullptr;
		device->CreateBuffer(desc, nullptr, &mBuffer);

		mBatchSize = batchSize;
	}

	SpriteBatch::~SpriteBatch() {
		mBuffer->Release();
	}

	PipelineResource* SpriteBatch::LoadPipeline(ResourceManager* manager, DG::FILTER_TYPE filterType, ShaderResource* pixelShader) {
		if (pixelShader) {
			auto cache = manager->GetCache<PipelineResource>();

			pixelShader->AddRef();
			factory_func_t factory = [pixelShader, filterType](DG::IRenderDevice* device, 
				ResourceManager* manager, 
				IRenderer* renderer, 
				PipelineResource* into,
				const ShaderPreprocessorConfig* overrides,
				const AsyncResourceParams* async) {
				auto taskId = CreateSpriteBatchPipeline(device, manager, renderer, into, 
					overrides, filterType, async, pixelShader);
				pixelShader->Release();
				return taskId;
			};

			return cache->LoadFromFactory(factory);
		} else {
			auto cache = manager->GetCache<PipelineResource>();

			factory_func_t factory = [filterType](DG::IRenderDevice* device, 
				ResourceManager* manager, 
				IRenderer* renderer, 
				PipelineResource* into,
				const ShaderPreprocessorConfig* overrides,
				const AsyncResourceParams* async) {

				return CreateSpriteBatchPipeline(device, manager, renderer, into, 
					overrides, filterType, async, nullptr);
			};

			return cache->LoadFromFactory(factory);
		}
	}

	void SpriteBatch::Begin(DG::IDeviceContext* context, const SpriteBatchState* state) {
		if (state) {
			mCurrentState = *state;
		} else {
			mCurrentState = mDefaultState;
		}

		context->SetPipelineState(mCurrentState.mPipeline->GetState());

		DG::IBuffer* buffers[] = { mBuffer };
		DG::Uint32 offsets[] = { 0 };
		
		context->SetVertexBuffers(0, 1, buffers, offsets, 
			DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, 
			DG::SET_VERTEX_BUFFERS_FLAG_RESET);

		mMapHelper.Map(context, mBuffer, DG::MAP_WRITE, DG::MAP_FLAG_DISCARD);

		mWriteIndex = 0;
		mCurrentContext = context;
		mLastTexture = nullptr;
	}

	void SpriteBatch::Flush() {
		if (mWriteIndex > 0) {
			DG::DrawAttribs attribs;
			attribs.Flags = DG::DRAW_FLAG_VERIFY_ALL;
			attribs.StartVertexLocation = 0;
			attribs.NumVertices = mWriteIndex;
			
			mMapHelper.Unmap();
			mCurrentState.mTextureVariable->Set(mLastTexture->GetShaderView());
			mCurrentContext->CommitShaderResources(mCurrentState.mShaderBinding, 
						RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			mCurrentContext->Draw(attribs);

			mMapHelper.Map(mCurrentContext, mBuffer, DG::MAP_WRITE, DG::MAP_FLAG_DISCARD);
			
			mWriteIndex = 0;
		}
	}

	void SpriteBatch::End() {
		Flush();
		mMapHelper.Unmap();
		mCurrentState = SpriteBatchState();
	}

	void SpriteBatch::Draw(TextureResource* texture, const DG::float3& pos,
		const DG::float2& size, const SpriteRect& rect, 
		const DG::float2& origin, const float rotation, 
		const DG::float4& color) {

		if (!mLastTexture)
			mLastTexture = texture;
		if (mWriteIndex == mBatchSize || (mLastTexture && mLastTexture != texture)) {
			Flush();
			mLastTexture = texture;
		}

		SpriteBatchVSInput* instance = &mMapHelper[mWriteIndex];
		auto dim2d = texture->GetDimensions2D();
		
		auto uvtop_unscaled = DG::float2(rect.mPosition.x - 0.5, rect.mPosition.y - 0.5);
		auto uvbottom_unscaled = uvtop_unscaled + rect.mSize;

		instance->mPos.x = pos.x;
		instance->mPos.y = pos.y;
		instance->mPos.z = pos.z;
		instance->mPos.w = rotation;

		instance->mColor = color;
		instance->mOrigin = origin;
		instance->mSize = size;
		instance->mUVTop = uvtop_unscaled / dim2d;
		instance->mUVBottom = uvbottom_unscaled / dim2d;

		++mWriteIndex;
	}

	void SpriteBatch::Draw(TextureResource* texture, const DG::float2& pos, 
		const DG::float2& size, const SpriteRect& rect, 
		const DG::float2& origin, const float rotation, 
		const DG::float4& color) {

		if (!mLastTexture)
			mLastTexture = texture;
		if (mWriteIndex == mBatchSize || mLastTexture != texture) {
			Flush();
			mLastTexture = texture;
		}

		SpriteBatchVSInput* instance = &mMapHelper[mWriteIndex];
		auto dim2d = texture->GetDimensions2D();
		
		auto uvtop_unscaled = DG::float2(rect.mPosition.x - 0.5, rect.mPosition.y - 0.5);
		auto uvbottom_unscaled = uvtop_unscaled + rect.mSize;

		instance->mPos.x = pos.x;
		instance->mPos.y = pos.y;
		instance->mPos.z = 0.0;
		instance->mPos.w = rotation;

		instance->mColor = color;
		instance->mOrigin = origin;
		instance->mSize = size;
		instance->mUVTop = uvtop_unscaled / dim2d;
		instance->mUVBottom = uvbottom_unscaled / dim2d;

		++mWriteIndex;
	}

	SpriteBatchState SpriteBatch::CreateState(PipelineResource* resource) {
		auto state = resource->GetState();

		DG::IShaderResourceBinding* srb = nullptr;
		state->CreateShaderResourceBinding(&srb, true);

		auto texVar = srb->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mTexture");
		return SpriteBatchState(srb, texVar, resource);
	}

	TaskId CreateSpriteBatchPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides,
		DG::FILTER_TYPE filterType,
		const AsyncResourceParams* asyncParams,
		ShaderResource* pixelShader) {

		LoadParams<ShaderResource> vsParams(
			"internal/SpriteBatch.vsh",
			DG::SHADER_TYPE_VERTEX,
			"Sprite Batch VS",
			overrides,
			"main"
		);

		LoadParams<ShaderResource> gsParams(
			"internal/SpriteBatch.gsh",
			DG::SHADER_TYPE_GEOMETRY,
			"Sprite Batch GS",
			overrides,
			"main"
		);

		LoadParams<ShaderResource> psParams(
			"internal/SpriteBatch.psh",
			DG::SHADER_TYPE_PIXEL,
			"Sprite Batch PS",
			overrides,
			"main"
		);

		ShaderResource *sbVertex = nullptr;
		ShaderResource *sbGeo = nullptr;
		ShaderResource *sbPixel = nullptr;

		TaskBarrier* postLoadBarrier = into->GetLoadBarrier();
		TaskId loadVSTask = TASK_NONE;
		TaskId loadGSTask = TASK_NONE;
		TaskId loadPSTask = TASK_NONE;

		if (!asyncParams->bUseAsync) {
			sbVertex = manager->Load<ShaderResource>(vsParams);
			sbGeo = manager->Load<ShaderResource>(gsParams);
			if (!pixelShader)
				sbPixel = manager->Load<ShaderResource>(psParams);
			else
				sbPixel = pixelShader;
		} else {
			loadVSTask = manager->AsyncLoadDeferred<ShaderResource>(vsParams, 
				&sbVertex);
			loadGSTask = manager->AsyncLoadDeferred<ShaderResource>(gsParams,
				&sbGeo);
			if (!pixelShader)
				loadPSTask = manager->AsyncLoadDeferred<ShaderResource>(psParams, 
					&sbPixel);
			else
				sbPixel = pixelShader;
		}

		std::function<void()> buildPipeline = [=]() {
			auto batchVS = sbVertex->GetShader();
			auto batchGS = sbGeo->GetShader();
			auto batchPS = sbPixel->GetShader();

			DG::SamplerDesc SamDesc
			{
				filterType, filterType, filterType, 
				DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP
			};

			DG::IPipelineState* result = nullptr;

			// Create Irradiance Pipeline
			DG::GraphicsPipelineStateCreateInfo PSOCreateInfo;
			DG::PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
			DG::GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

			PSODesc.Name         = "Sprite Batch Pipeline";
			PSODesc.PipelineType = DG::PIPELINE_TYPE_GRAPHICS;

			GraphicsPipeline.NumRenderTargets             = 1;
			GraphicsPipeline.RTVFormats[0]                = renderer->GetBackbufferColorFormat();
			GraphicsPipeline.PrimitiveTopology            = DG::PRIMITIVE_TOPOLOGY_POINT_LIST;
			GraphicsPipeline.RasterizerDesc.CullMode      = DG::CULL_MODE_BACK;
			GraphicsPipeline.DepthStencilDesc.DepthEnable = false;
			GraphicsPipeline.DSVFormat 					  = renderer->GetBackbufferDepthFormat();

			DG::RenderTargetBlendDesc blendState;
			blendState.BlendEnable = true;
			blendState.BlendOp = DG::BLEND_OPERATION_ADD;
			blendState.BlendOpAlpha = DG::BLEND_OPERATION_ADD;
			blendState.DestBlend = DG::BLEND_FACTOR_INV_SRC_ALPHA;
			blendState.SrcBlend = DG::BLEND_FACTOR_SRC_ALPHA;
			blendState.DestBlendAlpha = DG::BLEND_FACTOR_ONE;
			blendState.SrcBlendAlpha = DG::BLEND_FACTOR_ONE;

			GraphicsPipeline.BlendDesc.RenderTargets[0] = blendState;

			// Number of MSAA samples
			GraphicsPipeline.SmplDesc.Count = 1;

			uint stride = sizeof(SpriteBatchVSInput);

			std::vector<DG::LayoutElement> layoutElements = {
				DG::LayoutElement(0, 0, 4, DG::VT_FLOAT32, false, 
					DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
				DG::LayoutElement(1, 0, 4, DG::VT_FLOAT32, false, 
					DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
				DG::LayoutElement(2, 0, 2, DG::VT_FLOAT32, false, 
					DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
				DG::LayoutElement(3, 0, 2, DG::VT_FLOAT32, false, 
					DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
				DG::LayoutElement(4, 0, 2, DG::VT_FLOAT32, false, 
					DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
				DG::LayoutElement(5, 0, 2, DG::VT_FLOAT32, false, 
					DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			};

			GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
			GraphicsPipeline.InputLayout.LayoutElements = &layoutElements[0];

			PSOCreateInfo.pVS = batchVS;
			PSOCreateInfo.pGS = batchGS;
			PSOCreateInfo.pPS = batchPS;

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
				{DG::SHADER_TYPE_PIXEL, "mTexture_sampler", SamDesc}
			};
			// clang-format on
			PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
			PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;

			device->CreateGraphicsPipelineState(PSOCreateInfo, &result);

			auto globalsVar = result->GetStaticVariableByName(DG::SHADER_TYPE_VERTEX, "Globals");
			if (globalsVar)
				globalsVar->Set(renderer->GetGlobalsBuffer());

			sbVertex->Release();
			sbGeo->Release();
			sbPixel->Release();

			VertexAttributeLayout layout;
			layout.mPosition = 0;
			layout.mStride = sizeof(SpriteBatchVSInput);

			into->SetAll(
				result,
				layoutElements,
				layout,
				InstancingType::NONE);
		};

		if (!asyncParams->bUseAsync) {
			buildPipeline(); // Build pipeline on the current thread
			return TASK_NONE;
		} else {
			auto queue = asyncParams->mThreadPool->GetQueue();

			TaskId buildPipelineTask = queue.MakeTask([buildPipeline](const TaskParams& params) { 
				buildPipeline();
			}, postLoadBarrier, 0);

			// Schedule the loading of the build pipeline task
			queue.Dependencies(buildPipelineTask)
				.After(sbVertex->GetLoadBarrier())
				.After(sbGeo->GetLoadBarrier())
				.After(sbPixel->GetLoadBarrier());
		
			postLoadBarrier->SetCallback(asyncParams->mCallback);

			// Create a deferred task to trigger the loading of the vertex and pixel shaders
			return queue.MakeTask([loadVSTask, loadPSTask, loadGSTask](const TaskParams& params) {
				auto queue = params.mPool->GetQueue();

				// Load vertex and pixel shaders
				queue.Schedule(loadVSTask);
				queue.Schedule(loadGSTask);
				queue.Schedule(loadPSTask);
			});
		}
	}
}