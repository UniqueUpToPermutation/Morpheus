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
		// A description of the texture
		DG::TextureDesc mDesc;
		// The data of the texture, storred contiguously as byte data
		std::vector<uint8_t> mData;
		// A list of all of the texture subresources
		std::vector<TextureSubResDataDesc> mSubDescs;
		// The intensity of the texture
		float mIntensity = 1.0f;

		TaskBarrier mBarrier;

		TaskId LoadAsyncDeferred(const LoadParams<TextureResource>& params, ThreadPool* pool);
		TaskId LoadPngAsyncDeferred(const LoadParams<TextureResource>& params, ThreadPool* pool);
		TaskId LoadGliAsyncDeferred(const LoadParams<TextureResource>& params, ThreadPool* pool);
		TaskId LoadStbAsyncDeferred(const LoadParams<TextureResource>& params, ThreadPool* pool);
		TaskId LoadArchiveAsyncDeferred(const std::string& source, ThreadPool* pool);

	public:
		inline TaskBarrier* GetLoadBarrier() {
			return &mBarrier;
		}

		inline const DG::TextureDesc& GetDesc() const {
			return mDesc;
		}

		inline const std::vector<TextureSubResDataDesc>& GetSubDataDescs() const {
			return mSubDescs;
		}

		inline float GetIntensity() const {
			return mIntensity;
		}

		inline void SetIntensity(float intensity) {
			mIntensity = intensity;
		}

		inline const std::vector<uint8_t>& GetData() const {
			return mData;
		}

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

		void Load(const LoadParams<TextureResource>& params);
		void LoadAsync(const LoadParams<TextureResource>& params, ThreadPool* pool);
		void LoadPng(const LoadParams<TextureResource>& params, 
			const uint8_t* rawData, const size_t length);
		void LoadPng(const LoadParams<TextureResource>& params);
		void LoadPngAsync(const LoadParams<TextureResource>& params, ThreadPool* pool);
		void LoadGli(const LoadParams<TextureResource>& params, 
			const uint8_t* rawData, const size_t length);
		void LoadGli(const LoadParams<TextureResource>& params);
		void LoadGliAsync(const LoadParams<TextureResource>& params,
			 ThreadPool* pool);
		void LoadArchive(const uint8_t* rawArchive, const size_t length);
		void LoadArchive(const std::string& source);
		void LoadArchiveAsync(const std::string& source, ThreadPool* pool);
		void LoadStb(const LoadParams<TextureResource>& params);
		void LoadStb(const LoadParams<TextureResource>& params, const uint8_t* rawData, const size_t length);
		void LoadStbAsync(const LoadParams<TextureResource>& params, ThreadPool* pool);
		void Save(const std::string& path);
		void SaveGli(const std::string& path);
		void SavePng(const std::string& path, bool bSaveMips = false);
		void RetrieveData(DG::ITexture* texture, DG::IRenderDevice* device, DG::IDeviceContext* context);

		inline RawTexture(const LoadParams<TextureResource>& params) {
			Load(params);
		}

		inline RawTexture(const std::string& source) {
			Load(LoadParams<TextureResource>::FromString(source));
		}
		inline void Load(const std::string& source) {
			Load(LoadParams<TextureResource>::FromString(source));
		}
		inline void LoadStb(const std::string& source) {
			LoadStb(LoadParams<TextureResource>::FromString(source));
		}
		inline void LoadPng(const std::string& source) {
			LoadPng(LoadParams<TextureResource>::FromString(source));
		}
		inline void LoadGli(const std::string& source) {
			LoadGli(LoadParams<TextureResource>::FromString(source));
		}

		// Read raw texture from texture data on the GPU.
		inline RawTexture(DG::ITexture* texture, DG::IRenderDevice* device, DG::IDeviceContext* context) {
			RetrieveData(texture, device, context);
		}

		RawTexture(RawTexture&& other) = default;
		RawTexture(const RawTexture& other) = delete;

		DG::ITexture* SpawnOnGPU(DG::IRenderDevice* device);

		friend class ResourceCache<TextureResource>;
	};

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