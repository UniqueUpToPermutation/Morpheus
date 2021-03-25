#pragma once

#include <Engine/Resources/Resource.hpp>
#include <Engine/Resources/ShaderResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/TextureResource.hpp>

#include "MapHelper.hpp"

#define DEFAULT_SPRITE_BATCH_SIZE 100

namespace Morpheus {
	struct SpriteRect {
		DG::float2 mPosition;
		DG::float2 mSize;

		inline SpriteRect() {
		}

		inline SpriteRect(const DG::float2& position, const DG::float2& size) :
			mPosition(position),
			mSize(size) {
		}

		inline SpriteRect(float upperX, float upperY, float sizeX, float sizeY) :
			mPosition(upperX, upperY),
			mSize(sizeX, sizeY) {
		}
	};

	class SpriteBatchState {
	private:
		DG::IShaderResourceBinding* mShaderBinding;
		DG::IShaderResourceVariable* mTextureVariable;
		PipelineResource* mPipeline;

	public:
		SpriteBatchState() :
			mShaderBinding(nullptr),
			mTextureVariable(nullptr),
			mPipeline(nullptr) {
		}

		SpriteBatchState(DG::IShaderResourceBinding* shaderBinding, 
			DG::IShaderResourceVariable* textureVariable, 
			PipelineResource* pipeline) :
			mShaderBinding(shaderBinding),
			mTextureVariable(textureVariable),
			mPipeline(pipeline) {
		}

		void CopyFrom(const SpriteBatchState& state) {
			mShaderBinding = state.mShaderBinding;
			mTextureVariable = state.mTextureVariable;
			mPipeline = state.mPipeline;

			mShaderBinding->AddRef();
			mPipeline->AddRef();
			mTextureVariable->AddRef();
		}

		void Swap(SpriteBatchState&& state) {
			std::swap(mShaderBinding, state.mShaderBinding);
			std::swap(mTextureVariable, state.mTextureVariable);
			std::swap(mPipeline, state.mPipeline);
		}

		SpriteBatchState(const SpriteBatchState& other) {
			CopyFrom(other);
		}

		SpriteBatchState(SpriteBatchState&& other) noexcept {
			Swap(std::move(other));
		}

		SpriteBatchState& operator=(const SpriteBatchState& other) {
			CopyFrom(other);
			return *this;
		}

		SpriteBatchState& operator=(SpriteBatchState&& other) noexcept {
			Swap(std::move(other));
			return *this;
		}

		~SpriteBatchState() {
			if (mShaderBinding) {
				mShaderBinding->Release();
				mPipeline->Release();
			}
		}

		friend class SpriteBatch;
	};

	struct SpriteBatchVSInput;

	class SpriteBatch {
	private:
		DG::IBuffer* mBuffer;
		SpriteBatchState mDefaultState;
		SpriteBatchState mCurrentState;
		DG::IDeviceContext* mCurrentContext;
		DG::ITexture* mLastTexture;

		uint mWriteIndex;
		uint mBatchSize;
		uint mBatchSizeBytes;

		DG::MapHelper<SpriteBatchVSInput> mMapHelper;

	public:
		SpriteBatch(DG::IRenderDevice* device, PipelineResource* defaultPipeline, 
			uint batchSize = DEFAULT_SPRITE_BATCH_SIZE);
		SpriteBatch(DG::IRenderDevice* device, ResourceManager* resourceManager, 
			DG::FILTER_TYPE filterType = DG::FILTER_TYPE_LINEAR, 
			uint batchSize = DEFAULT_SPRITE_BATCH_SIZE);
		SpriteBatch(DG::IRenderDevice* device, 
			uint batchSize = DEFAULT_SPRITE_BATCH_SIZE);
		~SpriteBatch();

		static PipelineResource* LoadPipeline(ResourceManager* manager, 
			DG::FILTER_TYPE filterType = DG::FILTER_TYPE_LINEAR, 
			ShaderResource* pixelShader = nullptr);
		static SpriteBatchState CreateState(PipelineResource* resource);

		inline void SetDefaultPipeline(PipelineResource* pipeline) {
			mDefaultState = CreateState(pipeline);
		}

		inline void ResetDefaultPipeline(ResourceManager* resourceManager) {
			auto pipeline = LoadPipeline(resourceManager);
			SetDefaultPipeline(pipeline);
			pipeline->Release();
		}

		void Begin(DG::IDeviceContext* context, const SpriteBatchState* state = nullptr);
		void Flush();
		void End();

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


		inline void Draw(TextureResource* texture, const DG::float3& pos,
			const DG::float2& size, const SpriteRect& rect, 
			const DG::float2& origin, const float rotation, 
			const DG::float4& color) {
			Draw(texture->GetTexture(), pos, size, rect, origin, rotation, color);
		}

		inline void Draw(TextureResource* texture, const DG::float2& pos, 
			const DG::float2& size, const SpriteRect& rect, 
			const DG::float2& origin, const float rotation, 
			const DG::float4& color) {
			Draw(texture->GetTexture(), pos, size, rect, origin, rotation, color);
		}

		inline void Draw(TextureResource* texture, const DG::float3& pos,
			const DG::float2& size,
			const DG::float2& origin, const float rotation) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos,  size, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}

		inline void Draw(TextureResource* texture, const DG::float2& pos, 
			const DG::float2& size,
			const DG::float2& origin, const float rotation) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos,  size, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}

		inline void Draw(TextureResource* texture, const DG::float3& pos,
			const DG::float2& size,
			const DG::float2& origin, const float rotation,
			const DG::float4& color) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos, size, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, color); 
		}

		inline void Draw(TextureResource* texture, const DG::float2& pos, 
			const DG::float2& size,
			const DG::float2& origin, const float rotation,
			const DG::float4& color) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos,  size, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, color); 
		}

		inline void Draw(TextureResource* texture, const DG::float3& pos) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				DG::float2(0.0, 0.0), 0.0, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(TextureResource* texture, const DG::float2& pos) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				DG::float2(0.0, 0.0), 0.0, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(TextureResource* texture, const DG::float3& pos, 
			const DG::float4& color) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				DG::float2(0.0, 0.0), 0.0, color); 
		}
		inline void Draw(TextureResource* texture, const DG::float2& pos, 
			const DG::float4& color) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				DG::float2(0.0, 0.0), 0.0, color); 
		}
		inline void Draw(TextureResource* texture, const DG::float3& pos, 
			const SpriteRect& rect, const DG::float4& color) {
			Draw(texture, pos, rect.mSize, rect,
				DG::float2(0.0, 0.0), 0.0, color); 
		}
		inline void Draw(TextureResource* texture, const DG::float2& pos, 
			const SpriteRect& rect, const DG::float4& color) {
			Draw(texture, pos, rect.mSize, rect,
				DG::float2(0.0, 0.0), 0.0, color); 
		}
		inline void Draw(TextureResource* texture, const DG::float3& pos, 
			const SpriteRect& rect) {
			Draw(texture, pos, rect.mSize, rect,
				DG::float2(0.0, 0.0), 0.0, 
				DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(TextureResource* texture, const DG::float2& pos, 
			const SpriteRect& rect) {
			Draw(texture, pos, rect.mSize, rect,
				DG::float2(0.0, 0.0), 0.0, 
				DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(TextureResource* texture, const DG::float3& pos, 
			const SpriteRect& rect, const DG::float2& origin, const float rotation) {
			Draw(texture, pos, rect.mSize, rect,
				origin, rotation, 
				DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(TextureResource* texture, const DG::float2& pos, 
			const SpriteRect& rect, const DG::float2& origin, const float rotation) {
			Draw(texture, pos, rect.mSize, rect,
				origin, rotation, 
				DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(TextureResource* texture, const DG::float3& pos, 
			const DG::float2& origin, const float rotation, const DG::float4& color) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, color); 
		}
		inline void Draw(TextureResource* texture, const DG::float2& pos, 
			const DG::float2& origin, const float rotation, const DG::float4& color) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, color); 
		}
		inline void Draw(TextureResource* texture, const DG::float3& pos, 
			const DG::float2& origin, const float rotation) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(TextureResource* texture, const DG::float2& pos, 
			const DG::float2& origin, const float rotation) {
			auto dimensions = texture->GetDimensions2D();
			Draw(texture, pos, dimensions, SpriteRect{DG::float2(0.0, 0.0), dimensions},
				origin, rotation, DG::float4(1.0, 1.0, 1.0, 1.0)); 
		}
		inline void Draw(TextureResource* texture, const DG::float3& pos,
			const SpriteRect& rect, const DG::float2& origin, 
			const float rotation, const DG::float4& color) {
			Draw(texture, pos, rect.mSize, rect,
				origin, rotation, color); 
		}
		inline void Draw(TextureResource* texture, const DG::float2& pos, 
			const SpriteRect& rect, const DG::float2& origin, 
			const float rotation, const DG::float4& color) {
			Draw(texture, pos, rect.mSize, rect,
				origin, rotation, color); 
		}
	};

	TaskId CreateSpriteBatchPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides,
		DG::FILTER_TYPE filterType,
		const AsyncResourceParams* asyncParams = nullptr,
		ShaderResource* pixelShader = nullptr);
}