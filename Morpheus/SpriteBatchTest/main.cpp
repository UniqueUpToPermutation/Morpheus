#include <Engine/Core.hpp>
#include <Engine/SpriteBatch.hpp>

#include <random>

using namespace Morpheus;

MAIN() {
	// Create windowing stuff
	Platform platform;
	platform.Startup();

	// Create graphics device and swap chain
	RealtimeGraphics graphics(platform);
	graphics.Startup();

	// For embedded shaders and stuff
	EmbeddedFileLoader embeddedFileLoader;

	{
		// Create sprite batch globals to forward camera info to GPU
		SpriteBatchGlobals sbGlobals(graphics);

		auto& scDesc = graphics.SwapChain()->GetDesc();	

		// Actually load everything
		auto sbPipelineLoadTask = SpriteBatchPipeline::LoadDefault(graphics, &sbGlobals, 
			DG::FILTER_TYPE_LINEAR, &embeddedFileLoader);
		auto textureLoadTask = Texture::Load(graphics.Device(), "sprite.png");

		ImmediateTaskQueue queue;
		auto sbPipelineFuture = queue.AdoptAndTrigger(std::move(sbPipelineLoadTask));
		auto textureFuture = queue.AdoptAndTrigger(std::move(textureLoadTask));
		queue.YieldUntilEmpty();

		assert(sbPipelineFuture.IsAvailable());
		assert(textureFuture.IsAvailable());

		auto sbPipeline = sbPipelineFuture.Get();
		auto spriteTexture = textureFuture.Get();

		// Actually create the sprite batch!
		SpriteBatch spriteBatch(graphics, sbPipeline);

		// Setup camera
		Camera camera;
		camera.SetType(CameraType::ORTHOGRAPHIC);
		camera.SetClipPlanes(-1.0, 1.0);

		// Create all the objects on screen
		struct ObjInstance {
			DG::float2 mPositionBase;
			float mRotation;
			DG::float4 mColor;
			float mAngularVelocity;
			DG::float2 mOscillatorVector;
			float mOscillatorVelocity;
			float mOscillatorX;
		};

		std::default_random_engine generator;
		std::uniform_real_distribution<double> distribution1(-1.0, 1.0);
		std::uniform_real_distribution<double> distribution2(0.0, 1.0);

		constexpr uint obj_count = 350;
		std::vector<ObjInstance> insts(obj_count);
		for (auto& obj : insts) {
			obj.mPositionBase.x = distribution1(generator) * 400;
			obj.mPositionBase.y = distribution1(generator) * 300;
			obj.mRotation = distribution1(generator) * DG::PI;
			obj.mColor = DG::float4(distribution2(generator), distribution2(generator), distribution2(generator), 1.0);
			obj.mAngularVelocity = distribution1(generator) * 0.01;
			obj.mOscillatorVector.x = distribution1(generator) * 50.0;
			obj.mOscillatorVector.y = distribution1(generator) * 50.0;
			obj.mOscillatorVelocity = distribution1(generator) * 0.01;
			obj.mOscillatorX = distribution1(generator) * DG::PI;
		}

		while (platform.IsValid()) {
			// Perform window IO
			platform.MessagePump();

			// Clear the screen
			auto context = graphics.ImmediateContext();
			auto swapChain = graphics.SwapChain();
			auto rtv = swapChain->GetCurrentBackBufferRTV();
			auto dsv = swapChain->GetDepthBufferDSV();
			float color[] = { 0.8f, 0.8f, 0.8f, 1.0f };
			context->SetRenderTargets(1, &rtv, dsv, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context->ClearRenderTarget(rtv, color, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			context->ClearDepthStencil(dsv, DG::CLEAR_DEPTH_FLAG, 1.0f, 0, 
				DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			// Make sure that the camera width/height match window width/height
			auto& scDesc = swapChain->GetDesc();
			camera.SetOrthoSize(scDesc.Width, scDesc.Height);

			// Send camera information to the GPU
			auto cameraAttribs = camera.GetLocalAttribs(graphics);
			sbGlobals.Write(graphics.ImmediateContext(), cameraAttribs);

			for (auto& obj : insts) {
				obj.mOscillatorX += obj.mOscillatorVelocity;
				obj.mRotation += obj.mAngularVelocity;
			}

			// Draw all sprites
			spriteBatch.Begin(graphics.ImmediateContext());
			for (auto& obj : insts) {
				spriteBatch.Draw(spriteTexture, obj.mPositionBase + std::cos(obj.mOscillatorX) * obj.mOscillatorVector,
					DG::float2(128, 128), DG::float2(64, 64), obj.mRotation, obj.mColor);
			}
			spriteBatch.End();

			// Swap front and back buffer
			graphics.Present(1);
		}

		spriteTexture->Release();
	}

	graphics.Shutdown();
	platform.Shutdown();
}