#include <Engine/Engine.hpp>
#include <Engine/Platform.hpp>
#include <Engine/DefaultRendererOld.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/GeometryResource.hpp>
#include <Engine/Resources/TextureResource.hpp>
#include <Engine/Resources/MaterialResource.hpp>
#include <Engine/Systems/RendererBridge.hpp>
#include <Engine/Pipelines/ImageBasedLightingView.hpp>

#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cmath>

#include "PlatformDefinitions.h"
#include "Errors.hpp"
#include "StringTools.hpp"
#include "Image.h"
#include "FileWrapper.hpp"

#define INTERMEDIATE_TEXTURE_FORMAT DG::TEX_FORMAT_RGBA16_FLOAT

#if D3D11_SUPPORTED
#    include "EngineFactoryD3D11.h"
#endif

#if D3D12_SUPPORTED
#    include "EngineFactoryD3D12.h"
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
#    include "EngineFactoryOpenGL.h"
#endif

#if VULKAN_SUPPORTED
#    include "EngineFactoryVk.h"
#endif

#if METAL_SUPPORTED
#    include "EngineFactoryMtl.h"
#endif

#include "imgui.h"
#include "ImGuiImplDiligent.hpp"
#include "ImGuiUtils.hpp"

#if PLATFORM_LINUX
#if VULKAN_SUPPORTED
#    include "ImGuiImplLinuxXCB.hpp"
#endif
#include "ImGuiImplLinuxX11.hpp"
#endif

using namespace Diligent;

namespace Morpheus {

	// Common sampler states
	static const SamplerDesc Sam_LinearClamp
	{
		FILTER_TYPE_LINEAR,
		FILTER_TYPE_LINEAR,
		FILTER_TYPE_LINEAR, 
		TEXTURE_ADDRESS_CLAMP,
		TEXTURE_ADDRESS_CLAMP,
		TEXTURE_ADDRESS_CLAMP
	};

	void DefaultRenderer::RequestConfiguration(DG::EngineD3D11CreateInfo* info) {
	}

	void DefaultRenderer::RequestConfiguration(DG::EngineD3D12CreateInfo* info) {
	}

	void DefaultRenderer::RequestConfiguration(DG::EngineGLCreateInfo* info) {
	}

	void DefaultRenderer::RequestConfiguration(DG::EngineVkCreateInfo* info) {
	}

	void DefaultRenderer::RequestConfiguration(DG::EngineMtlCreateInfo* info) {
	}

	DefaultRenderer::DefaultRenderer(uint instanceBatchSize) :
		mInstanceBuffer(nullptr),
		mFrameBuffer(nullptr),
		mResolveBuffer(nullptr),
		mMSAADepthBuffer(nullptr),
		mInstanceBatchSize(instanceBatchSize),
		bUseSHIrradiance(true) {
	}

	
	void DefaultRenderer::Initialize(Engine* engine) {

		mEngine = engine;
		mGlobals.Initialize(engine->GetDevice());
		
		auto device = mEngine->GetDevice();
		auto context = mEngine->GetImmediateContext();

		DG::BufferDesc desc;
		desc.Name = "Renderer Instance Buffer";
		desc.Usage = DG::USAGE_DYNAMIC;
		desc.BindFlags = DG::BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = DG::CPU_ACCESS_WRITE;
		desc.uiSizeInBytes = sizeof(DG::float4x4) * mInstanceBatchSize;

		device->CreateBuffer(desc, nullptr, &mInstanceBuffer);

		// Create default textures
		static constexpr Uint32 TexDim = 8;

        TextureDesc TexDesc;
        TexDesc.Name      = "White texture for renderer";
        TexDesc.Type      = RESOURCE_DIM_TEX_2D;
        TexDesc.Usage     = USAGE_IMMUTABLE;
        TexDesc.BindFlags = BIND_SHADER_RESOURCE;
        TexDesc.Width     = TexDim;
        TexDesc.Height    = TexDim;
        TexDesc.Format    = TEX_FORMAT_RGBA8_UNORM;
        TexDesc.MipLevels = 1;

        std::vector<Uint32>     Data(TexDim * TexDim, 0xFFFFFFFF);
        TextureSubResData       Level0Data{Data.data(), TexDim * 4};
        TextureData             InitData{&Level0Data, 1};

		DG::ITexture* whiteTex = nullptr;
        device->CreateTexture(TexDesc, &InitData, &whiteTex);
        auto whiteSRV = whiteTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

        TexDesc.Name = "Black texture for renderer";
        for (auto& c : Data) c = 0;
        
		DG::ITexture* blackTex = nullptr;
        device->CreateTexture(TexDesc, &InitData, &blackTex);
        auto blackSRV = blackTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

        TexDesc.Name = "Default normal map for renderer";
        for (auto& c : Data) c = 0x00FF7F7F;

		DG::ITexture* defaultNormalTex = nullptr;
        device->CreateTexture(TexDesc, &InitData, &defaultNormalTex);
        auto defaultNormalSRV = defaultNormalTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

        // clang-format off
        StateTransitionDesc Barriers[] = 
        {
            {whiteTex,         RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, true},
            {blackTex,         RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, true},
            {defaultNormalTex, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, true} 
        };

        // clang-format on
        context->TransitionResourceStates(_countof(Barriers), Barriers);

        mDefaultSampler = nullptr;
        device->CreateSampler(Sam_LinearClamp, &mDefaultSampler);
        whiteSRV->SetSampler(mDefaultSampler);
        blackSRV->SetSampler(mDefaultSampler);
        defaultNormalSRV->SetSampler(mDefaultSampler);

		auto textureCache = mEngine->GetResourceManager()->GetCache<TextureResource>();
		mWhiteTexture = textureCache->MakeResource(
			whiteTex, "WHITE_TEXTURE");
		mBlackTexture = textureCache->MakeResource(
			blackTex, "BLACK_TEXTURE");
		mDefaultNormalTexture = textureCache->MakeResource(
			defaultNormalTex, "DEFAULT_NORMAL_TEXTURE");

		std::cout << "Precomputing Cook-Torrance BRDF..." << std::endl;

		mCookTorranceLut.Compute(mEngine->GetDevice(), context);

		context->SetRenderTargets(0, nullptr, nullptr,
			RESOURCE_STATE_TRANSITION_MODE_NONE);

		auto swapDesc = mEngine->GetSwapChain()->GetDesc();
		ReallocateIntermediateFramebuffer(swapDesc.Width, swapDesc.Height);

		mPostProcessor.Initialize(mEngine->GetDevice(),
			swapDesc.ColorBufferFormat,
			swapDesc.DepthBufferFormat);
	}

	DefaultRenderer::~DefaultRenderer() {
		mInstanceBuffer->Release();

		if (mFrameBuffer) {
			mFrameBuffer->Release();
			mFrameBuffer = nullptr;
		}

		if (mResolveBuffer) {
			mResolveBuffer->Release();
			mResolveBuffer = nullptr;
		}

		if (mMSAADepthBuffer) {
			mMSAADepthBuffer->Release();
			mMSAADepthBuffer = nullptr;
		}

		mDefaultSampler->Release();
		mWhiteTexture->Release();
		mBlackTexture->Release();
		mDefaultNormalTexture->Release();
	}

	void DefaultRenderer::WriteGlobalData(EntityNode cameraNode, LightProbe* globalLightProbe) {
		Camera* camera = cameraNode.TryGet<Camera>();
		MatrixTransformCache* cameraTransformCache = cameraNode.TryGet<MatrixTransformCache>();

		auto context = mEngine->GetImmediateContext();
		auto projection = camera->GetProjection(mEngine);
		auto swapChain = mEngine->GetSwapChain();
		auto& swapChainDesc = swapChain->GetDesc();
		DG::float2 viewportSize((float)swapChainDesc.Width, (float)swapChainDesc.Height);

		WriteRenderGlobalsData(&mGlobals, context, viewportSize, camera,
			projection,
			(cameraTransformCache == nullptr ? nullptr : &cameraTransformCache->mCache),
			globalLightProbe);
	}

	void DefaultRenderer::ReallocateIntermediateFramebuffer(
		uint width, uint height) {

		if (mFrameBuffer) {
			mFrameBuffer->Release();
			mFrameBuffer = nullptr;
		}

		if (mResolveBuffer) {
			mResolveBuffer->Release();
			mResolveBuffer = nullptr;
		}

		if (mMSAADepthBuffer) {
			mMSAADepthBuffer->Release();
			mMSAADepthBuffer = nullptr;
		}

		auto device = mEngine->GetDevice();

		DG::TextureDesc desc;
		desc.Width = width;
		desc.Height = height;
		desc.BindFlags = DG::BIND_RENDER_TARGET | DG::BIND_SHADER_RESOURCE;
		desc.Name = "Intermediate Framebuffer";
		desc.Type = DG::RESOURCE_DIM_TEX_2D;
		desc.Usage = DG::USAGE_DEFAULT;
		desc.Format = GetIntermediateFramebufferFormat();
		desc.MipLevels = 1;
		desc.SampleCount = GetMSAASamples();
		
		device->CreateTexture(desc, nullptr, &mFrameBuffer);

		if (desc.SampleCount > 1) {
			DG::TextureDesc resolveDesc;
			resolveDesc.Width = width;
			resolveDesc.Height = height;
			resolveDesc.BindFlags = DG::BIND_SHADER_RESOURCE;
			resolveDesc.Name = "Resolve Buffer";
			resolveDesc.Type = DG::RESOURCE_DIM_TEX_2D;
			resolveDesc.Usage = DG::USAGE_DEFAULT;
			resolveDesc.Format = GetIntermediateFramebufferFormat();
			resolveDesc.MipLevels = 1;

			device->CreateTexture(resolveDesc, nullptr, &mResolveBuffer);

			desc.BindFlags = DG::BIND_DEPTH_STENCIL;
			desc.Name = "Intermediate Depth Buffer";
			desc.Format = GetIntermediateDepthbufferFormat();
			desc.MipLevels = 1;
			desc.SampleCount = GetMSAASamples();

			device->CreateTexture(desc, nullptr, &mMSAADepthBuffer);
		}
	}
	
	void DefaultRenderer::OnWindowResized(uint width, uint height) {
		ReallocateIntermediateFramebuffer(width, height);
	}

	void DefaultRenderer::InitializeSystems(Scene* scene) {
		// Add the render interface to the current scene
		scene->AddSystem<DefaultRendererBridge>(this, mEngine->GetResourceManager());
	}

	DG::IBuffer* DefaultRenderer::GetGlobalsBuffer() {
		return mGlobals.Get();
	}

	void DefaultRenderer::RenderSkybox(SkyboxComponent* skybox) {
		auto context = mEngine->GetImmediateContext();
		
		auto pipeline = skybox->GetPipeline();
		context->SetPipelineState(pipeline->GetState());
		context->CommitShaderResources(skybox->GetResourceBinding(),
			RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		DG::DrawAttribs attribs;
		attribs.NumVertices = 4;
		context->Draw(attribs);
	}

	void DefaultRenderer::RenderStaticMeshes(
		entt::registry* registry,
		DefaultRendererBridge* renderBridge,
		LightProbe* globalLightProbe) {

		PipelineResource* currentPipeline = nullptr;
		MaterialResource* currentMaterial = nullptr;

		auto context = mEngine->GetImmediateContext();
		auto& meshGroup = renderBridge->GetRenderableGroup();

		auto currentIt = meshGroup->begin();
		auto endIt = meshGroup->end();

		while (currentIt != endIt) {
			// First upload transforms to GPU buffer
			void* mappedData;
			context->MapBuffer(mInstanceBuffer, DG::MAP_WRITE, DG::MAP_FLAG_DISCARD, 
				mappedData);

			DG::float4x4* ptr = reinterpret_cast<DG::float4x4*>(mappedData);
			int maxInstances = mInstanceBuffer->GetDesc().uiSizeInBytes / sizeof(DG::float4x4);
			int transformWriteIdx = 0;
			int transformReadIdx = 0;

			auto matrixCopyIt = currentIt;
			for (; matrixCopyIt != endIt && transformWriteIdx < maxInstances;
				++transformWriteIdx, ++matrixCopyIt) {
				auto& transformCache = registry->get<MatrixTransformCache>(*matrixCopyIt);
				ptr[transformWriteIdx] = transformCache.mCache.Transpose();
			}

			context->UnmapBuffer(mInstanceBuffer, DG::MAP_WRITE);

			while (currentIt != matrixCopyIt) {
				auto geometry = meshGroup->get<GeometryComponent>(*currentIt).RawPtr();
				auto material = meshGroup->get<MaterialComponent>(*currentIt).RawPtr();
				auto pipeline = material->GetPipeline();

				// Change pipeline
				if (pipeline != currentPipeline) {
					context->SetPipelineState(pipeline->GetState());
					currentPipeline = pipeline;

					auto iblView = pipeline->TryGetView<ImageBasedLightingView>();
					if (iblView && globalLightProbe) {
						iblView->SetEnvironment(globalLightProbe, 0);
					}
				}

				// Change material
				if (material != currentMaterial) {
					material->Apply(0);

					context->CommitShaderResources(pipeline->GetShaderResourceBindings()[0], 
						RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
					currentMaterial = material;
				}

				// Render all of these static meshes in a batch
				Uint32  offsets[] = { 0, (DG::Uint32)(transformReadIdx * sizeof(DG::float4x4)) };
				IBuffer* pBuffs[] = { geometry->GetVertexBuffer(), mInstanceBuffer };
				context->SetVertexBuffers(0, 2, pBuffs, offsets, 
					RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
				context->SetIndexBuffer( geometry->GetIndexBuffer(), 0, 
					RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

				// Count the number of instances of this specific mesh to render
				int instanceCount = 1;
				for (++currentIt; currentIt != matrixCopyIt
					&& meshGroup->get<GeometryComponent>(*currentIt).RawPtr() == geometry 
					&& meshGroup->get<MaterialComponent>(*currentIt).RawPtr() == material;
					++instanceCount, ++currentIt);

				DrawIndexedAttribs attribs = geometry->GetIndexedDrawAttribs();
				attribs.Flags = DG::DRAW_FLAG_VERIFY_ALL;
				attribs.NumInstances = instanceCount;
				context->DrawIndexed(attribs);

				transformReadIdx += instanceCount;
			}
		}
	}

	void DefaultRenderer::Render(Scene* scene, EntityNode cameraNode, const RenderPassTargets& targets) {
		auto context = mEngine->GetImmediateContext();
		auto swapChain = mEngine->GetSwapChain();

		auto renderBridge = scene->GetSystem<DefaultRendererBridge>();

		entt::registry* registry = nullptr;

		if (scene) {
			registry = scene->GetRegistry();

			FrameBeginEvent e;
			e.mScene = scene;
			e.mRenderer = this;
			scene->BeginFrame(e);
		}

		if (!context || !swapChain) {
			std::cout << "Warning: render context or swap chain has not been initialized!" << std::endl;
			return;
		}

		ITextureView* pRTV = targets.mColorOutputs[0];
		ITextureView* pDSV = targets.mDepthOutput;
		ITextureView* intermediateDepthView = nullptr;

		if (mMSAADepthBuffer) {
			intermediateDepthView = mMSAADepthBuffer->GetDefaultView(DG::TEXTURE_VIEW_DEPTH_STENCIL);
		} else {
			intermediateDepthView = pDSV;
		}

		float rgba[4] = {0.5, 0.5, 0.5, 1.0};

		if (scene && cameraNode.IsValid()) {
			auto rtView = mFrameBuffer->GetDefaultView(DG::TEXTURE_VIEW_RENDER_TARGET);
			context->SetRenderTargets(1, &rtView, intermediateDepthView,
				RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context->ClearRenderTarget(rtView, rgba, 
				RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context->ClearDepthStencil(intermediateDepthView, 
				CLEAR_DEPTH_FLAG, 1.f, 0, 
				RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			SkyboxComponent* skyboxComponent = nullptr;
			LightProbe* globalLightProbe = nullptr;

			auto skyboxView = registry->view<SkyboxComponent>();

			if (!skyboxView.empty()) {
				auto skyboxEntity = skyboxView.front();
				skyboxComponent = registry->try_get<SkyboxComponent>(skyboxEntity);
				globalLightProbe = registry->try_get<LightProbe>(skyboxEntity);
			}

			// Write global data to globals buffer
			WriteGlobalData(cameraNode, globalLightProbe);

			// Render all static meshes in the scene
			RenderStaticMeshes(registry, renderBridge, globalLightProbe);

			// Render skybox
			if (skyboxComponent) {
				RenderSkybox(skyboxComponent);
			}

			// Resolve MSAA before post processing
			if (mResolveBuffer != nullptr) {
				DG::ResolveTextureSubresourceAttribs resolveAttribs;
				resolveAttribs.DstTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
				resolveAttribs.SrcTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
				context->ResolveTextureSubresource(mFrameBuffer, mResolveBuffer, resolveAttribs);
			}

			// Restore default render target in case the sample has changed it
			context->SetRenderTargets(1, &pRTV, pDSV,
				RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			// Pass through post processor
			DG::ITextureView* framebufferView = nullptr;
			if (mResolveBuffer != nullptr)
				framebufferView = mResolveBuffer->GetDefaultView(DG::TEXTURE_VIEW_SHADER_RESOURCE);
			else
				framebufferView = mFrameBuffer->GetDefaultView(DG::TEXTURE_VIEW_SHADER_RESOURCE);

			PostProcessorParams ppParams;
			mPostProcessor.SetAttributes(context, ppParams);
			mPostProcessor.Draw(context, framebufferView);

		} else {

			if (!cameraNode.IsValid()) {
				std::cout << "Warning: Scene has no camera!" << std::endl;
			} else {
				std::cout << "Warning: Render cache is null!" << std::endl;
			}

			context->SetRenderTargets(1, &pRTV, pDSV, 
				RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context->ClearRenderTarget(pRTV, rgba,
				DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, 
				RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			// Restore default render target in case the sample has changed it
			context->SetRenderTargets(1, &pRTV, pDSV,
				RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		}
	}

	DG::FILTER_TYPE DefaultRenderer::GetDefaultFilter() const {
		return DG::FILTER_TYPE_LINEAR;
	}

	uint DefaultRenderer::GetMaxAnisotropy() const {
		return 16;
	}

	uint DefaultRenderer::GetMSAASamples() const {
		return 8;
	}

	uint DefaultRenderer::GetMaxRenderThreadCount() const {
		return 1;
	}

	DG::TEXTURE_FORMAT DefaultRenderer::GetBackbufferColorFormat() const {
		return mEngine->GetSwapChain()->GetDesc().ColorBufferFormat;
	}

	DG::TEXTURE_FORMAT DefaultRenderer::GetIntermediateFramebufferFormat() const {
		return INTERMEDIATE_TEXTURE_FORMAT;
	}

	DG::TEXTURE_FORMAT DefaultRenderer::GetBackbufferDepthFormat() const {
		return mEngine->GetSwapChain()->GetDesc().DepthBufferFormat;
	}

	DG::TEXTURE_FORMAT DefaultRenderer::GetIntermediateDepthbufferFormat() const {
		return mEngine->GetSwapChain()->GetDesc().DepthBufferFormat;
	}

	DG::ITextureView* DefaultRenderer::GetLUTShaderResourceView() {
		return mCookTorranceLut.GetShaderView();
	}

	bool DefaultRenderer::GetUseSHIrradiance() const {
		return bUseSHIrradiance;
	}

	bool DefaultRenderer::GetUseIBL() const {
		return true;
	}

	DG::IRenderDevice* DefaultRenderer::GetDevice() {
		return mEngine->GetDevice();
	}

	DG::IDeviceContext* DefaultRenderer::GetImmediateContext() {
		return mEngine->GetImmediateContext();
	}
}