#include <Engine/Engine.hpp>
#include <Engine/Platform.hpp>
#include <Engine/DefaultRenderer.hpp>
#include <Engine/PipelineResource.hpp>
#include <Engine/GeometryResource.hpp>
#include <Engine/TextureResource.hpp>
#include <Engine/MaterialResource.hpp>
#include <Engine/StaticMeshResource.hpp>

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

	DefaultRenderer::DefaultRenderer(Engine* engine,
		uint instanceBatchSize) :
		mCookTorranceLut(engine->GetDevice()),
		mInstanceBuffer(nullptr),
		mEngine(engine), 
		mPostProcessor(engine->GetDevice()),
		mFrameBuffer(nullptr),
		mGlobals(engine->GetDevice()) {

		auto device = mEngine->GetDevice();
		auto context = mEngine->GetImmediateContext();

		DG::BufferDesc desc;
		desc.Name = "Renderer Instance Buffer";
		desc.Usage = DG::USAGE_DYNAMIC;
		desc.BindFlags = DG::BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = DG::CPU_ACCESS_WRITE;
		desc.uiSizeInBytes = sizeof(DG::float4x4) * instanceBatchSize;

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
		mWhiteTexture = textureCache->MakeResource(whiteTex, "WHITE_TEXTURE");
		mBlackTexture = textureCache->MakeResource(blackTex, "BLACK_TEXTURE");
		mDefaultNormalTexture = textureCache->MakeResource(defaultNormalTex, "DEFAULT_NORMAL_TEXTURE");
	}

	DefaultRenderer::~DefaultRenderer() {
		mInstanceBuffer->Release();

		if (mFrameBuffer) {
			mFrameBuffer->Release();
			mFrameBuffer = nullptr;
		}

		mDefaultSampler->Release();
		mWhiteTexture->Release();
		mBlackTexture->Release();
		mDefaultNormalTexture->Release();
	}

	void DefaultRenderer::WriteGlobalData(EntityNode cameraNode) {

		auto camera = cameraNode.GetComponent<Camera>();
		auto camera_transform = cameraNode.TryGetComponent<Transform>();

		float4x4 view = camera->GetView();
		float4x4 projection = camera->GetProjection(mEngine);
		float3 eye = camera->GetEye();

		if (camera_transform) {
			auto camera_transform_mat = camera_transform->GetCache();
			auto camera_transform_inv = camera_transform_mat.Inverse();
			view = camera_transform_inv * view;
			eye = eye * camera_transform_mat;
		}

		auto context = mEngine->GetImmediateContext();
		auto swapChain = mEngine->GetSwapChain();
		auto swapChainDesc = swapChain->GetDesc();

		RendererGlobalData data;
		data.mCamera.fNearPlaneZ = camera->GetNearZ();
		data.mCamera.fFarPlaneZ = camera->GetFarZ();
		data.mCamera.f4Position = DG::float4(eye, 1.0f);
		data.mCamera.f4ViewportSize = DG::float4(
			swapChainDesc.Width,
			swapChainDesc.Height,
			1.0f / (float)swapChainDesc.Width,
			1.0f / (float)swapChainDesc.Height);
		data.mCamera.mViewT = view.Transpose();
		data.mCamera.mProjT = projection.Transpose();
		data.mCamera.mViewProjT = (view * projection).Transpose();
		data.mCamera.mProjInvT = data.mCamera.mProjT.Inverse();
		data.mCamera.mViewInvT = data.mCamera.mViewT.Inverse();
		data.mCamera.mViewProjInvT = data.mCamera.mViewProjT.Inverse();

		mGlobals.Write(context, data);
	}

	void DefaultRenderer::ReallocateIntermediateFramebuffer(
		uint width, uint height) {

		if (mFrameBuffer) {
			mFrameBuffer->Release();
			mFrameBuffer = nullptr;
		}

		DG::TextureDesc desc;
		desc.Width = width;
		desc.Height = height;
		desc.BindFlags = DG::BIND_RENDER_TARGET | DG::BIND_SHADER_RESOURCE;
		desc.Name = "Intermediate Framebuffer";
		desc.Type = DG::RESOURCE_DIM_TEX_2D;
		desc.Usage = DG::USAGE_DEFAULT;
		desc.Format = GetIntermediateFramebufferFormat();
		desc.MipLevels = 1;
		
		auto device = mEngine->GetDevice();
		device->CreateTexture(desc, nullptr, &mFrameBuffer);
	}
	
	void DefaultRenderer::OnWindowResized(uint width, uint height) {
		ReallocateIntermediateFramebuffer(width, height);
	}

	void DefaultRenderer::Initialize() {
		std::cout << "Precomputing Cook-Torrance BRDF..." << std::endl;

		auto context = mEngine->GetImmediateContext();

		mCookTorranceLut.Compute(mEngine->GetResourceManager(),
			context, 
			mEngine->GetDevice());

		context->SetRenderTargets(0, nullptr, nullptr,
			RESOURCE_STATE_TRANSITION_MODE_NONE);

		auto swapDesc = mEngine->GetSwapChain()->GetDesc();
		ReallocateIntermediateFramebuffer(swapDesc.Width, swapDesc.Height);

		mPostProcessor.Initialize(mEngine->GetResourceManager(),
			swapDesc.ColorBufferFormat,
			swapDesc.DepthBufferFormat);
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

	void DefaultRenderer::RenderStaticMeshes(std::vector<StaticMeshCache>& cache) {
		auto pipelineComp = [](const StaticMeshCache& c1, const StaticMeshCache& c2) {
			return c1.mStaticMesh->GetPipeline() < c2.mStaticMesh->GetPipeline();
		};

		auto materialComp = [](const StaticMeshCache& c1, const StaticMeshCache& c2) {
			return c1.mStaticMesh->GetMaterial() < c2.mStaticMesh->GetMaterial();
		};

		auto meshComp = [](const StaticMeshCache& c1, const StaticMeshCache& c2) {
			return c1.mStaticMesh < c2.mStaticMesh;
		};

		std::sort(cache.begin(), cache.end(), pipelineComp);
		std::stable_sort(cache.begin(), cache.end(), materialComp);
		std::stable_sort(cache.begin(), cache.end(), meshComp);

		PipelineResource* currentPipeline = nullptr;
		MaterialResource* currentMaterial = nullptr;
		StaticMeshResource* currentMesh = nullptr;

		int currentIdx = 0;

		auto context = mEngine->GetImmediateContext();

		int meshCount = cache.size();

		while (currentIdx < meshCount) {
			// First upload transforms to GPU buffer
			void* mappedData;
			context->MapBuffer(mInstanceBuffer, DG::MAP_WRITE, DG::MAP_FLAG_DISCARD, 
				mappedData);

			DG::float4x4* ptr = reinterpret_cast<DG::float4x4*>(mappedData);
			int maxInstances = mInstanceBuffer->GetDesc().uiSizeInBytes / sizeof(DG::float4x4);
			int currentInstanceIdx = 0;

			for (; currentIdx + currentInstanceIdx < meshCount && currentInstanceIdx < maxInstances;
				++currentInstanceIdx) {
				ptr[currentInstanceIdx] = cache[currentIdx + currentInstanceIdx].mTransform->GetCache();
			}

			context->UnmapBuffer(mInstanceBuffer, DG::MAP_WRITE);

			int instanceBatchTotal = currentInstanceIdx;
			for (currentInstanceIdx = 0; currentInstanceIdx < instanceBatchTotal;) {
				auto mesh = cache[currentIdx + currentInstanceIdx].mStaticMesh->GetMesh();
				auto material = mesh->GetMaterial();
				auto pipeline = material->GetPipeline();

				// Change pipeline
				if (pipeline != currentPipeline) {
					context->SetPipelineState(pipeline->GetState());
					currentPipeline = pipeline;
				}

				// Change material
				if (material != currentMaterial) {
					context->CommitShaderResources(material->GetResourceBinding(), 
						RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
					currentMaterial = material;
				}

				// Render all of these static meshes in a batch
				auto geometry = mesh->GetGeometry();
				Uint32  offsets[]  = { 0, (DG::Uint32)currentInstanceIdx };
				IBuffer* pBuffs[] = { geometry->GetVertexBuffer(), mInstanceBuffer };
				context->SetVertexBuffers(0, 2, pBuffs, offsets, 
					RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
				context->SetIndexBuffer( geometry->GetIndexBuffer(), 0, 
					RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

				// Count the number of instances of this specific mesh to render
				int instanceCount = 1;
				for (; currentInstanceIdx + instanceCount < instanceBatchTotal 
					&& cache[currentIdx + currentInstanceIdx + instanceCount].mStaticMesh->GetMesh() == mesh;
					++instanceCount);

				DrawIndexedAttribs attribs = geometry->GetIndexedDrawAttribs();
				attribs.Flags = DG::DRAW_FLAG_VERIFY_ALL;
				attribs.NumInstances = instanceCount;
				context->DrawIndexed(attribs);

				currentInstanceIdx += instanceCount;
			}

			currentIdx += instanceBatchTotal;
		}
	}

	void DefaultRenderer::Render(IRenderCache* cache, EntityNode cameraNode) {
		DefaultRenderCache* cacheCast = dynamic_cast<DefaultRenderCache*>(cache);

		auto context = mEngine->GetImmediateContext();
		auto swapChain = mEngine->GetSwapChain();
		auto registry = cacheCast->mScene->GetRegistry();

		if (!context || !swapChain)
			return;

		ITextureView* pRTV = swapChain->GetCurrentBackBufferRTV();
		ITextureView* pDSV = swapChain->GetDepthBufferDSV();

		float rgba[4] = {0.5, 0.5, 0.5, 1.0};

		if (cameraNode.IsValid() && cacheCast) {
			auto rtView = mFrameBuffer->GetDefaultView(DG::TEXTURE_VIEW_RENDER_TARGET);
			context->SetRenderTargets(1, &rtView, pDSV,
				RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context->ClearRenderTarget(rtView, rgba, 
				RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, 
				RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			// Write global data to globals buffer
			WriteGlobalData(cameraNode);

			// Render all static meshes in the scene
			RenderStaticMeshes(cacheCast->mStaticMeshes);

			// Render skybox
			if (cacheCast->mSkybox != entt::null) {
				auto& skyboxComponent = registry->get<SkyboxComponent>(cacheCast->mSkybox);
				RenderSkybox(&skyboxComponent);
			}
		} else {
			context->SetRenderTargets(1, &pRTV, pDSV, 
				RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, 
				RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		}

		// Restore default render target in case the sample has changed it
		context->SetRenderTargets(1, &pRTV, pDSV,
			RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		// Pass through post processor
		auto srView = mFrameBuffer->GetDefaultView(DG::TEXTURE_VIEW_SHADER_RESOURCE);
		PostProcessorParams ppParams;
		mPostProcessor.SetAttributes(context, ppParams);
		mPostProcessor.Draw(context, srView);

		auto imGui = mEngine->GetUI();

		if (imGui)
		{
			if (mEngine->GetShowUI())
			{
				// No need to call EndFrame as ImGui::Render calls it automatically
				imGui->Render(context);
			}
			else
			{
				imGui->EndFrame();
			}
		}
	}

	DG::FILTER_TYPE DefaultRenderer::GetDefaultFilter() {
		return DG::FILTER_TYPE_LINEAR;
	}

	uint DefaultRenderer::GetMaxAnisotropy() {
		return 0;
	}

	DG::TEXTURE_FORMAT DefaultRenderer::GetIntermediateFramebufferFormat() const {
		return INTERMEDIATE_TEXTURE_FORMAT;
	}

	DG::TEXTURE_FORMAT DefaultRenderer::GetIntermediateDepthbufferFormat() const {
		return mEngine->GetSwapChain()->GetDesc().DepthBufferFormat;
	}

	DG::ITextureView* DefaultRenderer::GetLUTShaderResourceView() {
		return mCookTorranceLut.GetShaderView();
	}

	IRenderCache* DefaultRenderer::BuildRenderCache(SceneHeirarchy* scene) {
		std::stack<Transform*> transformStack;
		transformStack.emplace(&mIdentityTransform);

		mIdentityTransform.UpdateCache();
		
		auto registry = scene->GetRegistry();
		auto device = mEngine->GetDevice();
		auto resourceManager = mEngine->GetResourceManager();
		auto immediateContext = mEngine->GetImmediateContext();
		auto textureCache = resourceManager->GetCache<TextureResource>();

		DefaultRenderCache* cache = new DefaultRenderCache(scene);

		for (auto it = scene->GetDoubleIterator(); it; ++it) {
			if (it.GetDirection() == IteratorDirection::DOWN) {
				auto transform = it->TryGetComponent<Transform>();
				auto staticMesh = it->TryGetComponent<StaticMeshComponent>();
				auto skybox = it->TryGetComponent<SkyboxComponent>();

				// Node has a transform; update cache
				if (transform) {
					transform->UpdateCache(transformStack.top());
					transformStack.emplace(transform);
				}

				// If a static mesh is on this node
				if (staticMesh) {
					StaticMeshCache meshCache;
					meshCache.mStaticMesh = staticMesh;
					meshCache.mTransform = transformStack.top();
					cache->mStaticMeshes.emplace_back(meshCache);
				}
			}
			else if (it.GetDirection() == IteratorDirection::UP) {
				auto transform = it->TryGetComponent<Transform>();

				// Pop the transform of this node on the way up
				if (transform) {
					transformStack.pop();
				}
			}
		}

		auto skybox_view = registry->view<SkyboxComponent>();

		for (auto entity : skybox_view) {
			if (cache->mSkybox != entt::null) {
				std::cout << "Warning: multiple skyboxes detected!" << std::endl;
			}
			cache->mSkybox = entity;

			auto& skybox = skybox_view.get<SkyboxComponent>(entity);

			// Build light probe if this skybox has none
			auto lightProbe = registry->try_get<LightProbe>(entity);
			if (!lightProbe) {

				std::cout << "Warning: skybox detected without light probe! Running light probe processor..." << std::endl;

				LightProbeProcessor processor(device);
				processor.Initialize(resourceManager, 
					DG::TEX_FORMAT_RGBA16_FLOAT,
					DG::TEX_FORMAT_RGBA16_FLOAT);

				LightProbe newProbe = processor.ComputeLightProbe(device,
					immediateContext, textureCache, 
					skybox.GetCubemap()->GetShaderView());

				// Add light probe to skybox
				registry->emplace<LightProbe>(entity, newProbe);
			}
		}

		auto camera_view = registry->view<Camera>();

		for (auto entity : camera_view) {
			if (!registry->has<Transform>(entity)) {
				std::cout << "Warning! Camera detected without Transform!" << std::endl;
			}
		}

		return cache;
	}
}