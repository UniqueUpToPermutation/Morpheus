#include <Engine/Engine.hpp>

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

	Engine en;
	
	en.Startup();

	while (en.IsReady()) {
		en.Update([](double, double) { });
		en.Render(nullptr);
		en.RenderUI();
		en.Present();
	}

	en.Shutdown();
}