#include <Engine/Engine.hpp>
#include <Engine/Platform.hpp>
#include <Engine/DefaultRenderer.hpp>
#include <Engine/PipelineResource.hpp>

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
		mEngine(engine) {
	}

	DefaultRenderer::~DefaultRenderer() {
		mPipeline->Release();
	}

	void DefaultRenderer::Initialize() {
		mPipeline = mEngine->GetResourceManager()->Load<PipelineResource>("pipeline.json");
	}

	void DefaultRenderer::Render() {
		auto context = mEngine->GetImmediateContext();
		auto swapChain = mEngine->GetSwapChain();

		if (!context || !swapChain)
			return;

		ITextureView* pRTV = swapChain->GetCurrentBackBufferRTV();
		ITextureView* pDSV = swapChain->GetDepthBufferDSV();
		context->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		float rgba[4] = {0.5, 0.5, 0.5, 1.0};
		context->ClearRenderTarget(pRTV, rgba, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		context->SetPipelineState(mPipeline->GetState());

		DrawAttribs attribs;
		attribs.NumVertices = 3;
		context->Draw(attribs);

		// Restore default render target in case the sample has changed it
		context->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

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
}