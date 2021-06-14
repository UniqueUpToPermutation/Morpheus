#include <Engine/Systems/DefaultRenderer.hpp>
#include <Engine/Resources/Shader.hpp>
#include <Engine/Components/SkyboxComponent.hpp>

namespace Morpheus {

	VertexLayout DefaultStaticMeshLayout() {
		uint stride = 12 * sizeof(float);

		VertexLayout layout;

		layout.mElements = {
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

		layout.mPosition = 0;
		layout.mUV = 1;
		layout.mNormal = 2;
		layout.mTangent = 4;
		layout.mStride = stride;

		return layout;
	}

	// Common sampler states
	static const DG::SamplerDesc Sam_LinearClamp
	{
		DG::FILTER_TYPE_LINEAR,
		DG::FILTER_TYPE_LINEAR,
		DG::FILTER_TYPE_LINEAR, 
		DG::TEXTURE_ADDRESS_CLAMP,
		DG::TEXTURE_ADDRESS_CLAMP,
		DG::TEXTURE_ADDRESS_CLAMP
	};

	Task DefaultRenderer::Startup(SystemCollection& collection) {
		struct Data {
			Future<Handle<DG::IShader>> mStaticMeshResult;
			Future<Handle<DG::IShader>> mLambertVSResult;
			Future<Handle<DG::IShader>> mLambertPSResult;
			Future<Handle<DG::IShader>> mCookTorrenceResult;
			Future<Handle<DG::IShader>> mSkyboxVSResult;
			Future<Handle<DG::IShader>> mSkyboxPSResult;
		};

		Task startupTask([this, data = Data()](const TaskParams& e) mutable {
			if (e.mTask->BeginSubTask()) {
				ShaderPreprocessorConfig config;
				config.mDefines["USE_IBL"] = "1";
				config.mDefines["USE_SH"] = "1";

				LoadParams<RawShader> staticMeshParams("internal/StaticMesh.vsh",
					DG::SHADER_TYPE_VERTEX,
					"Static Mesh VS", 
					config);

				LoadParams<RawShader> lambertVSParams("internal/Lambert.vsh",
					DG::SHADER_TYPE_VERTEX,
					"Lambert VS",
					config);

				LoadParams<RawShader> lambertPSParams("internal/Lambert.psh",
					DG::SHADER_TYPE_PIXEL,
					"Lambert PS",
					config);

				LoadParams<RawShader> cookTorrenceParams("internal/PBR.psh",
					DG::SHADER_TYPE_PIXEL,
					"Cook Torrence IBL PS",
					config);

				LoadParams<RawShader> skyboxVSParams("internal/Skybox.vsh",
					DG::SHADER_TYPE_VERTEX,
					"Skybox VS",
					config);

				LoadParams<RawShader> skyboxPSParams("internal/Skybox.psh",
					DG::SHADER_TYPE_PIXEL,
					"Skybox PS",
					config);

				auto device = mGraphics->Device();

				auto staticMeshLoad			= LoadShaderHandle(device, staticMeshParams);
				auto lambertVSLoad			= LoadShaderHandle(device, lambertVSParams);
				auto lambertPSLoad			= LoadShaderHandle(device, lambertPSParams);
				auto cookTorrenceLoad		= LoadShaderHandle(device, cookTorrenceParams);
				auto skyboxVSLoad			= LoadShaderHandle(device, skyboxVSParams);
				auto skyboxPSLoad			= LoadShaderHandle(device, skyboxPSParams);

				data.mStaticMeshResult 		= e.mQueue->AdoptAndTrigger(std::move(staticMeshLoad));
				data.mLambertVSResult 		= e.mQueue->AdoptAndTrigger(std::move(lambertVSLoad));
				data.mLambertPSResult 		= e.mQueue->AdoptAndTrigger(std::move(lambertPSLoad));
				data.mCookTorrenceResult 	= e.mQueue->AdoptAndTrigger(std::move(cookTorrenceLoad));
				data.mSkyboxVSResult 		= e.mQueue->AdoptAndTrigger(std::move(skyboxVSLoad));
				data.mSkyboxPSResult 		= e.mQueue->AdoptAndTrigger(std::move(skyboxPSLoad));

				e.mTask->EndSubTask();	

				// Wait for resources to load.
				if (e.mTask->In().Lock()
					.Connect(data.mStaticMeshResult.Out())
					.Connect(data.mLambertVSResult.Out())
					.Connect(data.mLambertPSResult.Out())
					.Connect(data.mCookTorrenceResult.Out())
					.Connect(data.mSkyboxVSResult.Out())
					.Connect(data.mSkyboxPSResult.Out())
					.ShouldWait())
					return TaskResult::WAITING;
			}

			if (e.mTask->RequestThreadSwitch(e, ASSIGN_THREAD_MAIN))
				return TaskResult::REQUEST_THREAD_SWITCH;

			// Acquire loaded resources
			mResources.mCookTorrenceIBL.mCookTorrencePS 
											= data.mCookTorrenceResult.Get();
			mResources.mStaticMeshVS 		= data.mStaticMeshResult.Get();
			mResources.mLambert.mLambertPS 	= data.mLambertPSResult.Get();
			mResources.mLambert.mLambertVS 	= data.mLambertVSResult.Get();
			mResources.mSkybox.mVS 			= data.mSkyboxVSResult.Get();
			mResources.mSkybox.mPS 			= data.mSkyboxPSResult.Get();

			InitializeDefaultResources();
			CreateLambertPipeline(mResources.mLambert.mLambertVS, mResources.mLambert.mLambertPS);
			CreateCookTorrenceIBLPipeline(mResources.mStaticMeshVS, mResources.mCookTorrenceIBL.mCookTorrencePS);
			CreateSkyboxPipeline(mResources.mSkybox.mVS, mResources.mSkybox.mPS);
		
			auto device = mGraphics->Device();
		
			// Skybox stuff
			DG::IShaderResourceBinding* skyboxBinding = nullptr;
			mResources.mSkybox.mPipeline->CreateShaderResourceBinding(&skyboxBinding, true);
			mResources.mSkybox.mSkyboxBinding.Adopt(skyboxBinding);
			
			skyboxBinding->GetVariableByName(DG::SHADER_TYPE_VERTEX, 
				"CameraData")->Set(mResources.mCameraData.Get());
			mResources.mSkybox.mTexture = skyboxBinding->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mTexture");

			bIsInitialized = true;
			return TaskResult::FINISHED;
		}, "Initialize DefaultRenderer");


		// Update all transform caches
		ParameterizedTask<RenderParams> updateTransformCache([this](const TaskParams& e, const RenderParams& params) {
			CacheUpdater().UpdateChanges();
		});
		
		// Just clear the screen
		ParameterizedTask<RenderParams> renderTask([this](const TaskParams& e, const RenderParams& params) {
			auto graphics = mGraphics;
			auto context = graphics->ImmediateContext();
			auto swapChain = graphics->SwapChain();
			auto rtv = swapChain->GetCurrentBackBufferRTV();
			auto dsv = swapChain->GetDepthBufferDSV();
			float color[] = { 0.5f, 0.5f, 1.0f, 1.0f };
			context->SetRenderTargets(1, &rtv, dsv, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context->ClearRenderTarget(rtv, color, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context->ClearDepthStencil(dsv, DG::CLEAR_DEPTH_FLAG, 1.0f, 0, 
				DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		}, "Clear Screen", TaskType::RENDER, ASSIGN_THREAD_MAIN);

		renderTask->In().Lock().Connect(&updateTransformCache->Out());

		// Give this task to the frame processor
		collection.GetFrameProcessor().AddRenderTask(std::move(updateTransformCache));
		collection.GetFrameProcessor().AddRenderTask(std::move(renderTask));
		
		return startupTask;
	}

	ParameterizedTask<RenderParams> DefaultRenderer::BeginRender() {
		return ParameterizedTask<RenderParams>([this](const TaskParams& e, const RenderParams& params) {
			auto graphics = mGraphics;
			auto context = graphics->ImmediateContext();
			auto swapChain = graphics->SwapChain();
			auto rtv = swapChain->GetCurrentBackBufferRTV();
			auto dsv = swapChain->GetDepthBufferDSV();
			float color[] = { 0.5f, 0.5f, 1.0f, 1.0f };
			context->SetRenderTargets(1, &rtv, dsv, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			
			if (params.mFrame->mRegistry.view<SkyboxComponent>().size() == 0)
				context->ClearRenderTarget(rtv, color, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			
			context->ClearDepthStencil(dsv, DG::CLEAR_DEPTH_FLAG, 1.0f, 0, 
				DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		}, "Begin Render", TaskType::RENDER, ASSIGN_THREAD_MAIN);
	}
	
	ParameterizedTask<RenderParams> DefaultRenderer::DrawBackground() {
		return ParameterizedTask<RenderParams>([this](const TaskParams& e, const RenderParams& params) {
			auto skyboxes = params.mFrame->mRegistry.view<SkyboxComponent>();

			auto ent = skyboxes.front();
			if (ent != entt::null) {
				// Render skybox
				auto context = GetGraphics()->ImmediateContext();
				context->SetPipelineState(Resources().mSkybox.mPipeline.Ptr());
				context->CommitShaderResources(Resources().mSkybox.mSkyboxBinding.Ptr(),
					DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

				DG::DrawAttribs attribs;
				attribs.NumVertices = 4;
				context->Draw(attribs);
			}
		}, "Draw Skybox", TaskType::RENDER, ASSIGN_THREAD_MAIN);
	}

	ParameterizedTaskGroup<RenderParams>* DefaultRenderer::CreateRenderGroup() {
		auto group = new ParameterizedTaskGroup<RenderParams>();

		auto beginRender = BeginRender();
		auto drawBackground = DrawBackground();
		
		// Draw background after begin render stuff
		drawBackground->In().Lock().Connect(beginRender->Out());

		group->Adopt(std::move(beginRender));
		group->Adopt(std::move(drawBackground));

		return group;
	}

	void DefaultRenderer::InitializeDefaultResources() {
		auto device = mGraphics->Device();
		auto context = mGraphics->ImmediateContext();

		DG::BufferDesc desc;
		desc.Name = "Renderer Instance Buffer";
		desc.Usage = DG::USAGE_DYNAMIC;
		desc.BindFlags = DG::BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = DG::CPU_ACCESS_WRITE;
		desc.uiSizeInBytes = sizeof(DG::float4x4) * mInstanceBatchSize;

		DG::IBuffer* buffer = nullptr;
		device->CreateBuffer(desc, nullptr, &buffer);
		mResources.mInstanceBuffer = buffer;

		// Create default textures
		static constexpr DG::Uint32 TexDim = 8;

        DG::TextureDesc TexDesc;
        TexDesc.Name      = "White texture for renderer";
        TexDesc.Type      = DG::RESOURCE_DIM_TEX_2D;
        TexDesc.Usage     = DG::USAGE_IMMUTABLE;
        TexDesc.BindFlags = DG::BIND_SHADER_RESOURCE;
        TexDesc.Width     = TexDim;
        TexDesc.Height    = TexDim;
        TexDesc.Format    = DG::TEX_FORMAT_RGBA8_UNORM;
        TexDesc.MipLevels = 1;

        std::vector<DG::Uint32>     Data(TexDim * TexDim, 0xFFFFFFFF);
        DG::TextureSubResData       Level0Data{Data.data(), TexDim * 4};
        DG::TextureData             InitData{&Level0Data, 1};

		DG::ITexture* whiteTex = nullptr;
        device->CreateTexture(TexDesc, &InitData, &whiteTex);
        auto whiteSRV = whiteTex->GetDefaultView(DG::TEXTURE_VIEW_SHADER_RESOURCE);

        TexDesc.Name = "Black texture for renderer";
        for (auto& c : Data) c = 0;
        
		DG::ITexture* blackTex = nullptr;
        device->CreateTexture(TexDesc, &InitData, &blackTex);
        auto blackSRV = blackTex->GetDefaultView(DG::TEXTURE_VIEW_SHADER_RESOURCE);

        TexDesc.Name = "Default normal map for renderer";
        for (auto& c : Data) c = 0x00FF7F7F;

		DG::ITexture* defaultNormalTex = nullptr;
        device->CreateTexture(TexDesc, &InitData, &defaultNormalTex);
        auto defaultNormalSRV = defaultNormalTex->GetDefaultView(DG::TEXTURE_VIEW_SHADER_RESOURCE);

        // clang-format off
        DG::StateTransitionDesc Barriers[] = 
        {
            {whiteTex,         DG::RESOURCE_STATE_UNKNOWN, DG::RESOURCE_STATE_SHADER_RESOURCE, true},
            {blackTex,         DG::RESOURCE_STATE_UNKNOWN, DG::RESOURCE_STATE_SHADER_RESOURCE, true},
            {defaultNormalTex, DG::RESOURCE_STATE_UNKNOWN, DG::RESOURCE_STATE_SHADER_RESOURCE, true} 
        };

        // clang-format on
        context->TransitionResourceStates(_countof(Barriers), Barriers);

		DG::ISampler* sampler = nullptr;
        device->CreateSampler(Sam_LinearClamp, &sampler);
		mResources.mDefaultSampler = sampler;

        whiteSRV->SetSampler(sampler);
        blackSRV->SetSampler(sampler);
        defaultNormalSRV->SetSampler(sampler);

		mResources.mWhiteTexture = whiteTex;
		mResources.mBlackTexture = blackTex;
		mResources.mNormalTexture = defaultNormalTex;

		mResources.mWhiteSRV = whiteSRV;
		mResources.mBlackSRV = blackSRV;
		mResources.mDefaultNormalSRV = defaultNormalSRV;

		std::cout << "Precomputing Cook-Torrance BRDF..." << std::endl;

		mResources.mCookTorrenceLUT.Compute(mGraphics->Device(), context);

		context->SetRenderTargets(0, nullptr, nullptr,
			DG::RESOURCE_STATE_TRANSITION_MODE_NONE);
	}

	void DefaultRenderer::CreateLambertPipeline(Handle<DG::IShader> vs, Handle<DG::IShader> ps) {
		auto lambertVS = vs.Ptr();
		auto lambertPS = ps.Ptr();
		auto device = mGraphics->Device();

		auto anisotropyFactor = GetMaxAnisotropy();
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
		GraphicsPipeline.RTVFormats[0]                = GetIntermediateFramebufferFormat();
		GraphicsPipeline.PrimitiveTopology            = DG::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		GraphicsPipeline.RasterizerDesc.CullMode      = DG::CULL_MODE_BACK;
		GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
		GraphicsPipeline.DSVFormat 					  = GetIntermediateDepthbufferFormat();

		// Number of MSAA samples
		GraphicsPipeline.SmplDesc.Count = (DG::Uint8)GetMSAASamples();

		auto& layoutElements = mStaticMeshLayout.mElements;

		GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
		GraphicsPipeline.InputLayout.LayoutElements = &layoutElements[0];

		PSOCreateInfo.pVS = lambertVS;
		PSOCreateInfo.pPS = lambertPS;

		PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
		// clang-format off
		DG::ShaderResourceVariableDesc Vars[] = 
		{
			//{DG::SHADER_TYPE_PIXEL, "CameraData", DG::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
			{DG::SHADER_TYPE_VERTEX, "CameraData", DG::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
			{DG::SHADER_TYPE_PIXEL, "mAlbedo", DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
		};
		// clang-format on
		PSODesc.ResourceLayout.NumVariables = _countof(Vars);
		PSODesc.ResourceLayout.Variables    = Vars;

		// clang-format off
		DG::ImmutableSamplerDesc ImtblSamplers[] =
		{
			{DG::SHADER_TYPE_PIXEL, "mAlbedo_sampler", SamLinearClampDesc}
		};
		// clang-format on
		PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
		PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
		
		device->CreateGraphicsPipelineState(PSOCreateInfo, &result);

		mResources.mLambert.mStaticMeshPipeline	= result;
		result->Release();
	}

	void DefaultRenderer::CreateCookTorrenceIBLPipeline(Handle<DG::IShader> vs, Handle<DG::IShader> ps) {
		auto pbrStaticMeshVS = vs.Ptr();
		auto pbrStaticMeshPS = ps.Ptr();

		auto device = mGraphics->Device();

		auto anisotropyFactor = GetMaxAnisotropy();
		auto filterType = anisotropyFactor > 1 ? 
			DG::FILTER_TYPE_ANISOTROPIC : DG::FILTER_TYPE_LINEAR;

		DG::SamplerDesc SamLinearClampDesc
		{
			filterType, filterType, filterType, 
			DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP
		};

		DG::SamplerDesc SamLinearWrapDesc 
		{
			filterType, filterType, filterType, 
			DG::TEXTURE_ADDRESS_WRAP, DG::TEXTURE_ADDRESS_WRAP, DG::TEXTURE_ADDRESS_WRAP
		};

		SamLinearClampDesc.MaxAnisotropy = anisotropyFactor;
		SamLinearWrapDesc.MaxAnisotropy = anisotropyFactor;

		DG::IPipelineState* result = nullptr;

		DG::GraphicsPipelineStateCreateInfo PSOCreateInfo;
		DG::PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
		DG::GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

		PSODesc.Name         = "Static Mesh PBR Pipeline";
		PSODesc.PipelineType = DG::PIPELINE_TYPE_GRAPHICS;

		GraphicsPipeline.NumRenderTargets             = 1;
		GraphicsPipeline.RTVFormats[0]                = GetIntermediateFramebufferFormat();
		GraphicsPipeline.PrimitiveTopology            = DG::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		GraphicsPipeline.RasterizerDesc.CullMode      = DG::CULL_MODE_BACK;
		GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
		GraphicsPipeline.DepthStencilDesc.DepthFunc   = DG::COMPARISON_FUNC_LESS;
		GraphicsPipeline.DSVFormat 					  = GetIntermediateDepthbufferFormat();

		// Number of MSAA samples
		GraphicsPipeline.SmplDesc.Count = (DG::Uint8)GetMSAASamples();

		uint stride = mStaticMeshLayout.mStride;
		auto& layoutElements = mStaticMeshLayout.mElements;

		GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
		GraphicsPipeline.InputLayout.LayoutElements = &layoutElements[0];

		PSOCreateInfo.pVS = pbrStaticMeshVS;
		PSOCreateInfo.pPS = pbrStaticMeshPS;

		PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
		
		std::vector<DG::ShaderResourceVariableDesc> Vars;

		Vars.emplace_back(DG::ShaderResourceVariableDesc{
			DG::SHADER_TYPE_PIXEL, 
			"mAlbedo", 
			DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});
		Vars.emplace_back(DG::ShaderResourceVariableDesc{
			DG::SHADER_TYPE_PIXEL, 
			"mMetallic", 
			DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});
		Vars.emplace_back(DG::ShaderResourceVariableDesc{
			DG::SHADER_TYPE_PIXEL, 
			"mRoughness", 
			DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});
		Vars.emplace_back(DG::ShaderResourceVariableDesc{
			DG::SHADER_TYPE_PIXEL, 
			"mNormalMap", 
			DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});
		Vars.emplace_back(DG::ShaderResourceVariableDesc{
			DG::SHADER_TYPE_PIXEL, 
			"IrradianceSH", 
			DG::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE});
		Vars.emplace_back(DG::ShaderResourceVariableDesc{
			DG::SHADER_TYPE_PIXEL, 
			"mPrefilteredEnvMap", 
			DG::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE});
		Vars.emplace_back(DG::ShaderResourceVariableDesc{
			DG::SHADER_TYPE_PIXEL, 
			"mBRDF_LUT", 
			DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC});

		PSODesc.ResourceLayout.NumVariables = Vars.size();
		PSODesc.ResourceLayout.Variables    = &Vars[0];

		std::vector<DG::ImmutableSamplerDesc> ImtblSamplers;

		ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
			DG::SHADER_TYPE_PIXEL, "mAlbedo_sampler", SamLinearWrapDesc
		});
		ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
			DG::SHADER_TYPE_PIXEL, "mRoughness_sampler", SamLinearWrapDesc
		});
		ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
			DG::SHADER_TYPE_PIXEL, "mMetallic_sampler", SamLinearWrapDesc
		});
		ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
			DG::SHADER_TYPE_PIXEL, "mNormalMap_sampler", SamLinearWrapDesc
		});
		ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
			DG::SHADER_TYPE_PIXEL, "mPrefilteredEnvMap_sampler", SamLinearClampDesc
		});
		ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
			DG::SHADER_TYPE_PIXEL, "mBRDF_LUT_sampler", SamLinearClampDesc
		});

		// clang-format on
		PSODesc.ResourceLayout.NumImmutableSamplers = ImtblSamplers.size();
		PSODesc.ResourceLayout.ImmutableSamplers    = &ImtblSamplers[0];
		
		device->CreateGraphicsPipelineState(PSOCreateInfo, &result);

		auto lutVar = result->GetStaticVariableByName(DG::SHADER_TYPE_PIXEL, "mBRDF_LUT");
		if (lutVar)
			lutVar->Set(mResources.mCookTorrenceLUT.GetShaderView());
		
		mResources.mCookTorrenceIBL.mStaticMeshPipeline = result;
		result->Release();
	}

	void DefaultRenderer::CreateSkyboxPipeline(Handle<DG::IShader> vs, 
		Handle<DG::IShader> ps) {
		auto skyboxVS = vs.Ptr();
		auto skyboxPS = ps.Ptr();
		auto device = mGraphics->Device();

		auto anisotropyFactor = GetMaxAnisotropy();
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
		GraphicsPipeline.RTVFormats[0]                = GetIntermediateFramebufferFormat();
		GraphicsPipeline.PrimitiveTopology            = DG::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		GraphicsPipeline.RasterizerDesc.CullMode      = DG::CULL_MODE_NONE;
		GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
		GraphicsPipeline.DepthStencilDesc.DepthFunc   = DG::COMPARISON_FUNC_LESS_EQUAL;
		GraphicsPipeline.DSVFormat 					  = GetIntermediateDepthbufferFormat();

		// Number of MSAA samples
		GraphicsPipeline.SmplDesc.Count = (DG::Uint8)GetMSAASamples();

		GraphicsPipeline.InputLayout.NumElements = 0;

		PSOCreateInfo.pVS = skyboxVS;
		PSOCreateInfo.pPS = skyboxPS;

		PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
		// clang-format off
		DG::ShaderResourceVariableDesc Vars[] = 
		{
			{DG::SHADER_TYPE_PIXEL, "mTexture", DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
			{DG::SHADER_TYPE_VERTEX, "CameraData", DG::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
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

		mResources.mSkybox.mPipeline 	= result;
		result->Release();
	}

	bool DefaultRenderer::IsInitialized() const {
		return bIsInitialized;
	}

	void DefaultRenderer::Shutdown() {
		bIsInitialized = false;	

		// Clear resources
		mResources = std::move(Resources());
	}

	void DefaultRenderer::NewFrame(Frame* frame) {
		mUpdater.SetFrame(frame);
		mUpdater.UpdateAll();
	}

	const VertexLayout& DefaultRenderer::GetStaticMeshLayout() const {
		return mStaticMeshLayout;
	}

	GraphicsCapabilityConfig DefaultRenderer::GetCapabilityConfig() const {
		return GraphicsCapabilityConfig();
	}

	uint DefaultRenderer::GetMaxAnisotropy() const {
		return 16u;
	}

	uint DefaultRenderer::GetMSAASamples() const {
		return 1u;
	}

	DG::TEXTURE_FORMAT DefaultRenderer::GetIntermediateFramebufferFormat() const {
		return mGraphics->SwapChain()->GetDesc().ColorBufferFormat;
	}
	
	DG::TEXTURE_FORMAT DefaultRenderer::GetIntermediateDepthbufferFormat() const {
		return mGraphics->SwapChain()->GetDesc().DepthBufferFormat;
	}

	MaterialId DefaultRenderer::CreateUnmanagedMaterial(const MaterialDesc& desc) {
		return entt::null;
	}

	void DefaultRenderer::AddMaterialRef(MaterialId id) {

	}

	void DefaultRenderer::ReleaseMaterial(MaterialId id) {

	}

	void DefaultRenderer::OnAddedTo(SystemCollection& collection) {
		collection.RegisterInterface<IRenderer>(this);
		collection.RegisterInterface<IVertexFormatProvider>(this);
	}
}