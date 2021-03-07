#include <Engine/Core.hpp>
#include <Engine/DefaultRenderer.hpp>
#include <Engine/Resources/RawTexture.hpp>
#include <Engine/Resources/RawSampler.hpp>
#include <Engine/Resources/TextureIterator.hpp>

#include <filesystem>

using namespace Morpheus;

int main() {
	RawTexture texture("brick_albedo.png");

	{
		RawTexture textureCopy;
		textureCopy.CopyFrom(texture);
		textureCopy.Save("brick.tark");
	}

	RawTexture textureFromArchive("brick.tark");
	
	// Invert the brick texture
	{
		TextureIterator it(&texture);
		for (; it.IsValid(); it.Next()) {
			DG::float4 value;
			it.Value().Read(&value);
			value.r = 1.0 - value.r;
			value.g = 1.0 - value.g;
			value.b = 1.0 - value.b;
			it.Value().Write(value);
		}
	}

	// Create a texture programmatically
	DG::TextureDesc texTest;
	texTest.Width = 512;
	texTest.Height = 512;
	texTest.Format = DG::TEX_FORMAT_RGBA8_UNORM;
	texTest.MipLevels = 3;
	texTest.Type = DG::RESOURCE_DIM_TEX_2D;
	texTest.Usage = DG::USAGE_IMMUTABLE;
	texTest.BindFlags = DG::BIND_SHADER_RESOURCE;

	RawTexture fromDesc(texTest);

	for (int i = 0; i < texTest.MipLevels; ++i)
	{
		TextureIterator it(&fromDesc, i);
		for (; it.IsValid(); it.Next()) {
			DG::float4 value;

			auto uv = it.Position();

			value.r = uv.x;
			value.g = uv.y;
			value.b = 1.0;
			value.a = 1.0;
			it.Value().Write(value);
		}
	}

	// Load from an archive created from a texture from the GPU
	RawTexture fromArchive;
	bool bArchiveTextureExists = false;
	if (std::filesystem::exists("FromGpu.tark")) {
		fromArchive.Load("FromGpu.tark");
		bArchiveTextureExists = true;
	}

	// Create scene to render textures
	Engine en;
	en.AddComponent<DefaultRenderer>();
	en.Startup();

	std::unique_ptr<Scene> scene(new Scene());
	auto camera = scene->GetCamera();
	camera->SetType(CameraType::ORTHOGRAPHIC);
	camera->SetClipPlanes(-1.0, 1.0);

	std::unique_ptr<SpriteBatch> spriteBatch(
		new SpriteBatch(en.GetDevice(), en.GetResourceManager()));

	en.InitializeDefaultSystems(scene.get());
	scene->Begin();

	en.CollectGarbage();

	// Spawn textures on GPU
	auto gpuTexture1 = texture.SpawnOnGPU(en.GetDevice());
	auto gpuTexture2 = fromDesc.SpawnOnGPU(en.GetDevice());

	DG::ITexture* gpuTexture3 = nullptr;
	if (bArchiveTextureExists) 
		gpuTexture3 = fromArchive.SpawnOnGPU(en.GetDevice());

	auto gpuTexture4 = textureFromArchive.SpawnOnGPU(en.GetDevice());

	texture.Clear();
	fromDesc.Clear();
	fromArchive.Clear();
	textureFromArchive.Clear();

	auto& d = gpuTexture1->GetDesc();

	while (en.IsReady()) {

		auto& swapChainDesc = en.GetSwapChain()->GetDesc();
		camera->SetOrthoSize(swapChainDesc.Width, swapChainDesc.Height);

		en.Update(scene.get());
		en.Render(scene.get());

		spriteBatch->Begin(en.GetImmediateContext());
		spriteBatch->Draw(gpuTexture4, DG::float2(-400.0, -400.0));
		spriteBatch->Draw(gpuTexture1, DG::float2(-300.0, -300.0));
		spriteBatch->Draw(gpuTexture2, DG::float2(0.0, 0.0));
		if (gpuTexture3)
			spriteBatch->Draw(gpuTexture3, DG::float2(300.0, 300.0));
		spriteBatch->End();

		en.RenderUI();
		en.Present();
	}

	// Retreive textures from GPU and write to disk
	RawTexture fromGpu1(gpuTexture1, en.GetDevice(), en.GetImmediateContext());
	fromGpu1.SavePng("FromGpu1.png", false);
	fromGpu1.Save("FromGpu.tark");

	RawTexture fromGpu2(gpuTexture2, en.GetDevice(), en.GetImmediateContext());
	fromGpu2.SavePng("FromGpu2.png", true);

	gpuTexture1->Release();
	gpuTexture2->Release();
	if (gpuTexture3)
		gpuTexture3->Release();
	gpuTexture4->Release();

	spriteBatch.reset();
	scene.reset();

	en.Shutdown();
}