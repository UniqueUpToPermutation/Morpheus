#include <Engine/Core.hpp>
#include <Engine/Engine2D/Renderer2D.hpp>

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

	EngineParams params;

	Engine en;

	en.AddComponent<Renderer2D>();
	en.Startup(params);

	Scene* scene = new Scene();
	auto camera = scene->GetCamera();
	camera->SetType(CameraType::ORTHOGRAPHIC);
	camera->SetClipPlanes(-1.0, 1.0);

	TextureResource* spriteTexture = en.GetResourceManager()->Load<TextureResource>("sprite.png");
	auto spriteBatch = std::make_unique<SpriteBatch>(en.GetDevice(), en.GetResourceManager());

	en.InitializeDefaultSystems(scene);
	scene->Begin();

	en.CollectGarbage();

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

	while (en.IsReady()) {
		en.Update(scene);
		en.Render(scene);

		// Use a custom rendering proceedure to draw all sprites
		auto camera = scene->GetCamera();
		auto swapChain = en.GetSwapChain();
		auto& swapChainDesc = swapChain->GetDesc();

		camera->SetOrthoSize(swapChainDesc.Width, swapChainDesc.Height);

		for (auto& obj : insts) {
			obj.mOscillatorX += obj.mOscillatorVelocity;
			obj.mRotation += obj.mAngularVelocity;
		}

		// Draw all sprites
		spriteBatch->Begin(en.GetImmediateContext());

		for (auto& obj : insts) {
			spriteBatch->Draw(spriteTexture, obj.mPositionBase + std::cos(obj.mOscillatorX) * obj.mOscillatorVector,
				DG::float2(128, 128), DG::float2(64, 64), obj.mRotation, obj.mColor);
		}

		spriteBatch->End();

		en.RenderUI();
		en.Present();
	}

	delete scene;

	spriteBatch.reset();
	spriteTexture->Release();

	en.Shutdown();
}