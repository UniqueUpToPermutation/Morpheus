#include <Engine/Core.hpp>
#include <Engine/Engine2D/Renderer2D.hpp>
#include <Engine/Engine2D/Sprite.hpp>

using namespace Morpheus;

int main() {
	Engine en;

	en.AddComponent<Renderer2D>();
	en.Startup();

	std::unique_ptr<Scene> scene(new Scene());
	TextureResource* texture = en.GetResourceManager()->Load<TextureResource>("blocks_1.png");

	RenderLayer2DComponent renderLayer1;
	RenderLayer2DComponent renderLayer2;

	renderLayer1.mId = 0;
	renderLayer1.mName = "Background";
	renderLayer1.mOrder = -1;
	
	renderLayer2.mId = 1;
	renderLayer2.mName = "Foreground";
	renderLayer2.mOrder = 1;

	scene->CreateNode().Add<RenderLayer2DComponent>(renderLayer1);
	scene->CreateNode().Add<RenderLayer2DComponent>(renderLayer2);

	{
		auto spriteEntity = scene->CreateNode();
		auto& sprite = spriteEntity.Add<SpriteComponent>(texture);
		sprite.mRenderLayer = 0;
		sprite.mColor.g = 0.5;
		sprite.mColor.r = 0.0;
		spriteEntity.Add<Transform>();
	}

	// Goes on top
	{
		auto spriteEntity = scene->CreateNode();
		auto& sprite = spriteEntity.Add<SpriteComponent>(texture);
		sprite.mRenderLayer = 1;
		sprite.mColor.g = 0.0;
		sprite.mColor.b = 0.0;
		spriteEntity.Add<Transform>().SetTranslation(20.0f, 20.0f, 0.0f);
	}

	{
		auto spriteEntity = scene->CreateNode();
		auto& sprite = spriteEntity.Add<SpriteComponent>(texture);
		sprite.mRenderLayer = 0;
		sprite.mColor.g = 0.5;
		sprite.mColor.r = 0.0;
		spriteEntity.Add<Transform>().SetTranslation(40.0f, 40.0f, 0.0f);
	}

	auto camera = scene->GetCamera();
	camera->SetType(CameraType::ORTHOGRAPHIC);
	camera->SetClipPlanes(-1.0, 1.0);

	texture->Release();

	en.InitializeDefaultSystems(scene.get());
	scene->Begin();

	while (en.IsReady()) {
		const auto& SCD = en.GetSwapChain()->GetDesc();
		camera->SetOrthoSize(SCD.Width, SCD.Height);

		en.Update(scene.get());
		en.Render(scene.get());
		en.RenderUI();
		en.Present();
	}

	scene.reset();
	en.Shutdown();
}