#include <Engine/Systems/EmptyRenderer.hpp>

namespace Morpheus {

	Task EmptyRenderer::Startup(SystemCollection& collection) {

		// Just clear the screen
		ParameterizedTask<RenderParams> renderTask([graphics = mGraphics](const TaskParams& e, const RenderParams& params) {
			
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

		// Give this task to the frame processor
		collection.GetFrameProcessor().AddRenderTask(std::move(renderTask));

		return Task();
	}
	
	bool EmptyRenderer::IsInitialized() const { 
		return true;
	}

	void EmptyRenderer::Shutdown() { 
	}

	void EmptyRenderer::NewFrame(Frame* frame) {
	}

	void EmptyRenderer::OnAddedTo(SystemCollection& collection) {
		collection.RegisterInterface<IRenderer>(this);
		collection.RegisterInterface<IVertexFormatProvider>(this);
	}

	const VertexLayout& EmptyRenderer::GetStaticMeshLayout() const {
		return mDefaultLayout;
	}

	MaterialId EmptyRenderer::CreateUnmanagedMaterial(const MaterialDesc& desc) {
		return entt::null;
	}

	void EmptyRenderer::AddMaterialRef(MaterialId id) {
	}
	void EmptyRenderer::ReleaseMaterial(MaterialId id) {
	}

	GraphicsCapabilityConfig EmptyRenderer::GetCapabilityConfig() const { 
		return GraphicsCapabilityConfig();
	}
}