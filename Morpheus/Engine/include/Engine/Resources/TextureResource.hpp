#pragma once

#include <Engine/Resources/Resource.hpp>
#include <Engine/InputController.hpp>

#include "EngineFactory.h"
#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "BasicMath.hpp"

#include <shared_mutex>

namespace DG = Diligent;

namespace Morpheus {

	inline uint MipCount(const uint width, const uint height) {
		return (uint)(1 + std::floor(std::log2(std::max(width, height))));
	}

	inline uint MipCount(const uint width, const uint height, const uint depth) {
		return (uint)(1 + std::floor(std::log2(std::max(width, std::max(height, depth)))));
	}

	struct TextureSubResDataDesc {
		DG::Uint32 mDepthStride;
		DG::Uint32 mSrcOffset;
		DG::Uint32 mStride;
	};

	class RawTexture {
	private:
		DG::TextureDesc mDesc;
		std::vector<uint8_t> mData;
		std::vector<TextureSubResDataDesc> mSubDescs;

	public:
		inline RawTexture() {
		}

		inline RawTexture(const DG::TextureDesc& desc, 
			std::vector<uint8_t>&& data, 
			const std::vector<TextureSubResDataDesc>& subDescs) :
			mDesc(desc),
			mData(data),
			mSubDescs(subDescs) {
		}

		inline RawTexture(const DG::TextureDesc& desc, 
			const std::vector<uint8_t>& data, 
			const std::vector<TextureSubResDataDesc>& subDescs) :
			mDesc(desc),
			mData(data),
			mSubDescs(subDescs) {
		}

		void Set(const DG::TextureDesc& desc, std::vector<uint8_t>&& data,
			const std::vector<TextureSubResDataDesc>& subDescs) {
			mDesc = desc;
			mData = data;
			mSubDescs = subDescs;
		}

		RawTexture(RawTexture&& other) = default;
		RawTexture(const RawTexture& other) = delete;

		DG::ITexture* SpawnOnGPU(DG::IRenderDevice* device);
	};

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

		TaskId LoadGli(const LoadParams<TextureResource>& params, TextureResource* texture,
			const AsyncResourceParams& asyncParams);
		TaskId LoadStb(const LoadParams<TextureResource>& params, TextureResource* texture,
			const AsyncResourceParams& asyncParams);
		TaskId LoadPng(const LoadParams<TextureResource>& params, TextureResource* texture,
			const AsyncResourceParams& asyncParams);
		TaskId Load(const LoadParams<TextureResource>& params, TextureResource* texture,
			const AsyncResourceParams& asyncParams);
	};

	template <>
	class ResourceCache<TextureResource> : public IResourceCache {
	private:
		std::unordered_map<std::string, TextureResource*> mResourceMap;
		std::set<TextureResource*> mResourceSet;
		ResourceManager* mManager;
		TextureLoader mLoader;

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