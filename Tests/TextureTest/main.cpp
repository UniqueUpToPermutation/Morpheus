#include <Engine/Core.hpp>
#include <Engine/Resources/Texture.hpp>
#include <Engine/Resources/RawSampler.hpp>
#include <Engine/Resources/TextureIterator.hpp>
#include <Engine/SpriteBatch.hpp>

#include <filesystem>

using namespace Morpheus;

int main() {
	Texture texture(Device::CPU(), "brick_albedo.png");
	
	assert(texture.GetDevice().IsCPU());

	{
		Texture textureCopy;
		textureCopy.CopyFrom(texture);
		textureCopy.BinarySerializeToFile("brick.bin");
	}

	Texture textureFromArchive;
	textureFromArchive.BinaryDeserializeFromFile("brick.bin");

	assert(textureFromArchive.GetDevice().IsCPU());
	
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

	Texture fromDesc(texTest);

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
	Texture fromArchive;
	bool bArchiveTextureExists = false;
	if (std::filesystem::exists("FromGpu.bin")) {
		fromArchive.BinaryDeserializeFromFile("FromGpu.bin");
		bArchiveTextureExists = true;
	}

	// Create scene to render textures
	Platform platform;
	platform.Startup();

	RealtimeGraphics graphics(platform);
	graphics.Startup();

	Camera camera;
	camera.SetType(CameraType::ORTHOGRAPHIC);
	camera.SetClipPlanes(-1.0, 1.0);

	{
		SpriteBatchGlobals sbGlobals(graphics);
		EmbeddedFileLoader embeddedFileSystem;

		auto sbPipeline = SpriteBatchPipeline::LoadDefault(graphics, &sbGlobals, 
			DG::FILTER_TYPE_LINEAR, &embeddedFileSystem).Evaluate();

		SpriteBatch spriteBatch(graphics, sbPipeline);

		// Spawn textures on GPU
		auto gpuTexture1 = texture.To(graphics.Device());
		auto gpuTexture2 = fromDesc.To(graphics.Device());
		Texture gpuTexture3;
		if (bArchiveTextureExists) 
			gpuTexture3 = std::move(fromArchive.To(graphics.Device()));
		auto gpuTexture4 = textureFromArchive.To(graphics.Device());

		assert(gpuTexture1.GetDevice().IsGPU());
		assert(gpuTexture2.GetDevice().IsGPU());
		assert(gpuTexture4.GetDevice().IsGPU());

		texture.Clear();
		fromDesc.Clear();
		fromArchive.Clear();
		textureFromArchive.Clear();

		while (platform->IsValid()) {

			auto& swapChainDesc = graphics.SwapChain()->GetDesc();
			camera.SetOrthoSize(swapChainDesc.Width, swapChainDesc.Height);

			// Perform window IO
			platform->MessagePump();

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

			// Draw textures
			spriteBatch.Begin(graphics.ImmediateContext());
			spriteBatch.Draw(&gpuTexture4, DG::float2(-400.0, -400.0));
			spriteBatch.Draw(&gpuTexture1, DG::float2(-300.0, -300.0));
			spriteBatch.Draw(&gpuTexture2, DG::float2(0.0, 0.0));
			if (gpuTexture3)
				spriteBatch.Draw(&gpuTexture3, DG::float2(300.0, 300.0));
			spriteBatch.End();

			graphics.Present(1);
		}

		// Retreive textures from GPU and write to disk
		Texture fromGpu1 = gpuTexture1.To(Device::CPU(), graphics.ImmediateContext());
		
		fromGpu1.SavePng("FromGpu1.png", false);
		fromGpu1.BinarySerializeToFile("FromGpu.bin");

		Texture fromGpu2 = gpuTexture2.To(Device::CPU(), graphics.ImmediateContext());
		
		fromGpu2.SavePng("FromGpu2.png", true);

		assert(fromGpu1.GetDevice().IsCPU());
		assert(fromGpu2.GetDevice().IsCPU());

		gpuTexture1 = Texture();
		gpuTexture2 = Texture();
		gpuTexture3 = Texture();
		gpuTexture4 = Texture();
	}

	graphics.Shutdown();
	platform.Shutdown();
}