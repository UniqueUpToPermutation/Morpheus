#pragma once

#include <Engine/Resources/Resource.hpp>
#include <Engine/Resources/Shader.hpp>
#include <Engine/Resources/Texture.hpp>
#include <Engine/GeometryStructures.hpp>
#include <Engine/Buffer.hpp>
#include <Engine/Graphics.hpp>

#include <shaders/Utils/BasicStructures.hlsl>

#include "MapHelper.hpp"

#define DEFAULT_SPRITE_BATCH_SIZE 100

namespace Morpheus {

	class SpriteBatchGlobals {
	private:
		DynamicGlobalsBuffer<HLSL::CameraAttribs> mCamera;

	public:
		inline SpriteBatchGlobals(DG::IRenderDevice* device) : mCamera(device) {
		}

		inline SpriteBatchGlobals(RealtimeGraphics& graphics) : SpriteBatchGlobals(graphics.Device()) {
		}

		inline DG::IBuffer* GetCameraBuffer() {
			return mCamera.Get();
		}

		inline void Write(DG::IDeviceContext* context, const HLSL::CameraAttribs& attribs) {
			mCamera.Write(context, attribs);
		}
	};

	class SpriteShaders {
	public:
		Handle<DG::IShader> mVS;
		Handle<DG::IShader> mGS;
		Handle<DG::IShader> mPS;

		inline SpriteShaders() {
		}

		inline SpriteShaders(DG::IShader* vs, 
			DG::IShader* gs, 
			DG::IShader* ps) :
			mVS(vs), mGS(gs), mPS(ps) {
		}

		static Future<SpriteShaders> LoadDefaults(
			DG::IRenderDevice* device, 
			IVirtualFileSystem* system = EmbeddedFileLoader::GetGlobalInstance());
	
		static inline Future<SpriteShaders> LoadDefaults(
			RealtimeGraphics& graphics,
			IVirtualFileSystem* system = EmbeddedFileLoader::GetGlobalInstance()) {
			return LoadDefaults(graphics.Device(), system);
		}
	};

	class SpriteBatchState {
	private:
		DG::IShaderResourceVariable* mTextureVariable;
		Handle<DG::IShaderResourceBinding> mShaderBinding;
		Handle<DG::IPipelineState> mPipeline;

	public:
		inline SpriteBatchState() :
			mTextureVariable(nullptr) {
		}

		inline SpriteBatchState(DG::IShaderResourceBinding* shaderBinding, 
			DG::IShaderResourceVariable* textureVariable, 
			DG::IPipelineState* pipeline) :
			mTextureVariable(textureVariable),
			mShaderBinding(shaderBinding),
			mPipeline(pipeline) {
		}

		friend class SpriteBatch;
	};

	class SpriteBatchPipeline {
	public:
		Handle<DG::IPipelineState> mPipeline;
		SpriteShaders mShaders;

		inline SpriteBatchPipeline() {
		}

		SpriteBatchPipeline(DG::IRenderDevice* device,
			SpriteBatchGlobals* globals,
			DG::TEXTURE_FORMAT backbufferFormat,
			DG::TEXTURE_FORMAT depthbufferFormat,
			uint samples,
			DG::FILTER_TYPE filterType,
			const SpriteShaders& shaders);

		inline SpriteBatchPipeline(RealtimeGraphics& graphics,
			SpriteBatchGlobals* globals,
			DG::TEXTURE_FORMAT backbufferFormat,
			DG::TEXTURE_FORMAT depthbufferFormat,
			uint samples,
			DG::FILTER_TYPE filterType,
			const SpriteShaders& shaders) : SpriteBatchPipeline(graphics.Device(),
				globals, backbufferFormat, depthbufferFormat, samples, filterType, shaders) {
		}

		inline SpriteBatchPipeline(RealtimeGraphics& graphics,
			SpriteBatchGlobals* globals,
			DG::FILTER_TYPE filterType,
			const SpriteShaders& shaders) : SpriteBatchPipeline(graphics.Device(),
				globals, 
				graphics.SwapChain()->GetDesc().ColorBufferFormat,
				graphics.SwapChain()->GetDesc().DepthBufferFormat,
				1,
				filterType,
				shaders) {
		}

		inline SpriteBatchPipeline(
			DG::IPipelineState* pipeline,
			SpriteBatchGlobals* globals) :
			mPipeline(pipeline) {
			auto cameraVar = mPipeline->GetStaticVariableByName(DG::SHADER_TYPE_VERTEX, "Globals");
			cameraVar->Set(globals->GetCameraBuffer());
		}

		SpriteBatchState CreateState();

		static Future<SpriteBatchPipeline> LoadDefault(
			DG::IRenderDevice* device,
			SpriteBatchGlobals* globals,
			DG::TEXTURE_FORMAT backbufferFormat,
			DG::TEXTURE_FORMAT depthbufferFormat,
			uint samples,
			DG::FILTER_TYPE filterType,
			IVirtualFileSystem* system = EmbeddedFileLoader::GetGlobalInstance());

		inline static Future<SpriteBatchPipeline> LoadDefault(
			RealtimeGraphics& graphics,
			SpriteBatchGlobals* globals,
			DG::TEXTURE_FORMAT backbufferFormat,
			DG::TEXTURE_FORMAT depthbufferFormat,
			uint samples,
			DG::FILTER_TYPE filterType,
			IVirtualFileSystem* system = EmbeddedFileLoader::GetGlobalInstance()) {
			return LoadDefault(graphics.Device(), globals, backbufferFormat, 
				depthbufferFormat, samples, filterType, system);
		}

		inline static Future<SpriteBatchPipeline> LoadDefault(
			RealtimeGraphics& graphics,
			SpriteBatchGlobals* globals,
			DG::FILTER_TYPE filterType = DG::FILTER_TYPE_LINEAR,
			IVirtualFileSystem* system = EmbeddedFileLoader::GetGlobalInstance()) {
			auto& scDesc = graphics.SwapChain()->GetDesc();
			return LoadDefault(graphics.Device(), globals,
				scDesc.ColorBufferFormat, scDesc.DepthBufferFormat, 1,
				filterType, system);
		}
	};

	struct SpriteBatchVSInput;

	struct SpriteBatchCall2D {
		DG::ITexture* mTexture; 
		DG::float2 mPosition;
		DG::float2 mSize;
		SpriteRect mRect;
		DG::float2 mOrigin; 
		float mRotation;
		DG::float4 mColor;
	};

	struct SpriteBatchCall3D {
		DG::ITexture* mTexture; 
		DG::float3 mPosition;
		DG::float2 mSize;
		SpriteRect mRect;
		DG::float2 mOrigin; 
		float mRotation;
		DG::float4 mColor;
	};

	class SpriteBatch {
	private:
		Handle<DG::IBuffer> mBuffer;

		SpriteBatchState mDefaultState;
		SpriteBatchState mCurrentState;

		DG::ITexture* mLastTexture;
		DG::IDeviceContext* mCurrentContext;

		uint mWriteIndex;
		uint mBatchSize;
		uint mBatchSizeBytes;

		DG::MapHelper<SpriteBatchVSInput> mMapHelper;

	public:
		SpriteBatch(DG::IRenderDevice* device,
			SpriteBatchState&& defaultState, 
			uint batchSize = DEFAULT_SPRITE_BATCH_SIZE);

		inline SpriteBatch(RealtimeGraphics& graphics,	
			SpriteBatchState&& defaultState,
			uint batchSize = DEFAULT_SPRITE_BATCH_SIZE) : 
			SpriteBatch(graphics.Device(), std::move(defaultState), batchSize) {
		}

		inline SpriteBatch(RealtimeGraphics& graphics,	
			SpriteBatchPipeline defaultPipeline,
			uint batchSize = DEFAULT_SPRITE_BATCH_SIZE) : 
			SpriteBatch(graphics.Device(), defaultPipeline.CreateState(), batchSize) {
		}
	
		void Begin(DG::IDeviceContext* context, const SpriteBatchState* state = nullptr);
		void Flush();
		void End();

		void Draw(const SpriteBatchCall2D sprites[], size_t count);
		void Draw(const SpriteBatchCall3D sprites[], size_t count);

		void Draw(DG::ITexture* texture, const DG::float3& pos,
			const DG::float2& size, const SpriteRect& rect, 
			const DG::float2& origin, const float rotation, 
			const DG::float4& color);

		void Draw(DG::ITexture* texture, const DG::float2& pos, 
			const DG::float2& size, const SpriteRect& rect, 
			const DG::float2& origin, const float rotation, 
			const DG::float4& color);

		inline void Draw(DG::ITexture* texture, const DG::float3& pos,
			const DG::float2& size,
			const DG::float2& origin, const float rotation) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, pos, size, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}

		inline void Draw(DG::ITexture* texture, const DG::float2& pos, 
			const DG::float2& size,
			const DG::float2& origin, const float rotation) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, pos,  size, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}

		inline void Draw(DG::ITexture* texture, const DG::float3& pos,
			const DG::float2& size,
			const DG::float2& origin, const float rotation,
			const DG::float4& color) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, pos, size, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, color); 
		}

		inline void Draw(DG::ITexture* texture, const DG::float2& pos, 
			const DG::float2& size,
			const DG::float2& origin, const float rotation,
			const DG::float4& color) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, pos,  size, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, color); 
		}

		inline void Draw(DG::ITexture* texture, const DG::float3& pos) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				DG::float2(0.0, 0.0), 0.0, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float2& pos) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				DG::float2(0.0, 0.0), 0.0, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float3& pos, 
			const DG::float4& color) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				DG::float2(0.0, 0.0), 0.0, color); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float2& pos, 
			const DG::float4& color) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				DG::float2(0.0, 0.0), 0.0, color); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float3& pos, 
			const SpriteRect& rect, const DG::float4& color) {
			Draw(texture, pos, rect.mSize, rect,
				DG::float2(0.0, 0.0), 0.0, color); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float2& pos, 
			const SpriteRect& rect, const DG::float4& color) {
			Draw(texture, pos, rect.mSize, rect,
				DG::float2(0.0, 0.0), 0.0, color); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float3& pos, 
			const SpriteRect& rect) {
			Draw(texture, pos, rect.mSize, rect,
				DG::float2(0.0, 0.0), 0.0, 
				DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float2& pos, 
			const SpriteRect& rect) {
			Draw(texture, pos, rect.mSize, rect,
				DG::float2(0.0, 0.0), 0.0, 
				DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float3& pos, 
			const SpriteRect& rect, const DG::float2& origin, const float rotation) {
			Draw(texture, pos, rect.mSize, rect,
				origin, rotation, 
				DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float2& pos, 
			const SpriteRect& rect, const DG::float2& origin, const float rotation) {
			Draw(texture, pos, rect.mSize, rect,
				origin, rotation, 
				DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float3& pos, 
			const DG::float2& origin, const float rotation, const DG::float4& color) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, color); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float2& pos, 
			const DG::float2& origin, const float rotation, const DG::float4& color) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, color); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float3& pos, 
			const DG::float2& origin, const float rotation) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float2& pos, 
			const DG::float2& origin, const float rotation) {
			auto& desc = texture->GetDesc();
			auto dimensions = DG::float2(desc.Width, desc.Height);
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float3& pos,
			const SpriteRect& rect, const DG::float2& origin, 
			const float rotation, const DG::float4& color) {
			Draw(texture, pos, rect.mSize, rect,
				origin, rotation, color); 
		}
		inline void Draw(DG::ITexture* texture, const DG::float2& pos, 
			const SpriteRect& rect, const DG::float2& origin, 
			const float rotation, const DG::float4& color) {
			Draw(texture, pos, rect.mSize, rect,
				origin, rotation, color); 
		}


		inline void Draw(Texture* texture, const DG::float3& pos,
			const DG::float2& size, const SpriteRect& rect, 
			const DG::float2& origin, const float rotation, 
			const DG::float4& color) {
			Draw(texture->GetRasterTexture(), 
				pos, size, rect, origin, rotation, color);
		}

		inline void Draw(Texture* texture, const DG::float2& pos, 
			const DG::float2& size, const SpriteRect& rect, 
			const DG::float2& origin, const float rotation, 
			const DG::float4& color) {
			Draw(texture->GetRasterTexture(), 
				pos, size, rect, origin, rotation, color);
		}

		inline void Draw(Texture* texture, const DG::float3& pos,
			const DG::float2& size,
			const DG::float2& origin, const float rotation) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos,  size, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}

		inline void Draw(Texture* texture, const DG::float2& pos, 
			const DG::float2& size,
			const DG::float2& origin, const float rotation) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos,  size, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}

		inline void Draw(Texture* texture, const DG::float3& pos,
			const DG::float2& size,
			const DG::float2& origin, const float rotation,
			const DG::float4& color) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos, size, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, color); 
		}

		inline void Draw(Texture* texture, const DG::float2& pos, 
			const DG::float2& size,
			const DG::float2& origin, const float rotation,
			const DG::float4& color) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos,  size, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, color); 
		}

		inline void Draw(Texture* texture, const DG::float3& pos) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				DG::float2(0.0, 0.0), 0.0, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(Texture* texture, const DG::float2& pos) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				DG::float2(0.0, 0.0), 0.0, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(Texture* texture, const DG::float3& pos, 
			const DG::float4& color) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				DG::float2(0.0, 0.0), 0.0, color); 
		}
		inline void Draw(Texture* texture, const DG::float2& pos, 
			const DG::float4& color) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				DG::float2(0.0, 0.0), 0.0, color); 
		}
		inline void Draw(Texture* texture, const DG::float3& pos, 
			const SpriteRect& rect, const DG::float4& color) {
			Draw(texture, pos, rect.mSize, rect,
				DG::float2(0.0, 0.0), 0.0, color); 
		}
		inline void Draw(Texture* texture, const DG::float2& pos, 
			const SpriteRect& rect, const DG::float4& color) {
			Draw(texture, pos, rect.mSize, rect,
				DG::float2(0.0, 0.0), 0.0, color); 
		}
		inline void Draw(Texture* texture, const DG::float3& pos, 
			const SpriteRect& rect) {
			Draw(texture, pos, rect.mSize, rect,
				DG::float2(0.0, 0.0), 0.0, 
				DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(Texture* texture, const DG::float2& pos, 
			const SpriteRect& rect) {
			Draw(texture, pos, rect.mSize, rect,
				DG::float2(0.0, 0.0), 0.0, 
				DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(Texture* texture, const DG::float3& pos, 
			const SpriteRect& rect, const DG::float2& origin, const float rotation) {
			Draw(texture, pos, rect.mSize, rect,
				origin, rotation, 
				DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(Texture* texture, const DG::float2& pos, 
			const SpriteRect& rect, const DG::float2& origin, const float rotation) {
			Draw(texture, pos, rect.mSize, rect,
				origin, rotation, 
				DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(Texture* texture, const DG::float3& pos, 
			const DG::float2& origin, const float rotation, const DG::float4& color) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, color); 
		}
		inline void Draw(Texture* texture, const DG::float2& pos, 
			const DG::float2& origin, const float rotation, const DG::float4& color) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, color); 
		}
		inline void Draw(Texture* texture, const DG::float3& pos, 
			const DG::float2& origin, const float rotation) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(Texture* texture, const DG::float2& pos, 
			const DG::float2& origin, const float rotation) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(Texture* texture, const DG::float3& pos,
			const SpriteRect& rect, const DG::float2& origin, 
			const float rotation, const DG::float4& color) {
			Draw(texture, pos, rect.mSize, rect,
				origin, rotation, color); 
		}
		inline void Draw(Texture* texture, const DG::float2& pos, 
			const SpriteRect& rect, const DG::float2& origin, 
			const float rotation, const DG::float4& color) {
			Draw(texture, pos, rect.mSize, rect,
				origin, rotation, color); 
		}
	};
}