#include <Engine/Systems/EmptyRenderer.hpp>

namespace Morpheus {

	std::unique_ptr<Task> EmptyRenderer::Startup(SystemCollection& collection) {

		auto& frameProcessor = collection.GetFrameProcessor();

		// Just clear the screen
		render_proto_t renderPrototype([graphics = mGraphics]
			(const TaskParams& e, Future<RenderParams> f_params) {
			auto context = graphics->ImmediateContext();
			auto swapChain = graphics->SwapChain();
			auto rtv = swapChain->GetCurrentBackBufferRTV();
			auto dsv = swapChain->GetDepthBufferDSV();
			float color[] = { 0.5f, 0.5f, 1.0f, 1.0f };
			context->SetRenderTargets(1, &rtv, dsv, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context->ClearRenderTarget(rtv, color, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context->ClearDepthStencil(dsv, DG::CLEAR_DEPTH_FLAG, 1.0f, 0, 
				DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		});

		// Give this task to the frame processor
		frameProcessor.AddRenderTask(
			renderPrototype(frameProcessor.GetRenderInput())
			.SetName("Render")
			.OnlyThread(THREAD_MAIN));

		return nullptr;
	}
	
	bool EmptyRenderer::IsInitialized() const { 
		return true;
	}

	void EmptyRenderer::Shutdown() { 
	}

	void EmptyRenderer::NewFrame(Frame* frame) {
	}

	std::unique_ptr<Task> EmptyRenderer::LoadResources(Frame* frame) {
		return nullptr;
	}

	void EmptyRenderer::OnAddedTo(SystemCollection& collection) {
		collection.RegisterInterface<IVertexFormatProvider>(this);
	}

	const VertexLayout& EmptyRenderer::GetStaticMeshLayout() const {
		return mDefaultLayout;
	}

	GraphicsCapabilityConfig EmptyRenderer::GetCapabilityConfig() const { 
		return GraphicsCapabilityConfig();
	}
}