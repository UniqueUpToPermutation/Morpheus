#include <Engine/Systems/DefaultRenderer.hpp>
#include <Engine/Resources/Shader.hpp>
#include <Engine/Components/SkyboxComponent.hpp>
#include <Engine/Components/StaticMeshComponent.hpp>
#include <Engine/RendererTransformCache.hpp>

namespace Morpheus {

	VertexLayout DefaultInstancedStaticMeshLayout() {
		uint stride = 12 * sizeof(float);

		VertexLayout layout;

		layout.mElements = {
			DG::LayoutElement(0, 0, 3, DG::VT_FLOAT32, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(1, 0, 2, DG::VT_FLOAT32, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(2, 0, 3, DG::VT_FLOAT32, false, 
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
		layout.mTangent = 3;

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

	std::unique_ptr<Task> DefaultRenderer::Startup(SystemCollection& collection) {
		
		typedef Future<Handle<DG::IShader>> shader_future_t;

		shader_future_t staticMeshResult;
		shader_future_t lambertPSResult;
		shader_future_t cookTorrenceResult;
		shader_future_t skyboxVSResult;
		shader_future_t skyboxPSResult;

		ShaderPreprocessorConfig config;
		config.mDefines["USE_IBL"] = "1";
		config.mDefines["USE_SH"] = "1";

		LoadParams<RawShader> staticMeshParams("internal/StaticMesh.vsh",
			DG::SHADER_TYPE_VERTEX,
			"Static Mesh VS", 
			config);

		LoadParams<RawShader> lambertPSParams("internal/LambertIBL.psh",
			DG::SHADER_TYPE_PIXEL,
			"Lambert PS",
			config);

		LoadParams<RawShader> cookTorrenceParams("internal/CookTorranceIBL.psh",
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

		staticMeshResult = LoadShaderHandle(device, staticMeshParams);
		lambertPSResult = LoadShaderHandle(device, lambertPSParams);
		cookTorrenceResult = LoadShaderHandle(device, cookTorrenceParams);
		skyboxVSResult = LoadShaderHandle(device, skyboxVSParams);
		skyboxPSResult = LoadShaderHandle(device, skyboxPSParams);

		FunctionPrototype<
			shader_future_t, 
			shader_future_t, 
			shader_future_t, 
			shader_future_t, 
			shader_future_t> createPipelinesPrototype([this](const TaskParams& e,
				shader_future_t staticMeshResult,
				shader_future_t lambertPSResult,
				shader_future_t cookTorrenceResult,
				shader_future_t skyboxVSResult,
				shader_future_t skyboxPSResult) {
				
				// Acquire loaded resources
			mResources.mCookTorrenceIBL.mCookTorrencePS 
											= cookTorrenceResult.Get();
			mResources.mStaticMeshVS 		= staticMeshResult.Get();
			mResources.mLambertIBL.mLambertPS 	= lambertPSResult.Get();
			mResources.mSkybox.mVS 			= skyboxVSResult.Get();
			mResources.mSkybox.mPS 			= skyboxPSResult.Get();

			InitializeDefaultResources();
			CreateLambertPipeline(mResources.mStaticMeshVS, mResources.mLambertIBL.mLambertPS);
			CreateCookTorrenceIBLPipeline(mResources.mStaticMeshVS, mResources.mCookTorrenceIBL.mCookTorrencePS);
			CreateSkyboxPipeline(mResources.mSkybox.mVS, mResources.mSkybox.mPS);
		
			auto device = mGraphics->Device();
		
			// Skybox stuff
			DG::IShaderResourceBinding* skyboxBinding = nullptr;
			mResources.mSkybox.mPipeline->CreateShaderResourceBinding(&skyboxBinding, true);
			mResources.mSkybox.mSkyboxBinding.Adopt(skyboxBinding);
		
			mResources.mSkybox.mTexture = skyboxBinding->GetVariableByName(
				DG::SHADER_TYPE_PIXEL, "mTexture");
		});

		CustomTask startupTask;
		startupTask.Add(
			createPipelinesPrototype(
				staticMeshResult, 
				lambertPSResult, 
				cookTorrenceResult, 
				skyboxVSResult, 
				skyboxPSResult)
			.SetName("Create Pipelines")
			.OnlyThread(THREAD_MAIN));

		// Update all transform caches
		render_proto_t updateTransformCache([this](const TaskParams& e, Future<RenderParams> params) {
			CacheUpdater().UpdateChanges();
		});

		auto& frameProcessor = collection.GetFrameProcessor();
		
		mRenderGroup = Render(frameProcessor.GetRenderInput());

		auto updateTransformCacheNode = 
			updateTransformCache(frameProcessor.GetRenderInput())
				.SetName("Update Transforms");
		updateTransformCacheNode.Before(mRenderGroup->InNode());

		// Give this task to the frame processor
		frameProcessor.AddRenderTask(updateTransformCacheNode);
		frameProcessor.AddRenderTask(mRenderGroup);
		
		return std::make_unique<Task>(std::move(startupTask));
	}

	TaskNode DefaultRenderer::BeginRender(Future<RenderParams> params) {
		render_proto_t prototype([this](const TaskParams& e, Future<RenderParams> f_params) {
			auto graphics = mGraphics;
			auto context = graphics->ImmediateContext();
			auto swapChain = graphics->SwapChain();
			auto rtv = swapChain->GetCurrentBackBufferRTV();
			auto dsv = swapChain->GetDepthBufferDSV();
			float color[] = { 0.5f, 0.5f, 1.0f, 1.0f };

			const auto& params = f_params.Get();

			// Clear the back buffer (if necessary) and the depth buffer
			context->SetRenderTargets(1, &rtv, dsv, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			
			if (params.mFrame->Registry().view<SkyboxComponent>().size() == 0)
				context->ClearRenderTarget(rtv, color, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			
			context->ClearDepthStencil(dsv, DG::CLEAR_DEPTH_FLAG, 1.0f, 0, 
				DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			// Write global camera data
			if (params.mFrame->GetCamera() != entt::null) {
				auto& cameraData = params.mFrame->CameraData();
				
				HLSL::ViewAttribs gpuViewData;
				gpuViewData.mCamera = cameraData.GetTransformedAttribs(
					params.mFrame->GetCamera(), 
					&params.mFrame->Registry(),
					*graphics);

				Resources().mViewData.Write(graphics->ImmediateContext(), gpuViewData);

			} else {
				Camera camera;
				HLSL::ViewAttribs gpuViewData;
				gpuViewData.mCamera = camera.GetLocalAttribs(*graphics);
				Resources().mViewData.Write(graphics->ImmediateContext(), gpuViewData);
				
				std::cout << "WARNING: No camera entity found!" << std::endl;
			}
			
		});

		return prototype(params)
			.SetName("Begin Render")
			.OnlyThread(THREAD_MAIN);
	}

	TaskNode DefaultRenderer::DrawStaticMeshes(Future<RenderParams> params) {
		render_proto_t prototype([this](const TaskParams& e, Future<RenderParams> f_params) {
			const auto& params = f_params.Get(); 

			auto skyboxes = params.mFrame->Registry().view<SkyboxComponent>();

			auto ent = skyboxes.front();
			if (ent != entt::null) {
				auto& skybox = skyboxes.get<SkyboxComponent>(ent);
				auto lightProbe = params.mFrame->TryGet<LightProbe>(ent);

				MaterialApplyParams applyParams;
				applyParams.mGlobalLightProbe = lightProbe;

				MaterialId currentMaterial = NullMaterialId;

				auto context = GetGraphics()->ImmediateContext();
				auto meshView = params.mFrame->Registry().view<StaticMeshComponent>();

				auto currentIt = meshView.begin();
				auto endIt = meshView.end();

				while (currentIt != endIt) {
					auto instanceBuffer = Resources().mInstanceBuffer.Ptr();

					// First upload transforms to GPU buffer
					void* mappedData;
					context->MapBuffer(instanceBuffer, DG::MAP_WRITE, 
						DG::MAP_FLAG_DISCARD, mappedData);

					DG::float4x4* ptr = reinterpret_cast<DG::float4x4*>(mappedData);
					int maxInstances = instanceBuffer->GetDesc().uiSizeInBytes / sizeof(DG::float4x4);
					int transformWriteIdx = 0;
					int transformReadIdx = 0;

					auto matrixCopyIt = currentIt;
					for (; matrixCopyIt != endIt && transformWriteIdx < maxInstances;
						++transformWriteIdx, ++matrixCopyIt) {
						auto transformCache = params.mFrame->TryGet<RendererTransformCache>(*matrixCopyIt);
						
						if (transformCache)
							ptr[transformWriteIdx] = transformCache->mCache.Transpose();
						else 
							ptr[transformWriteIdx] = DG::float4x4::Identity();
					}

					context->UnmapBuffer(instanceBuffer, DG::MAP_WRITE);

					while (currentIt != matrixCopyIt) {
						auto& staticMesh = meshView.get<StaticMeshComponent>(*currentIt);

						auto& material = staticMesh.mMaterial;
						auto& geometry = staticMesh.mGeometry;

						// Change pipeline
						if (material != NullMaterialId) {
							ApplyMaterial(context, material, applyParams);
							currentMaterial = material;
						}

						// Count the number of instances of this specific mesh to render
						int instanceCount = 1;
						for (++currentIt; currentIt != matrixCopyIt
							&& meshView.get<StaticMeshComponent>(*currentIt).mGeometry == geometry 
							&& meshView.get<StaticMeshComponent>(*currentIt).mMaterial == material;
							++instanceCount, ++currentIt);

						if (material != NullMaterialId) {
							// Render all of these static meshes in a batch
							DG::Uint32  offsets[] = { 0, (DG::Uint32)(transformReadIdx * sizeof(DG::float4x4)) };
							DG::IBuffer* pBuffs[] = { geometry->GetVertexBuffer(), instanceBuffer };
							context->SetVertexBuffers(0, 2, pBuffs, offsets, 
								DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, 
								DG::SET_VERTEX_BUFFERS_FLAG_RESET);
							context->SetIndexBuffer( geometry->GetIndexBuffer(), 0, 
								DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

							DG::DrawIndexedAttribs attribs = geometry->GetIndexedDrawAttribs();
							attribs.Flags = DG::DRAW_FLAG_VERIFY_ALL;
							attribs.NumInstances = instanceCount;
							context->DrawIndexed(attribs);
						}

						transformReadIdx += instanceCount;
					}
				}
			}
		});

		return prototype(params)
			.SetName("Draw Static Meshes")
			.OnlyThread(THREAD_MAIN);
	}
	
	TaskNode DefaultRenderer::DrawBackground(Future<RenderParams> params) {
		render_proto_t prototype([this](const TaskParams& e, Future<RenderParams> f_params) {
			const auto& params = f_params.Get();
			auto skyboxes = params.mFrame->Registry().view<SkyboxComponent>();

			auto ent = skyboxes.front();
			if (ent != entt::null) {
				auto& skybox = skyboxes.get<SkyboxComponent>(ent);

				// Render skybox
				auto context = GetGraphics()->ImmediateContext();
				Resources().mSkybox.mTexture->Set(skybox.mCubemap->GetShaderView());

				context->SetPipelineState(Resources().mSkybox.mPipeline.Ptr());
				context->CommitShaderResources(Resources().mSkybox.mSkyboxBinding.Ptr(),
					DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

				DG::DrawAttribs attribs;
				attribs.NumVertices = 4;
				context->Draw(attribs);
			}
		});
		
		return prototype(params)
			.SetName("Draw Skybox")
			.OnlyThread(THREAD_MAIN);
	}
	
	std::shared_ptr<Task> DefaultRenderer::Render(Future<RenderParams> params) {
		auto group = std::make_shared<CustomTask>();

		auto beginRender = BeginRender(params);
		auto drawBackground = DrawBackground(params);
		auto drawStaticMeshes = DrawStaticMeshes(params);
		
		// Draw background after begin render stuff
		drawBackground.After(beginRender);
		drawStaticMeshes.After(beginRender);

		group->Add(beginRender);
		group->Add(drawBackground);
		group->Add(drawStaticMeshes);

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

		mResources.mViewData.Initialize(mGraphics->Device());
		mResources.mMaterialData.Initialize(mGraphics->Device());
	}

	MaterialId DefaultRenderer::CreateNewMaterialId() {
		++mCurrentMaterialId;
		mCurrentMaterialId = std::max(0, mCurrentMaterialId);
		return mCurrentMaterialId;
	}
	
	void DefaultRenderer::ApplyMaterial(DG::IDeviceContext* context, MaterialId id, 
		const MaterialApplyParams& params) {
		auto it = mMaterials.find(id);

		if (it != mMaterials.end()) {
			auto& mat = it->second;
			switch (mat.mDesc.mParams.mType) {
			case MaterialType::COOK_TORRENCE:
				OnApplyCookTorrence(mat, params);
				break;
			case MaterialType::LAMBERT:
				OnApplyLambert(mat, params);
				break;
			case MaterialType::CUSTOM:
				break;
			}

			HLSL::MaterialAttribs attribs;
			attribs.mAlbedoFactor = mat.mDesc.mParams.mAlbedoFactor;
			attribs.mDisplacementFactor = mat.mDesc.mParams.mDisplacementFactor;
			attribs.mMetallicFactor = mat.mDesc.mParams.mMetallicFactor;
			attribs.mRoughnessFactor = mat.mDesc.mParams.mRoughnessFactor;

			mResources.mMaterialData.Write(context, attribs);

			context->SetPipelineState(mat.mPipeline);
			context->CommitShaderResources(mat.mBinding, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		} else {
			std::cout << "WARNING: Material Id: " << id << " cannot be found!" << std::endl;
		}
	}

	DefaultRenderer::Material DefaultRenderer::CreateCookTorrenceMaterial(const MaterialDesc& desc) {
		DG::IShaderResourceBinding* binding = nullptr;

		Resources().mCookTorrenceIBL.mStaticMeshPipeline
			->CreateShaderResourceBinding(&binding, true);

		DG::ITextureView* albedo = nullptr;
		DG::ITextureView* normal = nullptr;
		DG::ITextureView* roughness = nullptr;
		DG::ITextureView* metallic = nullptr;

		if (desc.mResources.mAlbedo)
			albedo = desc.mResources.mAlbedo->GetShaderView();
		else
			albedo = Resources().mWhiteSRV;

		if (desc.mResources.mNormal)
			normal = desc.mResources.mNormal->GetShaderView();
		else
			normal = Resources().mDefaultNormalSRV;
	
		if (desc.mResources.mRoughness)
			roughness = desc.mResources.mRoughness->GetShaderView();
		else
			roughness = Resources().mWhiteSRV;
	
		if (desc.mResources.mMetallic)
			metallic = desc.mResources.mMetallic->GetShaderView();
		else
			metallic = Resources().mWhiteSRV;

		// Optimize Later
		binding->GetVariableByName(DG::SHADER_TYPE_PIXEL,
			"mAlbedo")->Set(albedo);
		binding->GetVariableByName(DG::SHADER_TYPE_PIXEL,
			"mNormalMap")->Set(normal);
		binding->GetVariableByName(DG::SHADER_TYPE_PIXEL,
			"mRoughness")->Set(roughness);
		binding->GetVariableByName(DG::SHADER_TYPE_PIXEL,
			"mMetallic")->Set(metallic);

		Material mat(desc, 
			Resources().mCookTorrenceIBL.mStaticMeshPipeline.Ptr(), 
			binding);

		mat.mIrradianceSHVariable = binding->GetVariableByName(DG::SHADER_TYPE_PIXEL, "IrradianceSH");
		mat.mEnvMapVariable = binding->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mPrefilteredEnvMap");
	
		return mat;
	}

	void DefaultRenderer::OnApplyCookTorrence(
		DefaultRenderer::Material& mat, 
		const DefaultRenderer::MaterialApplyParams& params) {
		if (params.mGlobalLightProbe) {
			mat.mIrradianceSHVariable->Set(params.mGlobalLightProbe->GetIrradianceSH());
			mat.mEnvMapVariable->Set(params.mGlobalLightProbe->GetPrefilteredEnvView());
		}
	}

	void DefaultRenderer::OnApplyLambert(DefaultRenderer::Material& mat, 
		const MaterialApplyParams& params) {
		if (params.mGlobalLightProbe) {
			mat.mIrradianceSHVariable->Set(params.mGlobalLightProbe->GetIrradianceSH());
		}
	}

	DefaultRenderer::Material DefaultRenderer::CreateLambertMaterial(const MaterialDesc& desc) {
		DG::IShaderResourceBinding* binding = nullptr;

		Resources().mLambertIBL.mStaticMeshPipeline
			->CreateShaderResourceBinding(&binding, true);

		DG::ITextureView* albedo = nullptr;
		DG::ITextureView* normal = nullptr;

		if (desc.mResources.mAlbedo)
			albedo = desc.mResources.mAlbedo->GetShaderView();
		else
			albedo = Resources().mWhiteSRV;

		if (desc.mResources.mNormal)
			normal = desc.mResources.mNormal->GetShaderView();
		else
			normal = Resources().mDefaultNormalSRV;

		// Optimize later
		binding->GetVariableByName(DG::SHADER_TYPE_PIXEL, 
			"mAlbedo")->Set(albedo);
		binding->GetVariableByName(DG::SHADER_TYPE_PIXEL,
			"mNormalMap")->Set(normal);

		Material mat(desc, 
			Resources().mLambertIBL.mStaticMeshPipeline.Ptr(), 
			binding);

		mat.mIrradianceSHVariable = 
			binding->GetVariableByName(DG::SHADER_TYPE_PIXEL, "IrradianceSH");

		return mat;
	}

	void DefaultRenderer::CreateLambertPipeline(Handle<DG::IShader> vs, Handle<DG::IShader> ps) {
		auto lambertVS = vs.Ptr();
		auto lambertPS = ps.Ptr();
		auto device = mGraphics->Device();

		auto anisotropyFactor = GetMaxAnisotropy();
		auto filterType = anisotropyFactor > 1 ? DG::FILTER_TYPE_ANISOTROPIC : DG::FILTER_TYPE_LINEAR;

		DG::SamplerDesc SamLinearWrapDesc
		{
			filterType, filterType, filterType, 
			DG::TEXTURE_ADDRESS_WRAP, DG::TEXTURE_ADDRESS_WRAP, DG::TEXTURE_ADDRESS_WRAP
		};

		SamLinearWrapDesc.MaxAnisotropy = anisotropyFactor;

		DG::IPipelineState* result = nullptr;

		// Create Irradiance Pipeline
		DG::GraphicsPipelineStateCreateInfo PSOCreateInfo;
		DG::PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
		DG::GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

		PSODesc.Name         = "Lambert Pipeline";
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
			{DG::SHADER_TYPE_VERTEX, "ViewData", DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC},

			{DG::SHADER_TYPE_PIXEL, "ViewData", DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
			{DG::SHADER_TYPE_PIXEL, "BatchData", DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
			{DG::SHADER_TYPE_PIXEL, "mAlbedo", DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
			{DG::SHADER_TYPE_PIXEL, "mNormalMap", DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
			{DG::SHADER_TYPE_PIXEL, "IrradianceSH", DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
		};
		// clang-format on
		PSODesc.ResourceLayout.NumVariables = _countof(Vars);
		PSODesc.ResourceLayout.Variables    = Vars;

		// clang-format off
		DG::ImmutableSamplerDesc ImtblSamplers[] =
		{
			{DG::SHADER_TYPE_PIXEL, "mAlbedo_sampler", SamLinearWrapDesc},
			{DG::SHADER_TYPE_PIXEL, "mNormalMap_sampler", SamLinearWrapDesc}
		};
		// clang-format on
		PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
		PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
		
		device->CreateGraphicsPipelineState(PSOCreateInfo, &result);

		result->GetStaticVariableByName(DG::SHADER_TYPE_VERTEX, "ViewData")
			->Set(Resources().mViewData.Get());
		result->GetStaticVariableByName(DG::SHADER_TYPE_PIXEL, "ViewData")
			->Set(Resources().mViewData.Get());
		result->GetStaticVariableByName(DG::SHADER_TYPE_PIXEL, "BatchData")
			->Set(Resources().mMaterialData.Get());

		mResources.mLambertIBL.mStaticMeshPipeline.Adopt(result);
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

		auto& layoutElements = mStaticMeshLayout.mElements;

		GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
		GraphicsPipeline.InputLayout.LayoutElements = &layoutElements[0];

		PSOCreateInfo.pVS = pbrStaticMeshVS;
		PSOCreateInfo.pPS = pbrStaticMeshPS;

		PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
		
		std::vector<DG::ShaderResourceVariableDesc> Vars;

		Vars.emplace_back(DG::ShaderResourceVariableDesc{
			DG::SHADER_TYPE_VERTEX,
			"ViewData",
			DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC
		});
		Vars.emplace_back(DG::ShaderResourceVariableDesc{
			DG::SHADER_TYPE_PIXEL,
			"ViewData",
			DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC
		});

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

		result->GetStaticVariableByName(DG::SHADER_TYPE_PIXEL, "mBRDF_LUT")
			->Set(mResources.mCookTorrenceLUT.GetShaderView());
		result->GetStaticVariableByName(DG::SHADER_TYPE_VERTEX, "ViewData")
			->Set(mResources.mViewData.Get());
		result->GetStaticVariableByName(DG::SHADER_TYPE_PIXEL, "ViewData")
			->Set(mResources.mViewData.Get());
		result->GetStaticVariableByName(DG::SHADER_TYPE_PIXEL, "BatchData")
			->Set(mResources.mMaterialData.Get());
		
		mResources.mCookTorrenceIBL.mStaticMeshPipeline.Adopt(result);
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
			{DG::SHADER_TYPE_VERTEX, "ViewData", DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
			{DG::SHADER_TYPE_PIXEL, "ViewData", DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
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

		result->GetStaticVariableByName(DG::SHADER_TYPE_VERTEX, "ViewData")
			->Set(Resources().mViewData.Get());
		result->GetStaticVariableByName(DG::SHADER_TYPE_PIXEL, "ViewData")
			->Set(Resources().mViewData.Get());

		mResources.mSkybox.mPipeline.Adopt(result);
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
		Material mat;
		
		switch (desc.mParams.mType) {
			case MaterialType::COOK_TORRENCE:
				mat = CreateCookTorrenceMaterial(desc);
				break;
			case MaterialType::LAMBERT:
				mat = CreateLambertMaterial(desc);
				break;
			case MaterialType::CUSTOM:
				return NullMaterialId;
		}

		auto id = CreateNewMaterialId();
		mMaterials.emplace(id, std::move(mat));

		return id;
	}

	void DefaultRenderer::AddMaterialRef(MaterialId id) {
		std::lock_guard<std::mutex> lock(mMaterialAddRefMutex);
		mMaterialsToAddRef.emplace_back(id);
	}

	void DefaultRenderer::ReleaseMaterial(MaterialId id) {
		std::lock_guard<std::mutex> lock(mMaterialReleaseMutex);
		mMaterialsToRelease.emplace_back(id);
	}

	MaterialDesc DefaultRenderer::GetMaterialDesc(MaterialId id) {
		std::shared_lock lock(mMaterialsMutex);
		auto it = mMaterials.find(id);
		if (it == mMaterials.end())
			throw std::runtime_error("Material does not exist!");
		else
			return it->second.mDesc;
	}

	void DefaultRenderer::OnAddedTo(SystemCollection& collection) {
		collection.RegisterInterface<IRenderer>(this);
		collection.RegisterInterface<IVertexFormatProvider>(this);
	}
}