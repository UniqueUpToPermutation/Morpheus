#include <Engine/Core.hpp>

using namespace Morpheus;

int main(int argc, char** argv) {
	Engine en;

	en.Startup(argc, argv);

	Scene* scene = new Scene();

	auto camera = scene->GetCamera();

	auto SCD = en.GetSwapChain()->GetDesc();
	camera->SetType(CameraType::ORTHOGRAPHIC);
	camera->SetOrthoSize(SCD.Width, SCD.Height);
	camera->SetClipPlanes(-1.0, 1.0);

	TextureResource* texture = en.GetResourceManager()->Load<TextureResource>("brick_albedo.png");
	std::unique_ptr<SpriteBatch> spriteBatch(
		new SpriteBatch(en.GetDevice(), en.GetResourceManager()));

	en.SetScene(scene);

	float t = 0.0;

	while (en.IsReady()) {
		en.Update();
		en.Render();

		spriteBatch->Begin(en.GetImmediateContext());
		spriteBatch->Draw(texture, DG::float2(0.0, 0.0), 
			SpriteRect(DG::float2(0.0, 0.0), DG::float2(256, 256)),
			DG::float2(128, 128), t);

		spriteBatch->Draw(texture, DG::float2(256.0, 256.0), 
			SpriteRect(DG::float2(0.0, 0.0), DG::float2(256, 256)),
			DG::float2(128, 128), t);

		t += 0.01;
		spriteBatch->End();

		en.Present();
	}

	spriteBatch.reset();
	texture->Release();

	en.Shutdown();
}