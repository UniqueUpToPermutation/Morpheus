#pragma once

#include <Engine/Resources/Resource.hpp>
#include <Engine/InputController.hpp>

#include "EngineFactory.h"
#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "BasicMath.hpp"

namespace DG = Diligent;

namespace Morpheus {

	inline uint MipCount(const uint width, const uint height) {
		return 1 + std::floor(std::log2(std::max(width, height)));
	}

	inline uint MipCount(const uint width, const uint height, const uint depth) {
		return 1 + std::floor(std::log2(std::max(width, std::max(height, depth))));
	}

	class TextureResource : public IResource {
	private:
		DG::ITexture* mTexture;
		std::string mSource;

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

		inline std::string GetSource() const {
			return mSource;
		}

		inline DG::ITextureView* GetShaderView() {
			return mTexture->GetDefaultView(DG::TEXTURE_VIEW_SHADER_RESOURCE);
		}

		void SaveGli(const std::string& path);
		void SavePng(const std::string& path);

		friend class TextureLoader;
		friend class ResourceCache<TextureResource>;
	};

	void SavePng(DG::ITexture* texture, const std::string& path, 
		DG::IDeviceContext* context, DG::IRenderDevice* device);
	void SaveGli(DG::ITexture* texture, const std::string& path, 
		DG::IDeviceContext* context, DG::IRenderDevice* device);

	template <>
	struct LoadParams<TextureResource> {
		std::string mSource;
		bool bIsSRGB = false;
		bool bGenerateMips = true;

		static LoadParams<TextureResource> FromString(const std::string& str) {
			LoadParams<TextureResource> res;
			res.mSource = str;
			return res;
		}
	};

	class TextureLoader {
	private:
		ResourceManager* mManager;

	public:
		TextureLoader(ResourceManager* manager);

		void LoadGli(const LoadParams<TextureResource>& params, TextureResource* texture);
		void LoadDiligent(const LoadParams<TextureResource>& params, TextureResource* texture);
		void LoadStb(const LoadParams<TextureResource>& params, TextureResource* texture);
		void Load(const LoadParams<TextureResource>& params, TextureResource* texture);
	};

	template <>
	class ResourceCache<TextureResource> : public IResourceCache {
	private:
		std::unordered_map<std::string, TextureResource*> mResourceMap;
		std::set<TextureResource*> mResourceSet;
		ResourceManager* mManager;
		TextureLoader mLoader;
		std::vector<std::pair<TextureResource*, LoadParams<TextureResource>>> mDeferredResources;

	public:
		ResourceCache(ResourceManager* manager);
		~ResourceCache();

		TextureResource* MakeResource(DG::ITexture* texture, const std::string& source);
		TextureResource* MakeResource(DG::ITexture* texture);

		IResource* Load(const void* params) override;
		IResource* DeferredLoad(const void* params) override;
		void ProcessDeferred() override;
		void Add(IResource* resource, const void* params) override;
		void Unload(IResource* resource) override;
		void Clear() override;

		void Add(TextureResource* resource, const std::string& source);
		void Add(TextureResource* resource);
	};
}