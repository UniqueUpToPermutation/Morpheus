#include <Engine/Loading.hpp>
#include <Engine/ThreadPool.hpp>
#include <Engine/Graphics.hpp>
#include <Engine/Systems/System.hpp>
#include <Engine/Frame.hpp>

#include "Timer.hpp"

#include "BasicMath.hpp"

#include "Timer.hpp"

#include "imgui.h"

namespace DG = Diligent;

namespace Morpheus {
	ImVec2 Convert(const DG::float2& p) {
		return ImVec2(p.x, p.y);
	}

	void BasicLoadingScreen(Platform& platform, RealtimeGraphics& graphics,
		DG::ImGuiImplDiligent* imgui, BarrierOut barrier, IComputeQueue* queue) {
		
		float radius = 64.0f;
		int divisions = 40;
		DG::float2 position(120.0f, 120.0f);
		uint color = 0xFFFFFFFF;
		float thickness = 10.0f;

		DG::Timer timer;

		double time = 0.0;

		while (platform->IsValid() && !barrier.IsFinished()) {

			// Do loading stuff
			platform->MessagePump();
			queue->YieldFor(std::chrono::milliseconds(15));

			// Clear the screen
			auto context = graphics.ImmediateContext();
			auto swapChain = graphics.SwapChain();
			auto rtv = swapChain->GetCurrentBackBufferRTV();
			auto dsv = swapChain->GetDepthBufferDSV();
			float clearColor[] = { 0.8f, 0.8f, 0.8f, 1.0f };
			context->SetRenderTargets(1, &rtv, dsv, 
				DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context->ClearRenderTarget(rtv, clearColor, 
				DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context->ClearDepthStencil(dsv, DG::CLEAR_DEPTH_FLAG, 1.0f, 0, 
				DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			// Render spinning wheel
			auto& scDesc = graphics.SwapChain()->GetDesc();
			imgui->NewFrame(scDesc.Width, scDesc.Height, scDesc.PreTransform);

			auto bkg = ImGui::GetBackgroundDrawList();
			auto& desc = graphics.SwapChain()->GetDesc();

			bkg->PathClear();

			double time = timer.GetElapsedTime();

			int num_segments = 30;
		
			const float a_min = M_PI*2.0f * time;
			const float a_max = a_min + 1.5;
			const ImVec2 centre = ImVec2(desc.Width / 2, desc.Height / 2);
		
			for (int i = 0; i < num_segments; i++) {
				const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
				bkg->PathLineTo(ImVec2(centre.x + cos(a) * radius,
													centre.y + sin(a) * radius));
			}
			bkg->PathStroke(color, false, thickness);

			imgui->Render(graphics.ImmediateContext());

			graphics.Present(1);
		}
	}	
}