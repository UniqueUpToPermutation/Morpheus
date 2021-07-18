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

	inline uint MipCount(
		const uint width, 
		const uint height) {
		return (uint)(1 + std::floor(std::log2(std::max(width, height))));
	}

	inline uint MipCount(
		const uint width, 
		const uint height, 
		const uint depth) {
		return (uint)(1 + std::floor(
			std::log2(std::max(width, std::max(height, depth)))));
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

		ExternalAspect<ExtObjectType::TEXTURE> mExtAspect;

		// The intensity of the texture
		float mIntensity = 1.0f;

		Task ReadAsyncDeferred(const LoadParams<Texture>& params);
		Task ReadPngAsyncDeferred(const LoadParams<Texture>& params);
		Task ReadGliAsyncDeferred(const LoadParams<Texture>& params);
		Task ReadStbAsyncDeferred(const LoadParams<Texture>& params);
		Task ReadArchiveAsyncDeferred(const std::string& source);

		void RetrieveRawData(DG::ITexture* texture, 
			DG::IRenderDevice* device, 
			DG::IDeviceContext* context, 
			const DG::TextureDesc& desc);

		ResourceCache<Texture, 
			LoadParams<Texture>, 
			LoadParams<Texture>::Hasher>::iterator_t mCacheIterator;

	public:
		// -------------------------------------------------------------
		// Texture Aspects
		// -------------------------------------------------------------

		void CreateExternalAspect(IExternalGraphicsDevice* device,
			const Texture* source);
			
		inline void CreateExternalAspect(IExternalGraphicsDevice* device) {
			CreateExternalAspect(device, this);
		}

		void CreateRasterAspect(DG::IRenderDevice* device, 
			const Texture* source);
		void CreateRasterAspect(DG::ITexture* texture);
		void CreateDeviceAspect(GraphicsDevice device, 
			const Texture* source);

		inline void CreateRasterAspect(DG::IRenderDevice* device) {
			CreateRasterAspect(device, this);
		}
		inline void CreateDeviceAspect(GraphicsDevice device) {
			CreateDeviceAspect(device);
		}

		// -------------------------------------------------------------
		// Texture IO
		// -------------------------------------------------------------

		Task ReadTask(const LoadParams<Texture>& params);
		Task ReadPngTask(const LoadParams<Texture>& params);
		Task ReadGliTask(const LoadParams<Texture>& params);
		Task ReadStbTask(const LoadParams<Texture>& params);
		Task ReadArchiveTask(const std::string& path);

		inline void Read(const LoadParams<Texture>& params) {
			ReadTask(params)();
		}

		void ReadPng(const LoadParams<Texture>& params, 
			const uint8_t* rawData, 
			const size_t length);

		inline void ReadPng(const LoadParams<Texture>& params) {
			ReadPngTask(params)();
		}

		void ReadGli(const LoadParams<Texture>& params, 
			const uint8_t* rawData, 
			const size_t length);

		inline void ReadGli(const LoadParams<Texture>& params) {
			ReadGliTask(params)();
		}

		void ReadArchive(const uint8_t* rawArchive, 
			const size_t length);

		void ReadArchive(const std::string& source) {
			ReadArchiveTask(source)();
		}

		void ReadStb(const LoadParams<Texture>& params, 
			const uint8_t* rawData, 
			const size_t length);

		void ReadStb(const LoadParams<Texture>& params) {
			ReadStbTask(params)();
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

		void RetrieveRawData(DG::ITexture* texture, 
			DG::IRenderDevice* device, 
			DG::IDeviceContext* context);

		static ResourceTask<Texture*> LoadPointer(
			GraphicsDevice device, 
			const LoadParams<Texture>& params);
		static ResourceTask<Handle<Texture>> Load(
			GraphicsDevice device, 
			const LoadParams<Texture>& params);
		static ResourceTask<Texture*> LoadPointer(
			const LoadParams<Texture>& params);
		static ResourceTask<Handle<Texture>> Load(
			const LoadParams<Texture>& params);
		

		// -------------------------------------------------------------
		// Texture Constructors / Creation
		// -------------------------------------------------------------

		inline Texture() {
		}

		inline Texture(DG::ITexture* texture) {
			CreateRasterAspect(texture);
		}

		// Automatically instances texture and allocates data and raw subresources
		Texture(const DG::TextureDesc& desc);

		inline void Set(const DG::TextureDesc& desc, std::vector<uint8_t>&& data,
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

		inline Texture(const LoadParams<Texture>& params) {
			Read(params);
		}

		Texture(Texture&& other);
		Texture(const Texture& other) = delete;

		Texture& operator=(Texture&& other);
		Texture& operator=(const Texture& other) = delete;

		inline Texture(GraphicsDevice device, const Texture* texture) {
			CreateDeviceAspect(device, texture);
		}

		inline void To(GraphicsDevice device, Texture* out) {
			out->CreateDeviceAspect(device, this);
		}

		inline Texture To(GraphicsDevice device) {
			Texture tex;
			To(device, &tex);
			return tex;
		}

		void ToRaw(Texture* out) const;
		void ToRaw(Texture* out, DG::IRenderDevice* device, DG::IDeviceContext* context) const;
		
		inline Texture ToRaw() const {
			Texture texture;
			ToRaw(&texture);
			return texture;
		}

		inline Texture ToRaw(DG::IRenderDevice* device, DG::IDeviceContext* context) const {
			Texture texture;
			ToRaw(&texture, device, context);
			return texture;
		}

		void CopyTo(Texture* texture) const;
		void CopyFrom(const Texture& texture);

		size_t GetMipCount() const;
		void GenerateMips();

		// Automatically instances texture and allocates data and raw subresources
		void AllocRaw(const DG::TextureDesc& desc);
		void Clear();

		void AdoptData(Texture&& other);

		DG::ITexture* SpawnOnGPU(DG::IRenderDevice* device) const;

		// -------------------------------------------------------------
		// Texture Properties
		// -------------------------------------------------------------

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

		inline DG::ITexture* GetRasterTexture() const {
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
			return DG::float3((float)desc.Width, 
				(float)desc.Height, 
				(float)desc.Depth);
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
			assert(IsRasterResource());
			return mRasterAspect.mTexture->GetDefaultView(
				DG::TEXTURE_VIEW_SHADER_RESOURCE);
		}

		void* GetSubresourcePtr(uint mip = 0, uint arrayIndex = 0);
		size_t GetSubresourceSize(uint mip = 0, uint arrayIndex = 0);

		DG::VALUE_TYPE GetComponentType() const;
		int GetComponentCount() const;
		bool GetIsSRGB() const;
		size_t GetPixelByteSize() const;

		inline operator bool() const {
			return mFlags != 0u;
		}

		typedef LoadParams<Texture> LoadParameters;

		friend class TextureIterator;
		friend class RawSampler;
	};
}