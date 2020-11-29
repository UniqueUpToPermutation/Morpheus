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
#include "MapHelper.hpp"
#include "Image.h"
#include "FileWrapper.hpp"

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

	GlobalsBuffer::GlobalsBuffer(DG::IRenderDevice* renderDevice) {
        DG::BufferDesc CBDesc;
        CBDesc.Name           = "VS constants CB";
        CBDesc.uiSizeInBytes  = sizeof(DefaultRendererGlobals);
        CBDesc.Usage          = USAGE_DYNAMIC;
        CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

		mGlobalsBuffer = nullptr;
		renderDevice->CreateBuffer(CBDesc, nullptr, &mGlobalsBuffer);
	}

	GlobalsBuffer::~GlobalsBuffer() {
		mGlobalsBuffer->Release();
	}

	void GlobalsBuffer::Update(IDeviceContext* context,
		const float4x4& view,
		const float4x4& projection,
		const float3& eye,
		float time) {

		// Map the memory of the buffer and write out global transform data
		MapHelper<DefaultRendererGlobals> globals(context, mGlobalsBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
		globals->mView = view.Transpose();
		globals->mProjection = projection.Transpose();
		globals->mEye = eye;
		globals->mTime = time;
		globals->mViewProjection = (view * projection).Transpose();
		globals->mViewProjectionInverse = (view * projection).Inverse().Transpose();
	}

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
		mGlobalsBuffer(engine->GetDevice()) {

		DG::BufferDesc desc;
		desc.Name = "Renderer Instance Buffer";
		desc.Usage = DG::USAGE_DYNAMIC;
		desc.BindFlags = DG::BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = DG::CPU_ACCESS_WRITE;
		desc.uiSizeInBytes = sizeof(DG::float4x4) * instanceBatchSize;

		mEngine->GetDevice()->CreateBuffer(desc, nullptr, &mInstanceBuffer);
	}

	DefaultRenderer::~DefaultRenderer() {
		mInstanceBuffer->Release();
	}

	void DefaultRenderer::Initialize() {
		std::cout << "Precomputing Cook-Torrance BRDF..." << std::endl;

		auto context = mEngine->GetImmediateContext();

		mCookTorranceLut.Compute(mEngine->GetResourceManager(),
			context, 
			mEngine->GetDevice());

		context->SetRenderTargets(0, nullptr, nullptr,
			RESOURCE_STATE_TRANSITION_MODE_NONE);
	}

	DG::IBuffer* DefaultRenderer::GetGlobalsBuffer() {
		return mGlobalsBuffer.GetBuffer();
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
				Uint32  offsets[]  = { 0, 0 };
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

	void DefaultRenderer::Render(RenderCache* cache, EntityNode cameraNode) {
		DefaultRenderCache* cacheCast = dynamic_cast<DefaultRenderCache*>(cache);

		auto context = mEngine->GetImmediateContext();
		auto swapChain = mEngine->GetSwapChain();

		if (!context || !swapChain)
			return;

		ITextureView* pRTV = swapChain->GetCurrentBackBufferRTV();
		ITextureView* pDSV = swapChain->GetDepthBufferDSV();
		context->SetRenderTargets(1, &pRTV, pDSV, 
			RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		float rgba[4] = {0.5, 0.5, 0.5, 1.0};
		context->ClearRenderTarget(pRTV, rgba, 
			RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		context->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, 
			RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		if (cameraNode.IsValid() && cacheCast) {
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

			// Update the globals buffer with global stuff
			mGlobalsBuffer.Update(context, 
				view, projection, eye, 0.0f);

			// Render all static meshes in the scene
			RenderStaticMeshes(cacheCast->mStaticMeshes);

			// Render skybox
			if (cacheCast->mSkybox)
				RenderSkybox(cacheCast->mSkybox);
		}

		// Restore default render target in case the sample has changed it
		context->SetRenderTargets(1, &pRTV, pDSV,
			RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

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

	RenderCache* DefaultRenderer::BuildRenderCache(SceneHeirarchy* scene) {
		std::stack<Transform*> transformStack;
		transformStack.emplace(&mIdentityTransform);

		mIdentityTransform.UpdateCache();
		
		auto registry = scene->GetRegistry();

		DefaultRenderCache* cache = new DefaultRenderCache();

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
			auto& skybox = skybox_view.get<SkyboxComponent>(entity);
			if (cache->mSkybox) {
				std::cout << "Warning: multiple skyboxes detected!" << std::endl;
			}
			cache->mSkybox = &skybox;
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