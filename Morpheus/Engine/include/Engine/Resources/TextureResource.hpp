#pragma once

#include <Engine/Resources/Resource.hpp>
#include <Engine/InputController.hpp>
#include <Engine/Resources/RawTexture.hpp>

#include "EngineFactory.h"
#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "BasicMath.hpp"

#include <shared_mutex>

namespace DG = Diligent;

namespace Morpheus {

	class TextureResource : public IResource {
	private:
		DG::ITexture* mTexture;
		std::unordered_map<std::string, TextureResource*>::iterator mSource;
		bool bSourced = false;

	public:
		inline TextureResource(ResourceManager* manager, DG::ITexture* texture) :
			IResource(manager), mTexture(texture) {
		}

		inline TextureResource(ResourceManager* manager) :
			IResource(manager), mTexture(nullptr) {
		}

		~TextureResource();

		TextureResource* ToTexture() override;

		inline bool IsReady() const {
			return mTexture != nullptr;
		}

		inline DG::ITexture* GetTexture() {
			return mTexture;
		}

		entt::id_type GetType() const override {
			return resource_type::type<TextureResource>;
		}

		inline DG::float2 GetDimensions2D() const {
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

		inline bool HasSource() const {
			return bSourced;
		}

		inline void SetSource(const std::unordered_map<std::string, TextureResource*>::iterator& it) {
			bSourced = true;
			mSource = it;
		}

		void SaveGli(const std::string& path);
		void SavePng(const std::string& path, bool bSaveMips = false);

		friend class TextureLoader;
		friend class ResourceCache<TextureResource>;
	};

	void SavePng(DG::ITexture* texture, const std::string& path, 
		DG::IDeviceContext* context, DG::IRenderDevice* device, bool bSaveMips = false);
	void SaveGli(DG::ITexture* texture, const std::string& path, 
		DG::IDeviceContext* context, DG::IRenderDevice* device);

	template <>
	class ResourceCache<TextureResource> : public IResourceCache {
	private:
		std::unordered_map<std::string, TextureResource*> mResourceMap;
		std::set<TextureResource*> mResourceSet;
		ResourceManager* mManager;

		std::shared_mutex mMutex;

		TaskId ActuallyLoad(const void* params,
			const AsyncResourceParams& asyncParams,
			IResource** output);

	public:
		ResourceCache(ResourceManager* manager);
		~ResourceCache();

		TextureResource* MakeResource(DG::ITexture* texture, const std::string& source);
		TextureResource* MakeResource(DG::ITexture* texture);

		IResource* Load(const void* params) override;
		TaskId AsyncLoadDeferred(const void* params,
			ThreadPool* threadPool,
			IResource** output,
			const TaskBarrierCallback& callback = nullptr) override;
		void Add(IResource* resource, const void* params) override;
		void Unload(IResource* resource) override;
		void Clear() override;

		void Add(TextureResource* resource, const std::string& source);
		void Add(TextureResource* resource);
	};
}