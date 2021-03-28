#pragma once

#include <Engine/SpriteBatch.hpp>
#include <Engine/Resources/Resource.hpp>
#include <Engine/Engine2D/Base2D.hpp>

namespace DG = Diligent;
namespace Morpheus {
	struct SpriteComponent {
		TextureResource* mTextureResource;
		RenderLayerId mRenderLayerId;
		DG::float2 mOrigin;
		DG::float4 mColor;
		SpriteRect mRect;
	
		inline SpriteComponent(TextureResource* texture) : 
			mTextureResource(texture),
			mOrigin(0.0f, 0.0f),
			mColor(1.0f, 1.0f, 1.0f, 1.0f),
			mRect(0.0f, 0.0f, texture->GetWidth(), texture->GetHeight()),
			mRenderLayerId(RENDER_LAYER_NONE) {
			texture->AddRef();
		}

		inline SpriteComponent(const SpriteComponent& other) :
			mTextureResource(other.mTextureResource),
			mOrigin(other.mOrigin),
			mColor(other.mColor),
			mRect(other.mRect),
			mRenderLayerId(other.mRenderLayerId) {
			mTextureResource->AddRef();
		}

		inline SpriteComponent(SpriteComponent&& other) : 
			mTextureResource(other.mTextureResource),
			mOrigin(other.mOrigin),
			mColor(other.mColor),
			mRect(other.mRect),
			mRenderLayerId(other.mRenderLayerId) {
			mTextureResource->AddRef();
		}

		inline SpriteComponent& operator= (const SpriteComponent& other) {
			if (mTextureResource)
				mTextureResource->Release();
			
			mTextureResource = other.mTextureResource;
			mOrigin = other.mOrigin;
			mColor = other.mColor;
			mRect = other.mRect;
			mRenderLayerId = other.mRenderLayerId;

			mTextureResource->AddRef();
			return *this;
		}

		inline SpriteComponent& operator= (SpriteComponent&& other) {
			if (mTextureResource)
				mTextureResource->Release();
			
			mTextureResource = other.mTextureResource;
			mOrigin = other.mOrigin;
			mColor = other.mColor;
			mRect = other.mRect;
			mRenderLayerId = other.mRenderLayerId;

			mTextureResource->AddRef();
			return *this;
		}

		inline ~SpriteComponent() {
			if (mTextureResource)
				mTextureResource->Release();
		}
	};
}