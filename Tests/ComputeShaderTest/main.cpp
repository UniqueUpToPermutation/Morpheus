#include <Engine/Core.hpp>
#include <Engine/Resources/Texture.hpp>
#include <Engine/SpriteBatch.hpp>

#include <GLFW/glfw3.h>

#include "shaders/Mandelbrot.csh"

using namespace Morpheus;

void AddEmbeddedShaders(std::unordered_map<std::string, const char*>* map);

MAIN() {
	// Startup everything
	EmbeddedFileLoader embeddedFiles;
	embeddedFiles.Add(&AddEmbeddedShaders);

	Platform platform(CreatePlatformGLFW());
	platform.Startup();

	RealtimeGraphics graphics(platform);
	graphics.Startup();

	// Texture creation
	int cellSizeX = 32;
	int cellSizeY = 32;

	auto getTextureDimensions = [cellSizeX, cellSizeY]
		(int width, int height, int* out_width, int* out_height) {
		*out_width = ((width + cellSizeX - 1) / cellSizeX) * cellSizeX;
		*out_height = ((height + cellSizeY - 1) / cellSizeY) * cellSizeY;
	};

	auto generateTexture = [getTextureDimensions, &graphics, &platform] {
		int width;
		int height;
		glfwGetFramebufferSize(platform.GetWindowGLFW(),
			&width, &height);

		getTextureDimensions(width, height, &width, &height);

		DG::TextureDesc desc;
		desc.Width = width;
		desc.Height = height;
		desc.Type = DG::RESOURCE_DIM_TEX_2D;
		desc.MipLevels = 1;
		desc.Usage = DG::USAGE_DEFAULT;
		desc.BindFlags = DG::BIND_SHADER_RESOURCE | DG::BIND_UNORDERED_ACCESS;
		desc.Format = DG::TEX_FORMAT_RGBA8_UNORM;

		return Texture(graphics.Device(), desc);
	};

	// Compile compute shaders
	ShaderPreprocessorConfig config;
	config.mDefines["CELL_SIZE_X"] = std::to_string(cellSizeX);
	config.mDefines["CELL_SIZE_Y"] = std::to_string(cellSizeY);

	LoadParams<RawShader> shaderParams(
		"internal/Mandelbrot.csh",
		DG::SHADER_TYPE_COMPUTE,
		"Compute Shader",
		config,
		"CSMain");

	auto computeShader = LoadShaderHandle(
		graphics.Device(), 
		shaderParams, 
		&embeddedFiles).Evaluate();

	// Create compute pipeline
	DG::ShaderResourceVariableDesc Vars[] = 
    {
        {DG::SHADER_TYPE_COMPUTE, "mOutput", DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
		{DG::SHADER_TYPE_COMPUTE, "mUniformGlobals", DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC}
    };

	DG::ComputePipelineStateCreateInfo pipelineInfo;
	auto& psoDesc = pipelineInfo.PSODesc;
	pipelineInfo.pCS = computeShader;

	psoDesc.PipelineType = DG::PIPELINE_TYPE_COMPUTE;
	psoDesc.ResourceLayout.Variables = Vars;
	psoDesc.ResourceLayout.NumVariables = _countof(Vars);

	Handle<DG::IPipelineState> pipeline;
	graphics.Device()->CreateComputePipelineState(pipelineInfo, pipeline.Ref());

	// Create uniform buffer
	DynamicGlobalsBuffer<UniformGlobals> globalsBuffer(graphics.Device());
	auto unformGlobalsVar = pipeline->GetStaticVariableByName(DG::SHADER_TYPE_COMPUTE, "mUniformGlobals");
	unformGlobalsVar->Set(globalsBuffer.Get());

	// Create shader resource binding
	Handle<DG::IShaderResourceBinding> srb;
	pipeline->CreateShaderResourceBinding(srb.Ref(), true);

	DG::IShaderResourceVariable* computeShaderOutput = 
		srb->GetVariableByName(DG::SHADER_TYPE_COMPUTE, "mOutput");

	// Create texture
	Texture texture = generateTexture();
	auto view = texture.GetUnorderedAccessView();
	computeShaderOutput->Set(view);

	// Create sprite batch
	SpriteBatchGlobals sbGlobals(graphics);
	auto& scDesc = graphics.SwapChain()->GetDesc();	

	ImmediateComputeQueue queue;
	auto sbPipelineFuture = SpriteBatchPipeline::LoadDefault(graphics, &sbGlobals, 
		DG::FILTER_TYPE_LINEAR, &embeddedFiles);

	SpriteBatch spriteBatch(graphics, sbPipelineFuture.Evaluate());

	// Setup camera
	Camera camera;
	camera.SetType(CameraType::ORTHOGRAPHIC);
	camera.SetClipPlanes(-1.0, 1.0);

	DG::Timer timer;
	FrameTime time(timer);

	while (platform.IsValid()) {
		time.UpdateFrom(timer);
		platform.MessagePump();

		int width, height;
		glfwGetFramebufferSize(platform.GetWindowGLFW(),
			&width, &height);

		int tex_width, tex_height;
		getTextureDimensions(width, height, &tex_width, &tex_height);

		if (tex_width != texture.GetWidth() || tex_height != texture.GetHeight()) {
			texture = generateTexture();
			computeShaderOutput->Set(texture.GetUnorderedAccessView());
		}

		auto context = graphics.ImmediateContext();
		auto swapChain = graphics.SwapChain();
		auto rtv = swapChain->GetCurrentBackBufferRTV();
		auto dsv = swapChain->GetDepthBufferDSV();
		float color[] = { 0.5f, 0.5f, 1.0f, 1.0f };
		context->SetRenderTargets(1, &rtv, dsv, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		context->ClearRenderTarget(rtv, color, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		context->ClearDepthStencil(dsv, DG::CLEAR_DEPTH_FLAG, 1.0f, 0, 
			DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		// Write time globals
		UniformGlobals globals;
		globals.mTime = time.mCurrentTime;
		globalsBuffer.Write(graphics.ImmediateContext(), globals);

		DG::DispatchComputeAttribs attribs;
		attribs.ThreadGroupCountX = texture.GetWidth() / cellSizeX;
		attribs.ThreadGroupCountY = texture.GetHeight() / cellSizeY;
		attribs.ThreadGroupCountZ = 1;
		context->SetPipelineState(pipeline);
		context->CommitShaderResources(srb, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		context->DispatchCompute(attribs);

		// Make sure that the camera width/height match window width/height
		auto& scDesc = swapChain->GetDesc();
		camera.SetOrthoSize(scDesc.Width, scDesc.Height);

		// Send camera information to the GPU
		auto cameraAttribs = camera.GetLocalAttribs(graphics);
		sbGlobals.Write(graphics.ImmediateContext(), cameraAttribs);

		spriteBatch.Begin(graphics.ImmediateContext());
		spriteBatch.Draw(&texture, DG::float3(-(float)scDesc.Width / 2.0f, -(float)scDesc.Height / 2.0f, 0.0f));
		spriteBatch.End();

		graphics.Present(1);
	}

	graphics.Shutdown();
	platform.Shutdown();
}