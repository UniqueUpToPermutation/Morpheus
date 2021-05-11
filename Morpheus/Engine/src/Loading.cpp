#include <Engine/Loading.hpp>
#include <Engine/Engine.hpp>

#include "imgui.h"

namespace Morpheus {
	void LoadingScreen(Engine* en, const std::vector<IResource*>& resources) {
		auto currentResource = resources.begin();

		while (en->IsReady() && currentResource != resources.end()) {

			en->YieldFor(std::chrono::milliseconds(10));

			if ((*currentResource)->GetLoadBarrier()->mOut.Lock().IsFinished()) {
				++currentResource;
			}

			en->Update(nullptr);
			en->Render(nullptr);

			auto bkg = ImGui::GetBackgroundDrawList();

			bkg->AddText(ImVec2(16.0f, 16.0f), 0xFFFFFFFF, "Loading...");
			
			en->RenderUI();
			en->Present();
		}
	}	
}