#include <Engine/Platform.hpp>
#include <Engine/Graphics.hpp>
#include <Engine/Entity.hpp>
#include <Engine/Frame.hpp>
#include <Engine/Systems/EmptyRenderer.hpp>

#include "Timer.hpp"

using namespace Morpheus;

MAIN() {
	ThreadPool pool;
	pool.Startup();

	Platform platform;
	platform.Startup();

	RealtimeGraphics graphics(platform);
	graphics.Startup();

	SystemCollection systems;
	systems.Add<EmptyRenderer>(graphics);
	systems.Startup(&pool);

	auto frame = std::make_unique<Frame>();
	systems.SetFrame(frame.get());

	DG::Timer timer;
	FrameTime time(timer);

	while (platform.IsValid()) {
		time.UpdateFrom(timer);
		platform.MessagePump();

		systems.RunFrame(time, &pool);
		systems.WaitOnRender(&pool);
		graphics.Present(1);
		systems.WaitOnUpdate(&pool);
	}

	frame.reset();
	systems.Shutdown();
	graphics.Shutdown();
	platform->Shutdown();
}