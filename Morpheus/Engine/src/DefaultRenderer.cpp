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
		mStaticMesh->Release();
		mInstanceBuffer->Release();
	}

	void DefaultRenderer::Initialize() {
		mStaticMesh = mEngine->GetResourceManager()->Load<StaticMeshResource>("static_mesh.json");
	}

	DG::IBuffer* DefaultRenderer::GetGlobalsBuffer() {
		return mGlobalsBuffer.GetBuffer();
	}

	float rot = 0.0f;

	void DefaultRenderer::Render() {
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

		rot += 0.02f;

		mCamera.mEye = DG::float3(0.0f, 16.0f, -16.0f);
		mCamera.mLookAt = DG::float3(0.0f, 0.0f, 0.0f);

		
		float4x4 view = mCamera.GetView();
		float4x4 projection = mCamera.GetProjection(mEngine);

		// Update the globals buffer with global stuff
		mGlobalsBuffer.Update(context, 
			view, projection,
			mCamera.GetEye(), 0.0f);

		auto material = mStaticMesh->GetMaterial();
		auto pipeline = material->GetPipeline();
		auto geometry = mStaticMesh->GetGeometry();

		context->SetPipelineState(pipeline->GetState());
		context->CommitShaderResources(material->GetResourceBinding(), 
			RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		std::vector<DG::float4x4> instances;
		for (int x = -5; x <= 5; ++x) {
			for (int y = -5; y <= 5; ++y) {
				float4x4 rot_mat = float4x4::RotationY(rot);
				float4x4 trans_mat = float4x4::Translation(DG::float3(3.0f * x, 0.0f, 3.0f * y));

				instances.emplace_back(rot_mat * trans_mat);
			}
		}

		void* mappedData;
		context->MapBuffer(mInstanceBuffer, 
			DG::MAP_WRITE, DG::MAP_FLAG_DISCARD, 
			mappedData);

		memcpy(mappedData, instances.data(), sizeof(DG::float4x4) * instances.size());

		context->UnmapBuffer(mInstanceBuffer, DG::MAP_WRITE);

		Uint32   offsets[]  = { 0, 0 };
		IBuffer* pBuffs[] = { geometry->GetVertexBuffer(), mInstanceBuffer };
		context->SetVertexBuffers(0, 2, pBuffs, offsets, 
			RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
		context->SetIndexBuffer( geometry->GetIndexBuffer(), 0, 
			RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		DrawIndexedAttribs attribs = geometry->GetIndexedDrawAttribs();
		attribs.Flags = DG::DRAW_FLAG_VERIFY_ALL;
		attribs.NumInstances = instances.size();
		context->DrawIndexed(attribs);

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
}