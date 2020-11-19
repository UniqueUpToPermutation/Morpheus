#include <Engine/TextureResource.hpp>
#include <Engine/ResourceManager.hpp>
#include <Engine/Engine.hpp>

#include "TextureUtilities.h"

#include <ktx.h>

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
		auto pos = source.rfind('.');
		if (pos == std::string::npos) {
			throw std::runtime_error("Source does not have file extension!");
		}
		auto ext = source.substr(pos);

		if (ext == ".ktx") {
			LoadKtx(source, resource);
		} else {
			LoadDiligent(source, resource);
		}
	}

	void TextureLoader::LoadKtx(const std::string& source, TextureResource* resource) {
		ktxTexture* texture;
		KTX_error_code result;
		ktx_size_t offset;
		ktx_uint8_t* image;
		ktx_uint32_t level, layer, faceSlice;
		result = ktxTexture_CreateFromNamedFile(source.c_str(),
												KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
												&texture);

		ktx_uint32_t numLevels = texture->numLevels;
		ktx_uint32_t baseWidth = texture->baseWidth;
		ktx_bool_t isArray = texture->isArray;

		char* pValue;
		uint32_t valueLen;
		if (KTX_SUCCESS == ktxHashList_FindValue(&texture->kvDataHead,
												KTX_ORIENTATION_KEY,
												&valueLen, (void**)&pValue))
		{
			char s, t;
			if (sscanf(pValue, KTX_ORIENTATION2_FMT, &s, &t) == 2) {

			}
		}


		level = 1; layer = 0; faceSlice = 3;
		result = ktxTexture_GetImageOffset(texture, level, layer, faceSlice, &offset);
		image = ktxTexture_GetData(texture) + offset;
		// ...
		// Do something with the texture image.
		// ...
		ktxTexture_Destroy(texture);
	}

	void TextureLoader::LoadDiligent(const std::string& source, TextureResource* texture) {
		DG::TextureLoadInfo loadInfo;
		loadInfo.IsSRGB = true;
		DG::ITexture* tex = nullptr;

		std::cout << "Loading " << source << "..." << std::endl;

		CreateTextureFromFile(source.c_str(), loadInfo, mManager->GetParent()->GetDevice(), &tex);
		
		texture->mTexture = tex;
		texture->mSource = source;
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