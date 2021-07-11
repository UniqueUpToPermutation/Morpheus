#pragma once

#include <Engine/Resources/Resource.hpp>
#include <Engine/Resources/ResourceCache.hpp>
#include <Engine/Graphics.hpp>

#include "EngineFactory.h"
#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "BasicMath.hpp"

namespace DG = Diligent;

namespace Morpheus {

	template <>
	struct LoadParams<Texture> {
		std::string mSource;
		bool bIsSRGB = false;
		bool bGenerateMips = true;

		inline LoadParams(const std::string& source, 
			bool isSRGB = false, 
			bool generateMips = true) :
			mSource(source),
			bIsSRGB(isSRGB),
			bGenerateMips(generateMips) {
		}

		inline LoadParams(const char* source,
			bool isSRGB = false,
			bool generateMips = true) :
			mSource(source),
			bIsSRGB(isSRGB),
			bGenerateMips(generateMips) {	
		}

		bool operator==(const LoadParams<Texture>& t) const {
			return mSource == t.mSource && 
				bIsSRGB == t.bIsSRGB && 
				bGenerateMips == t.bGenerateMips;
		}

		struct Hasher {
			std::size_t operator()(const LoadParams<Texture>& k) const
			{
				using std::size_t;
				using std::hash;
				using std::string;

				return hash<string>()(k.mSource);
			}
		};
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

	class Texture : public IResource {
	private:
		struct RasterizerAspect {
			Handle<DG::ITexture> mTexture;
		} mRasterAspect;

		struct RawAspect {
			// A description of the texture
			DG::TextureDesc mDesc;
			// The data of the texture, storred contiguously as byte data
			std::vector<uint8_t> mData;
			// A list of all of the texture subresources
			std::vector<TextureSubResDataDesc> mSubDescs;
		} mRawAspect;

		// The intensity of the texture
		float mIntensity = 1.0f;

		TaskBarrier mBarrier;

		Task LoadAsyncDeferred(const LoadParams<Texture>& params);
		Task LoadPngAsyncDeferred(const LoadParams<Texture>& params);
		Task LoadGliAsyncDeferred(const LoadParams<Texture>& params);
		Task LoadStbAsyncDeferred(const LoadParams<Texture>& params);
		Task LoadArchiveAsyncDeferred(const std::string& source);

		void RetrieveData(DG::ITexture* texture, 
			DG::IRenderDevice* device, 
			DG::IDeviceContext* context, 
			const DG::TextureDesc& desc);

		ResourceCache<Texture, 
			LoadParams<Texture>, 
			LoadParams<Texture>::Hasher>::iterator_t mCacheIterator;

	public:
		void CreateRasterAspect(DG::IRenderDevice* device, 
			const Texture* source);
		void CreateDeviceAspect(GraphicsDevice device, 
			const Texture* source);

		inline void CreateRasterAspect(DG::IRenderDevice* device) {
			CreateRasterAspect(device, this);
		}
		inline void CreateDeviceAspect(GraphicsDevice device) {
			CreateDeviceAspect(device);
		}
		inline void To(GraphicsDevice device, Texture* out) {
			out->CreateDeviceAspect(device, this);
		}

		inline Texture To(GraphicsDevice device) {
			Texture tex;
			To(device, &tex);
			return tex;
		}

		inline Texture() {
		}

		inline Texture(DG::ITexture* texture) {
			mRasterAspect.mTexture = texture;
			mFlags |= RESOURCE_RASTERIZER_ASPECT;
		}

		inline Texture(GraphicsDevice device, const Texture* texture) {
			CreateDeviceAspect(device, texture);
		}

		void CopyTo(Texture* texture) const;
		void CopyFrom(const Texture& texture);

		size_t GetMipCount() const;
		void GenerateMips();

		inline TaskBarrier* GetLoadBarrier() {
			return &mBarrier;
		}

		inline const std::vector<TextureSubResDataDesc>& GetSubDataDescs() const {
			assert(mFlags & RESOURCE_RAW_ASPECT);

			return mRawAspect.mSubDescs;
		}

		inline float GetIntensity() const {
			return mIntensity;
		}

		inline void SetIntensity(float intensity) {
			mIntensity = intensity;
		}

		inline const std::vector<uint8_t>& GetData() const {
			assert(mFlags & RESOURCE_RAW_ASPECT);

			return mRawAspect.mData;
		}

		inline DG::ITexture* GetRasterTexture() {
			return mRasterAspect.mTexture;
		}

		inline const DG::TextureDesc& GetDesc() const {
			if (mFlags & RESOURCE_RASTERIZER_ASPECT)
				return mRasterAspect.mTexture->GetDesc();
			else
				return mRawAspect.mDesc;
		}

		inline DG::float2 GetDimensions2D() {
			auto& desc = GetDesc();
			return DG::float2((float)desc.Width, (float)desc.Height);
		}

		inline DG::float3 GetDimensions3D() const {
			auto& desc = GetDesc();
			return DG::float3((float)desc.Width, (float)desc.Height, (float)desc.Depth);
		}

		inline uint GetWidth() const {
			return GetDesc().Width;
		}

		inline uint GetHeight() const {
			return GetDesc().Height;
		}

		inline uint GetDepth() const {
			return GetDesc().Depth;
		}

		inline uint GetLevels() const {
			return GetDesc().MipLevels;
		}

		inline uint GetArraySize() const {
			return GetDesc().ArraySize;
		}

		inline DG::ITextureView* GetShaderView() {
			assert(mFlags | RESOURCE_RASTERIZER_ASPECT);

			return mRasterAspect.mTexture->GetDefaultView(DG::TEXTURE_VIEW_SHADER_RESOURCE);
		}

		// Automatically instances texture and allocates data and raw subresources
		void Alloc(const DG::TextureDesc& desc);

		void* GetSubresourcePtr(uint mip = 0, uint arrayIndex = 0);
		size_t GetSubresourceSize(uint mip = 0, uint arrayIndex = 0);

		DG::VALUE_TYPE GetComponentType() const;
		int GetComponentCount() const;
		bool GetIsSRGB() const;
		size_t GetPixelByteSize() const;
		
		// Automatically instances texture and allocates data and raw subresources
		Texture(const DG::TextureDesc& desc);

		void Set(const DG::TextureDesc& desc, std::vector<uint8_t>&& data,
			const std::vector<TextureSubResDataDesc>& subDescs) {
			mFlags |= RESOURCE_RAW_ASPECT;
			mRawAspect.mDesc = desc;
			mRawAspect.mData = std::move(data);
			mRawAspect.mSubDescs = subDescs;
		}

		inline Texture(const DG::TextureDesc& desc, 
			std::vector<uint8_t>&& data, 
			const std::vector<TextureSubResDataDesc>& subDescs) {
			Set(desc, std::move(data), subDescs);
		}

		Task LoadRawTask(const LoadParams<Texture>& params);
		Task LoadPngRawTask(const LoadParams<Texture>& params);
		Task LoadGliRawTask(const LoadParams<Texture>& params);
		Task LoadStbRawTask(const LoadParams<Texture>& params);
		Task LoadArchiveRawTask(const std::string& path);

		inline void LoadRaw(const LoadParams<Texture>& params) {
			LoadRawTask(params)();
		}

		void LoadPngRaw(const LoadParams<Texture>& params, 
			const uint8_t* rawData, 
			const size_t length);

		inline void LoadPngRaw(const LoadParams<Texture>& params) {
			LoadPngRawTask(params)();
		}

		void LoadGliRaw(const LoadParams<Texture>& params, 
			const uint8_t* rawData, 
			const size_t length);

		inline void LoadGliRaw(const LoadParams<Texture>& params) {
			LoadGliRawTask(params)();
		}

		void LoadArchiveRaw(const uint8_t* rawArchive, 
			const size_t length);

		void LoadArchiveRaw(const std::string& source) {
			LoadArchiveRawTask(source)();
		}

		void LoadStbRaw(const LoadParams<Texture>& params, 
			const uint8_t* rawData, 
			const size_t length);

		void LoadStbRaw(const LoadParams<Texture>& params) {
			LoadStbRawTask(params)();
		}

		Task SaveTask(const std::string& path);

		inline void Save(const std::string& path) {
			SaveTask(path)();
		}

		Task SaveGliTask(const std::string& path);

		inline void SaveGli(const std::string& path) {
			SaveGliTask(path)();
		}

		Task SavePngTask(const std::string& path, 
			bool bSaveMips = false);

		inline void SavePng(const std::string& path, 
			bool bSaveMips = false) {
			SavePngTask(path, bSaveMips)();
		}

		void RetrieveData(DG::ITexture* texture, 
			DG::IRenderDevice* device, 
			DG::IDeviceContext* context);

		inline Texture(const LoadParams<Texture>& params) {
			LoadRaw(params);
		}

		inline void Clear() {
			mRawAspect.mData.clear();
			mRawAspect.mSubDescs.clear();
		}

		void AdoptData(Texture&& other);

		Texture(Texture&& other);
		Texture(const Texture& other) = delete;

		DG::ITexture* SpawnOnGPU(DG::IRenderDevice* device) const;

		static ResourceTask<Texture*> Load(
			GraphicsDevice device, 
			const LoadParams<Texture>& params);
		static ResourceTask<Handle<Texture>> LoadHandle(
			GraphicsDevice device, 
			const LoadParams<Texture>& params);
		static ResourceTask<Texture*> Load(
			const LoadParams<Texture>& params);
		static ResourceTask<Handle<Texture>> LoadHandle(
			const LoadParams<Texture>& params);
		
		typedef LoadParams<Texture> LoadParameters;

		friend class TextureIterator;
		friend class RawSampler;
	};
}