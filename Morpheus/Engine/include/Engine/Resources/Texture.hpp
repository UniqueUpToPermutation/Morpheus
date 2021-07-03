#pragma once

#include <Engine/Resources/Resource.hpp>
#include <Engine/Resources/RawTexture.hpp>
#include <Engine/Resources/ResourceCache.hpp>

#include "EngineFactory.h"
#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "BasicMath.hpp"

namespace DG = Diligent;

namespace Morpheus {

	class Texture : public IResource {
	private:
		DG::ITexture* mTexture = nullptr;
		ResourceManagement mManagementScheme;

		ResourceCache<Texture, 
			LoadParams<Texture>, 
			LoadParams<Texture>::Hasher>::iterator_t mCacheIterator;

	public:
		inline Texture() {
		}

		inline Texture(DG::ITexture* texture) : mTexture(texture), 
			mManagementScheme(ResourceManagement::INTERNAL_UNMANAGED) {
		}

		inline Texture(DG::IRenderDevice* device, const RawTexture* texture) :
			mManagementScheme(ResourceManagement::INTERNAL_UNMANAGED) {
			texture->SpawnOnGPU(device);
		}

		inline Texture(DG::IRenderDevice* device, const RawTexture& texture) :
			Texture(device, &texture) {	
		}

		inline ~Texture() {
			if (mTexture)
				mTexture->Release();
		}

		inline void SetManagementScheme(ResourceManagement manag) { 
			mManagementScheme = manag;
		}

		inline ResourceManagement GetManagementScheme() const {
			return mManagementScheme;
		}

		inline bool IsManaged() const {
			return mManagementScheme == ResourceManagement::FROM_DISK_MANAGED ||
				mManagementScheme == ResourceManagement::INTERNAL_MANAGED;
		}

		inline DG::ITexture* Get() {
			return mTexture;
		}

		inline const DG::TextureDesc& GetDesc() const {
			return mTexture->GetDesc();
		}

		inline DG::float2 GetDimensions2D() {
			auto& desc = mTexture->GetDesc();
			return DG::float2((float)desc.Width, (float)desc.Height);
		}

		inline DG::float3 GetDimensions3D() const {
			auto& desc = mTexture->GetDesc();
			return DG::float3((float)desc.Width, (float)desc.Height, (float)desc.Depth);
		}

		inline uint GetWidth() const {
			return mTexture->GetDesc().Width;
		}

		inline uint GetHeight() const {
			return mTexture->GetDesc().Height;
		}

		inline uint GetDepth() const {
			return mTexture->GetDesc().Depth;
		}

		inline uint GetLevels() const {
			return mTexture->GetDesc().MipLevels;
		}

		inline uint GetArraySize() const {
			return mTexture->GetDesc().ArraySize;
		}

		inline DG::ITextureView* GetShaderView() {
			return mTexture->GetDefaultView(DG::TEXTURE_VIEW_SHADER_RESOURCE);
		}

		typedef LoadParams<Texture> LoadParameters;

		static ResourceTask<Texture*> Load(DG::IRenderDevice* device, const LoadParams<Texture>& params);
		static ResourceTask<Handle<Texture>> LoadHandle(DG::IRenderDevice* device, const LoadParams<Texture>& params);
	};
}