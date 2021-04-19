#include <Engine/EmptyRenderer.hpp>

namespace Morpheus {
	void EmptyRenderer::RequestConfiguration(DG::EngineD3D11CreateInfo* info) {
	}
	void EmptyRenderer::RequestConfiguration(DG::EngineD3D12CreateInfo* info) {
	}
	void EmptyRenderer::RequestConfiguration(DG::EngineGLCreateInfo* info) {
	}
	void EmptyRenderer::RequestConfiguration(DG::EngineVkCreateInfo* info) {
	}
	void EmptyRenderer::RequestConfiguration(DG::EngineMtlCreateInfo* info) {
	}

	void EmptyRenderer::Initialize(Engine* engine) {
		mEngine = engine;
		mGlobals.Initialize(engine->GetDevice());

		std::cout << "NOTE: You are using EmptyRenderer! Nothing will be renderered!" << std::endl;
	}
	void EmptyRenderer::InitializeSystems(Scene* scene) {
	}

	void EmptyRenderer::Render(Scene* scene, EntityNode cameraNode, const RenderPassTargets& targets) {
		float rgba[4] = {0.5, 0.5, 0.5, 1.0};

		auto context = mEngine->GetImmediateContext();

		DG::ITextureView* pRTV = targets.mColorOutputs[0];

		context->SetRenderTargets(1, &pRTV, targets.mDepthOutput,
			DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		context->ClearRenderTarget(pRTV, rgba, 
			DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		context->ClearDepthStencil(targets.mDepthOutput, 
			DG::CLEAR_DEPTH_FLAG, 1.f, 0, 
			DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	}

	DG::IRenderDevice* EmptyRenderer::GetDevice() {
		return mEngine->GetDevice();
	}
	DG::IDeviceContext* EmptyRenderer::GetImmediateContext() {
		return mEngine->GetImmediateContext();
	}

	// This buffer will be bound as a constant to all pipelines
	DG::IBuffer* EmptyRenderer::GetGlobalsBuffer() {
		return mGlobals.Get();
	}
	DG::FILTER_TYPE EmptyRenderer::GetDefaultFilter() const {
		return DG::FILTER_TYPE_POINT;
	}
	uint EmptyRenderer::GetMaxAnisotropy() const {
		return 1;
	}
	uint EmptyRenderer::GetMSAASamples() const {
		return 1;
	}
	uint EmptyRenderer::GetMaxRenderThreadCount() const {
		return 1;
	}

	void EmptyRenderer::OnWindowResized(uint width, uint height) {
	}

	DG::TEXTURE_FORMAT EmptyRenderer::GetBackbufferColorFormat() const {
		return mEngine->GetSwapChain()->GetDesc().ColorBufferFormat;
	}
	DG::TEXTURE_FORMAT EmptyRenderer::GetBackbufferDepthFormat() const {
		return mEngine->GetSwapChain()->GetDesc().DepthBufferFormat;
	}

	DG::TEXTURE_FORMAT EmptyRenderer::GetIntermediateFramebufferFormat() const {
		return GetBackbufferColorFormat();
	}
	DG::TEXTURE_FORMAT EmptyRenderer::GetIntermediateDepthbufferFormat() const {
		return GetBackbufferDepthFormat();
	}

	DG::ITextureView* EmptyRenderer::GetLUTShaderResourceView() {
		return nullptr;
	}
	bool EmptyRenderer::GetUseSHIrradiance() const {
		return false;
	}
	bool EmptyRenderer::GetUseIBL() const {
		return false;
	}
}