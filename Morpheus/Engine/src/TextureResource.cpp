#include <Engine/TextureResource.hpp>
#include <Engine/ResourceManager.hpp>
#include <Engine/Engine.hpp>

#include "TextureUtilities.h"

namespace Morpheus {
	TextureResource* TextureResource::ToTexture() {
		return this;
	}

	TextureResource::~TextureResource() {
		mTexture->Release();	
	}

	TextureLoader::TextureLoader(ResourceManager* manager) : mManager(manager) {
	}

	void TextureLoader::Load(const std::string& source, TextureResource* resource) {
		DG::TextureLoadInfo loadInfo;
		loadInfo.IsSRGB = true;
		DG::ITexture* tex = nullptr;

		std::cout << "Loading " << source << "..." << std::endl;

		CreateTextureFromFile(source.c_str(), loadInfo, mManager->GetParent()->GetDevice(), &tex);
		
		resource->mTexture = tex;
		resource->mSource = source;
	}

	ResourceCache<TextureResource>::ResourceCache(ResourceManager* manager) :
		mManager(manager), mLoader(manager) {
	}

	ResourceCache<TextureResource>::~ResourceCache() {
		Clear();
	}

	Resource* ResourceCache<TextureResource>::Load(const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<TextureResource>*>(params);
		
		auto it = mResources.find(params_cast->mSource);

		if (it != mResources.end()) {
			return it->second;
		} else {
			auto result = new TextureResource(mManager);
			mLoader.Load(params_cast->mSource, result);
			mResources[params_cast->mSource] = result;
			return result;
		}
	}

	Resource* ResourceCache<TextureResource>::DeferredLoad(const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<TextureResource>*>(params);
		
		auto it = mResources.find(params_cast->mSource);

		if (it != mResources.end()) {
			return it->second;
		} else {
			auto result = new TextureResource(mManager);
			mDeferredResources.emplace_back(std::make_pair(result, *params_cast));
			mResources[params_cast->mSource] = result;
			return result;
		}
	}

	void ResourceCache<TextureResource>::ProcessDeferred() {
		for (auto resource : mDeferredResources) {
			mLoader.Load(resource.second.mSource, resource.first);
		}

		mDeferredResources.clear();
	}

	void ResourceCache<TextureResource>::Add(Resource* resource, const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<TextureResource>*>(params);
		
		auto it = mResources.find(params_cast->mSource);

		auto geometryResource = resource->ToTexture();

		if (it != mResources.end()) {
			if (it->second != geometryResource)
				Unload(it->second);
			else
				return;
		} 

		mResources[params_cast->mSource] = geometryResource;
	}

	void ResourceCache<TextureResource>::Unload(Resource* resource) {
		auto tex = resource->ToTexture();

		auto it = mResources.find(tex->GetSource());
		if (it != mResources.end()) {
			if (it->second == tex) {
				mResources.erase(it);
			}
		}

		delete resource;
	}

	void ResourceCache<TextureResource>::Clear() {
		for (auto& item : mResources) {
			item.second->ResetRefCount();
			delete item.second;
		}

		mResources.clear();
	}
}