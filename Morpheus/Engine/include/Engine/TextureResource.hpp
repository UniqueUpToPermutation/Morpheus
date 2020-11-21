#pragma once

#include <Engine/Resource.hpp>

#include "EngineFactory.h"
#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "InputController.hpp"
#include "BasicMath.hpp"

namespace DG = Diligent;

namespace Morpheus {
	class TextureResource : public Resource {
	private:
		DG::ITexture* mTexture;
		std::string mSource;

	public:
		inline TextureResource(ResourceManager* manager, DG::ITexture* texture) :
			Resource(manager), mTexture(texture) {
		}

		inline TextureResource(ResourceManager* manager) :
			Resource(manager), mTexture(nullptr) {
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

		friend class TextureLoader;
		friend class ResourceCache<TextureResource>;
	};

	template <>
	struct LoadParams<TextureResource> {
		std::string mSource;

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

		void LoadGli(const std::string& source, TextureResource* texture);
		void LoadDiligent(const std::string& source, TextureResource* texture);
		void Load(const std::string& source, TextureResource* texture);
	};

	template <>
	class ResourceCache<TextureResource> : public IResourceCache {
	private:
		std::unordered_map<std::string, TextureResource*> mResources;
		ResourceManager* mManager;
		TextureLoader mLoader;
		std::vector<std::pair<TextureResource*, LoadParams<TextureResource>>> mDeferredResources;

	public:
		ResourceCache(ResourceManager* manager);
		~ResourceCache();

		Resource* Load(const void* params) override;
		Resource* DeferredLoad(const void* params) override;
		void ProcessDeferred() override;
		void Add(Resource* resource, const void* params) override;
		void Unload(Resource* resource) override;
		void Clear() override;
	};
}