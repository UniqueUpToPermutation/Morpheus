#pragma once

#include <Engine/Systems/System.hpp>
#include <Engine/Platform.hpp>
#include <Engine/Graphics.hpp>
namespace Diligent
{
	class ImGuiImplDiligent;
}
namespace Morpheus {

	class ImGuiSystem : public ISystem {
	private:
		std::unique_ptr<DG::ImGuiImplDiligent> mImGui;

		RealtimeGraphics* mGraphics;
		IPlatform* mPlatform;
		bool bShow = true;

	public:
		inline DG::ImGuiImplDiligent* GetImGui() const {
			return mImGui.get();
		}

		inline ImGuiSystem(RealtimeGraphics& graphics) :
			mPlatform(graphics.GetPlatform()), mGraphics(&graphics) {
		}

		std::unique_ptr<Task> Startup(SystemCollection& systems) override;
		bool IsInitialized() const override;
		void Shutdown() override;
		void NewFrame(Frame* frame) override;
		void OnAddedTo(SystemCollection& collection) override;
	};
}