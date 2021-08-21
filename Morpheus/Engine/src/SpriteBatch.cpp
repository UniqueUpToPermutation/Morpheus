#include <Engine/SpriteBatch.hpp>
#include <Engine/Renderer.hpp>
#include <Engine/GeometryStructures.hpp>
#include <Engine/Resources/Shader.hpp>

using namespace DG;

#include <shaders/SpriteBatchStructures.hlsl>

namespace Morpheus {

	Future<SpriteShaders> SpriteShaders::LoadDefaults(
			DG::IRenderDevice* device, 
			IVirtualFileSystem* system) {
		
		struct {
			Future<Handle<DG::IShader>> mVS;
			Future<Handle<DG::IShader>> mGS;
			Future<Handle<DG::IShader>> mPS;
		} data;

				LoadParams<RawShader> vsParams("internal/SpriteBatch.vsh",
			DG::SHADER_TYPE_VERTEX,
			"Sprite Batch VS");

		LoadParams<RawShader> gsParams("internal/SpriteBatch.gsh",
			DG::SHADER_TYPE_GEOMETRY,
			"Sprite Batch GS");

		LoadParams<RawShader> psParams("internal/SpriteBatch.psh",
			DG::SHADER_TYPE_PIXEL,
			"Sprite Batch PS");

		data.mVS = LoadShaderHandle(device, vsParams, system);
		data.mGS = LoadShaderHandle(device, gsParams, system);
		data.mPS = LoadShaderHandle(device, psParams, system);

		FunctionPrototype<
			Future<Handle<DG::IShader>>,
			Future<Handle<DG::IShader>>,
			Future<Handle<DG::IShader>>,
			Promise<SpriteShaders>>
			prototype([]
				(const TaskParams& e,
				Future<Handle<DG::IShader>> vs,
				Future<Handle<DG::IShader>> gs,
				Future<Handle<DG::IShader>> ps,
				Promise<SpriteShaders> output) {
			
			SpriteShaders shaders;
			shaders.mVS = vs.Get();
			shaders.mGS = gs.Get();
			shaders.mPS = ps.Get();

			output = std::move(shaders);
		});

		Promise<SpriteShaders> result;

		prototype(data.mVS, data.mGS, data.mPS, result)
			.SetName("Create SpriteShaders struct");

		return result;
	}

	Future<SpriteBatchPipeline> SpriteBatchPipeline::LoadDefault(
		DG::IRenderDevice* device,
		SpriteBatchGlobals* globals,
		DG::TEXTURE_FORMAT backbufferFormat,
		DG::TEXTURE_FORMAT depthbufferFormat,
		uint samples,
		DG::FILTER_TYPE filterType,
		IVirtualFileSystem* fileSystem) {

		FunctionPrototype<
			Future<SpriteShaders>,
			Promise<SpriteBatchPipeline>>
			prototype([device, backbufferFormat, depthbufferFormat, 
			samples, filterType, fileSystem, globals](const TaskParams& e,
				Future<SpriteShaders> in,
				Promise<SpriteBatchPipeline> out) {
			
			out = SpriteBatchPipeline(device, globals, 
				backbufferFormat, depthbufferFormat, samples,
				filterType, in.Get());
		});

		Promise<SpriteBatchPipeline> result;
		Future<SpriteShaders> shaders = SpriteShaders::LoadDefaults(device, fileSystem);

		prototype(shaders, result)
			.SetName("Create Sprite Batch Pipeline")
			.OnlyThread(THREAD_MAIN);

		return result;
	}

	SpriteBatchPipeline::SpriteBatchPipeline(
		DG::IRenderDevice* device,
		SpriteBatchGlobals* globals,
		DG::TEXTURE_FORMAT backbufferFormat,
		DG::TEXTURE_FORMAT depthbufferFormat,
		uint samples,
		DG::FILTER_TYPE filterType,
		const SpriteShaders& shaders) {

		DG::SamplerDesc SamDesc
		{
			filterType, filterType, filterType, 
			DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP
		};

		DG::IPipelineState* result = nullptr;

		// Create Irradiance Pipeline
		DG::GraphicsPipelineStateCreateInfo PSOCreateInfo;
		DG::PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
		DG::GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

		PSODesc.Name         = "Sprite Batch Pipeline";
		PSODesc.PipelineType = DG::PIPELINE_TYPE_GRAPHICS;

		GraphicsPipeline.NumRenderTargets             = 1;
		GraphicsPipeline.RTVFormats[0]                = backbufferFormat;
		GraphicsPipeline.PrimitiveTopology            = DG::PRIMITIVE_TOPOLOGY_POINT_LIST;
		GraphicsPipeline.RasterizerDesc.CullMode      = DG::CULL_MODE_BACK;
		GraphicsPipeline.DepthStencilDesc.DepthEnable = false;
		GraphicsPipeline.DSVFormat 					  = depthbufferFormat;

		DG::RenderTargetBlendDesc blendState;
		blendState.BlendEnable = true;
		blendState.BlendOp = DG::BLEND_OPERATION_ADD;
		blendState.BlendOpAlpha = DG::BLEND_OPERATION_ADD;
		blendState.DestBlend = DG::BLEND_FACTOR_INV_SRC_ALPHA;
		blendState.SrcBlend = DG::BLEND_FACTOR_SRC_ALPHA;
		blendState.DestBlendAlpha = DG::BLEND_FACTOR_ONE;
		blendState.SrcBlendAlpha = DG::BLEND_FACTOR_ONE;

		GraphicsPipeline.BlendDesc.RenderTargets[0] = blendState;

		// Number of MSAA samples
		GraphicsPipeline.SmplDesc.Count = 1;

		uint stride = sizeof(SpriteBatchVSInput);

		std::vector<DG::LayoutElement> layoutElements = {
			DG::LayoutElement(0, 0, 4, DG::VT_FLOAT32, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(1, 0, 4, DG::VT_FLOAT32, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(2, 0, 2, DG::VT_FLOAT32, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(3, 0, 2, DG::VT_FLOAT32, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(4, 0, 2, DG::VT_FLOAT32, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(5, 0, 2, DG::VT_FLOAT32, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
		};

		GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
		GraphicsPipeline.InputLayout.LayoutElements = &layoutElements[0];

		PSOCreateInfo.pVS = shaders.mVS;
		PSOCreateInfo.pGS = shaders.mGS;
		PSOCreateInfo.pPS = shaders.mPS;

		PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

		// clang-format off
		DG::ShaderResourceVariableDesc Vars[] = 
		{
			{DG::SHADER_TYPE_PIXEL, "mTexture", DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
		};
		// clang-format on
		PSODesc.ResourceLayout.NumVariables = _countof(Vars);
		PSODesc.ResourceLayout.Variables    = Vars;

		// clang-format off
		DG::ImmutableSamplerDesc ImtblSamplers[] =
		{
			{DG::SHADER_TYPE_PIXEL, "mTexture_sampler", SamDesc}
		};
		// clang-format on
		PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
		PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;

		device->CreateGraphicsPipelineState(PSOCreateInfo, &result);

		VertexLayout layout;
		layout.mElements = std::move(layoutElements);
		layout.mPosition = 0;
		
		mPipeline = result;
		mShaders = shaders;

		result->GetStaticVariableByName(DG::SHADER_TYPE_VERTEX, "Globals")
			->Set(globals->GetCameraBuffer());

		result->Release();
	}

	SpriteBatchState SpriteBatchPipeline::CreateState() {
		DG::IShaderResourceBinding* binding = nullptr;
		mPipeline->CreateShaderResourceBinding(&binding, true);

		DG::IShaderResourceVariable* textureVar = 
			binding->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mTexture");

		return SpriteBatchState(binding, textureVar, mPipeline.Ptr());
	}

	SpriteBatch::SpriteBatch(DG::IRenderDevice* device,
		SpriteBatchState&& defaultState, 
		uint batchSize) : mDefaultState(std::move(defaultState)) {

		mBatchSize = batchSize;
		mBatchSizeBytes = batchSize * sizeof(SpriteBatchVSInput);

		DG::BufferDesc desc;
		desc.Name = "Sprite Batch Buffer";
		desc.BindFlags = DG::BIND_VERTEX_BUFFER;
		desc.uiSizeInBytes = mBatchSizeBytes;
		desc.Usage = DG::USAGE_DYNAMIC;
		desc.CPUAccessFlags = DG::CPU_ACCESS_WRITE;

		DG::IBuffer* buf = nullptr;
		device->CreateBuffer(desc, nullptr, &buf);
		mBuffer = buf; // Adds to reference counter
		buf->Release(); 
	}

	void SpriteBatch::Begin(DG::IDeviceContext* context, const SpriteBatchState* state) {
		if (state) {
			mCurrentState = *state;
		} else {
			mCurrentState = mDefaultState;
		}

		context->SetPipelineState(mCurrentState.mPipeline);

		DG::IBuffer* buffers[] = { mBuffer };
		DG::Uint32 offsets[] = { 0 };
		
		context->SetVertexBuffers(0, 1, buffers, offsets, 
			DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, 
			DG::SET_VERTEX_BUFFERS_FLAG_RESET);

		mMapHelper.Map(context, mBuffer, DG::MAP_WRITE, DG::MAP_FLAG_DISCARD);

		mWriteIndex = 0;
		mCurrentContext = context;
		mLastTexture = nullptr;
	}

	void SpriteBatch::Flush() {
		if (mWriteIndex > 0) {
			DG::DrawAttribs attribs;
			attribs.Flags = DG::DRAW_FLAG_VERIFY_ALL;
			attribs.StartVertexLocation = 0;
			attribs.NumVertices = mWriteIndex;
			
			mMapHelper.Unmap();
			mCurrentState.mTextureVariable->Set(
				mLastTexture->GetDefaultView(DG::TEXTURE_VIEW_SHADER_RESOURCE));
			mCurrentContext->CommitShaderResources(mCurrentState.mShaderBinding, 
				RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			mCurrentContext->Draw(attribs);

			mMapHelper.Map(mCurrentContext, mBuffer, DG::MAP_WRITE, DG::MAP_FLAG_DISCARD);
			
			mWriteIndex = 0;
		}
	}

	void SpriteBatch::End() {
		Flush();
		mMapHelper.Unmap();
		mCurrentState = SpriteBatchState();
	}

	void SpriteBatch::Draw(DG::ITexture* texture, const DG::float3& pos,
		const DG::float2& size, const SpriteRect& rect, 
		const DG::float2& origin, const float rotation, 
		const DG::float4& color) {

		if (!mLastTexture)
			mLastTexture = texture;
		if (mWriteIndex == mBatchSize || (mLastTexture && mLastTexture != texture)) {
			Flush();
			mLastTexture = texture;
		}

		SpriteBatchVSInput* instance = &mMapHelper[mWriteIndex];
		auto& desc = texture->GetDesc();
		DG::float2 dim2d(desc.Width, desc.Height);
		
		DG::float2 uvtop_unscaled(rect.mPosition.x - 0.5f, rect.mPosition.y - 0.5f);
		auto uvbottom_unscaled = uvtop_unscaled + rect.mSize;

		instance->mPos.x = pos.x;
		instance->mPos.y = pos.y;
		instance->mPos.z = pos.z;
		instance->mPos.w = rotation;

		instance->mColor = color;
		instance->mOrigin = origin;
		instance->mSize = size;
		instance->mUVTop = uvtop_unscaled / dim2d;
		instance->mUVBottom = uvbottom_unscaled / dim2d;

		++mWriteIndex;
	}

	void SpriteBatch::Draw(const SpriteBatchCall2D sprites[], size_t count) {
		for (size_t i = 0; i < count; ++i) {
			auto& sprite = sprites[i];
			Draw(sprite.mTexture, sprite.mPosition, sprite.mSize, sprite.mRect,
				sprite.mOrigin, sprite.mRotation, sprite.mColor);
		}
	}

	void SpriteBatch::Draw(const SpriteBatchCall3D sprites[], size_t count) {
		for (size_t i = 0; i < count; ++i) {
			auto& sprite = sprites[i];
			Draw(sprite.mTexture, sprite.mPosition, sprite.mSize, sprite.mRect,
				sprite.mOrigin, sprite.mRotation, sprite.mColor);
		}
	}

	void SpriteBatch::Draw(DG::ITexture* texture, const DG::float2& pos, 
		const DG::float2& size, const SpriteRect& rect, 
		const DG::float2& origin, const float rotation, 
		const DG::float4& color) {

		if (!mLastTexture)
			mLastTexture = texture;
		if (mWriteIndex == mBatchSize || mLastTexture != texture) {
			Flush();
			mLastTexture = texture;
		}

		SpriteBatchVSInput* instance = &mMapHelper[mWriteIndex];
		auto& desc = texture->GetDesc();
		DG::float2 dim2d(desc.Width, desc.Height);
		
		DG::float2 uvtop_unscaled(rect.mPosition.x - 0.5f, rect.mPosition.y - 0.5f);
		auto uvbottom_unscaled = uvtop_unscaled + rect.mSize;

		instance->mPos.x = pos.x;
		instance->mPos.y = pos.y;
		instance->mPos.z = 0.0;
		instance->mPos.w = rotation;

		instance->mColor = color;
		instance->mOrigin = origin;
		instance->mSize = size;
		instance->mUVTop = uvtop_unscaled / dim2d;
		instance->mUVBottom = uvbottom_unscaled / dim2d;

		++mWriteIndex;
	}
}