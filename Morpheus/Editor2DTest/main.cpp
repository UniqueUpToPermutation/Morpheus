#include <Engine/Core.hpp>
#include <Engine/Im3d.hpp>
#include <Engine/Engine2D/Renderer2D.hpp>
#include <Engine/Engine2D/Editor2D.hpp>

using namespace Morpheus;

#if PLATFORM_WIN32
int __stdcall WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd) {
#endif

#if PLATFORM_LINUX
int main(int argc, char** argv) {
#endif

	EngineParams engineParams;
	engineParams.mWindow.mWindowTitle = "Morpheus 2D Editor";
	engineParams.mImGui.mExternalFonts = {
		ImFontLoad("NunitoSans-Regular.ttf", 32.0f)
	};

	Engine en;
	en.AddComponent<Renderer2D>();
	en.Startup(engineParams);

	Editor2DParams editorParams(&en);
	editorParams.mEditorFont = ImGui::GetIO().Fonts->Fonts[1];
	Editor2D editor(editorParams);

	auto scene = std::make_unique<Scene>();

	auto camera = scene->GetCamera();
	camera->SetType(CameraType::ORTHOGRAPHIC);
	camera->SetOrthoSize(2.0f, 2.0f);
	camera->SetClipPlanes(-1.0f, 1.0f);

	en.InitializeDefaultSystems(scene.get());
	scene->Begin();

	while (en.IsReady()) {
		double dt = 0.0;

		en.Update([&dt, &scene](double currTime, double elapsedTime) {
			scene->Update(currTime, elapsedTime);
			dt = elapsedTime;
		});
		
		editor.Update(&en, scene.get(), dt);

		en.Render(scene.get());
		editor.Render(&en, scene.get(), en.GetImmediateContext());
		editor.RenderUI(&en, scene.get());

		en.RenderUI();
		en.Present();
	}

	scene.reset();

	en.Shutdown();
}