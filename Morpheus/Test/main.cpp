#include <Engine/Engine.hpp>
#include <Engine/PipelineResource.hpp>

using namespace Morpheus;

int main(int argc, char** argv) {
	Engine en;

	en.Startup(argc, argv);
	
	while (en.IsReady()) {
		en.Update();
		en.Render();
		en.Present();
	}

	en.Shutdown();
}