#include <Engine/Engine.hpp>
#include <Engine/Platform.hpp>
#include <Engine/DefaultRenderer.hpp>
#include <Engine/PipelineResource.hpp>
#include <Engine/GeometryResource.hpp>
#include <Engine/TextureResource.hpp>

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
		const float4x4& world,
		const float4x4& view,
		const float4x4& projection,
		const float3& eye,
		float time) {

		// Map the memory of the buffer and write out global transform data
		MapHelper<DefaultRendererGlobals> globals(context, mGlobalsBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
		globals->mWorld = world.Transpose();
		globals->mView = view.Transpose();
		globals->mProjection = projection.Transpose();
		globals->mEye = eye;
		globals->mTime = time;
		globals->mWorldViewProjection = (world * view * projection).Transpose();
		globals->mWorldView = (world * view).Transpose();
		globals->mWorldViewProjectionInverse = (globals->mWorldViewProjection.Inverse()).Transpose();
		globals->mViewProjectionInverse = (view * projection).Inverse().Transpose();
		globals->mWorldInverseTranspose = world.Inverse();
		globals->mWorldViewInverseTranspose = globals->mWorldView.Inverse();
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

	DefaultRenderer::DefaultRenderer(Engine* engine) :
		mEngine(engine), mGlobalsBuffer(engine->GetDevice()) {
	}

	DefaultRenderer::~DefaultRenderer() {
		mGeometry->Release();
		mPipeline->Release();
		mTexture->Release();
		mSRB->Release();
	}

	void DefaultRenderer::Initialize() {
		mPipeline = mEngine->GetResourceManager()->Load<PipelineResource>("mesh_pipeline_textured.json");

		LoadParams<GeometryResource> resourceParams;
		resourceParams.mPipelineResource = mPipeline;
		resourceParams.mSource = "test.obj";
		mGeometry = mEngine->GetResourceManager()->Load<GeometryResource>(resourceParams);

		mTexture = mEngine->GetResourceManager()->Load<TextureResource>("brick.jpg");

		mPipeline->GetState()->CreateShaderResourceBinding(&mSRB, true);
		mSRB->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mTexture")->Set(mTexture->GetShaderView());
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

		mCamera.mEye = DG::float3(0.0f, 4.0f, -4.0f);
		mCamera.mLookAt = DG::float3(0.0f, 0.0f, 0.0f);

		float4x4 world = float4x4::RotationY(rot);
		float4x4 view = mCamera.GetView();
		float4x4 projection = mCamera.GetProjection(mEngine);

		// Update the globals buffer with global stuff
		mGlobalsBuffer.Update(context, 
			world, view, projection,
			mCamera.GetEye(), 0.0f);

		context->SetPipelineState(mPipeline->GetState());
		context->CommitShaderResources(mSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		Uint32   offset   = 0;
		IBuffer* pBuffs[] = { mGeometry->GetVertexBuffer() };
		context->SetVertexBuffers(0, 1, pBuffs, &offset, 
			RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
		context->SetIndexBuffer( mGeometry->GetIndexBuffer(), 0, 
			RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		DrawIndexedAttribs attribs = mGeometry->GetIndexedDrawAttribs();
		attribs.Flags = DG::DRAW_FLAG_VERIFY_ALL;
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