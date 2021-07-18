#include <Engine/Platform.hpp>
#include <Engine/Graphics.hpp>
#include <Engine/Entity.hpp>
#include <Engine/Systems/DefaultRenderer.hpp>

#include "Timer.hpp"

using namespace Morpheus;

MAIN() {
	Platform platform;
	platform.Startup();

	RealtimeGraphics graphics(platform);
	graphics.Startup();

	SystemCollection systems;
	systems.Add<DefaultRenderer>(graphics);
	systems.Startup();

	auto frame = std::make_unique<Frame>();
	systems.SetFrame(frame.get());

	DG::Timer timer;
	FrameTime time(timer);

	ImmediateTaskQueue queue;

	while (platform.IsValid()) {
		time.UpdateFrom(timer);
		platform.MessagePump();

		systems.RunFrame(time, &queue);
		systems.WaitOnRender(&queue);
		graphics.Present(1);
		systems.WaitOnUpdate(&queue);
	}

	frame.reset();
	systems.Shutdown();
	graphics.Shutdown();
	platform->Shutdown();
}