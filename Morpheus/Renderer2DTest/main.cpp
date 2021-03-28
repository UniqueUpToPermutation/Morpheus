#include <Engine/Core.hpp>
#include <Engine/Engine2D/Renderer2D.hpp>
#include <Engine/Engine2D/Sprite.hpp>
#include <Engine/Engine2D/Tilemap.hpp>

using namespace Morpheus;

int main() {
	EngineParams params;
	params.mThreads.mThreadCount = 1;

	Engine en;

	en.AddComponent<Renderer2D>();
	en.Startup(params);

	std::unique_ptr<Scene> scene(new Scene());
	TextureResource* texture = en.GetResourceManager()->Load<TextureResource>("blocks_1.png");

	RenderLayer2DComponent renderLayer;

	renderLayer.mId = 0;
	renderLayer.mName = "Tilemap";
	renderLayer.mOrder = -1;
	renderLayer.mSorting = LayerSorting2D::SORT_BY_Y_DECREASING;
	
	scene->CreateNode().Add<RenderLayer2DComponent>(renderLayer);

	TilemapComponent tilemap;
	TilemapView view(tilemap);
	
	DG::float2 spacing = texture->GetDimensions2D();
	spacing.x /= 2.0f;
	spacing.y = spacing.x / 2.0f;

	uint width = 21;
	uint height = 21;

	view.SetType(TilemapType::ISOMETRIC);
	view.SetDimensions(width, height);
	view.CreateNewLayer(texture->GetDimensions2D(), spacing);
	view.CreateNewTileset(texture, texture->GetDimensions2D(), texture->GetDimensions2D() / 2);

	auto tilemapLayer = view[0];
	tilemapLayer.Fill(0);
	tilemapLayer.SetRenderLayer(0);

	for (uint x = 1; x < width; x += 2) {
		for (uint y = 1; y < height; y += 2) {
			tilemapLayer(x, y).SetTileId(TILE_NONE);
		}
	}

	auto tilemapEntity = scene->CreateNode();
	tilemapEntity.Add<TilemapComponent>(std::move(tilemap));
	tilemapEntity.Add<Transform>();

	auto camera = scene->GetCamera();
	camera->SetType(CameraType::ORTHOGRAPHIC);
	camera->SetClipPlanes(-1.0, 1.0);
	scene->GetCameraNode().Add<Transform>();
	scene->GetCameraNode().Add<ScriptComponent>().AddScript<EditorCameraController2D>();

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