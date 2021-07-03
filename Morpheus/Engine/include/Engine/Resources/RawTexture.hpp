#pragma once

#include <cmath>

#include <Engine/Resources/Resource.hpp>

#include "RenderDevice.h"
#include "DeviceContext.h"

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
		std::atomic<bool> bIsLoaded;

		Task LoadAsyncDeferred(const LoadParams<Texture>& params);
		Task LoadPngAsyncDeferred(const LoadParams<Texture>& params);
		Task LoadGliAsyncDeferred(const LoadParams<Texture>& params);
		Task LoadStbAsyncDeferred(const LoadParams<Texture>& params);
		Task LoadArchiveAsyncDeferred(const std::string& source);

		void RetrieveData(DG::ITexture* texture, DG::IRenderDevice* device, DG::IDeviceContext* context, const DG::TextureDesc& desc);

	public:
		void CopyTo(RawTexture* texture) const;
		void CopyFrom(const RawTexture& texture);

		size_t GetMipCount() const;
		void GenerateMips();

		inline size_t GetWidth() const {
			return mDesc.Width;
		}

		inline size_t GetHeight() const {
			return mDesc.Height;
		}

		inline size_t GetDepth() const {
			return mDesc.Depth;
		}

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

		inline bool IsLoaded() const {
			return bIsLoaded;
		}

		inline void SetLoaded(bool value) {
			bIsLoaded = value;
		}

		inline RawTexture() {
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
		RawTexture(const DG::TextureDesc& desc);

		inline RawTexture(const DG::TextureDesc& desc, 
			std::vector<uint8_t>&& data, 
			const std::vector<TextureSubResDataDesc>& subDescs) :
			mDesc(desc),
			mData(data),
			mSubDescs(subDescs) {
			bIsLoaded = false;
		}

		inline RawTexture(const DG::TextureDesc& desc, 
			const std::vector<uint8_t>& data, 
			const std::vector<TextureSubResDataDesc>& subDescs) :
			mDesc(desc),
			mData(data),
			mSubDescs(subDescs) {
			bIsLoaded = false;
		}

		void Set(const DG::TextureDesc& desc, std::vector<uint8_t>&& data,
			const std::vector<TextureSubResDataDesc>& subDescs) {
			mDesc = desc;
			mData = data;
			mSubDescs = subDescs;
		}

		Task LoadTask(const LoadParams<Texture>& params);
		Task LoadPngTask(const LoadParams<Texture>& params);
		Task LoadGliTask(const LoadParams<Texture>& params);
		Task LoadStbTask(const LoadParams<Texture>& params);
		Task LoadArchiveTask(const std::string& path);

		inline void Load(const LoadParams<Texture>& params) {
			LoadTask(params)();
		}

		void LoadPng(const LoadParams<Texture>& params, const uint8_t* rawData, const size_t length);
		inline void LoadPng(const LoadParams<Texture>& params) {
			LoadPngTask(params)();
		}

		void LoadGli(const LoadParams<Texture>& params, const uint8_t* rawData, const size_t length);
		inline void LoadGli(const LoadParams<Texture>& params) {
			LoadGliTask(params)();
		}

		void LoadArchive(const uint8_t* rawArchive, const size_t length);
		void LoadArchive(const std::string& source) {
			LoadArchiveTask(source)();
		}

		void LoadStb(const LoadParams<Texture>& params, const uint8_t* rawData, const size_t length);
		void LoadStb(const LoadParams<Texture>& params) {
			LoadStbTask(params)();
		}

		Task SaveTask(const std::string& path);
		inline void Save(const std::string& path) {
			SaveTask(path)();
		}
		Task SaveGliTask(const std::string& path);
		inline void SaveGli(const std::string& path) {
			SaveGliTask(path)();
		}
		Task SavePngTask(const std::string& path, bool bSaveMips = false);
		inline void SavePng(const std::string& path, bool bSaveMips = false) {
			SavePngTask(path, bSaveMips)();
		}

		void RetrieveData(DG::ITexture* texture, DG::IRenderDevice* device, DG::IDeviceContext* context);

		// Retrieves data from a GPU texture
		inline RawTexture(DG::ITexture* texture, DG::IRenderDevice* device, DG::IDeviceContext* context) {
			RetrieveData(texture, device, context);
		}

		inline RawTexture(const LoadParams<Texture>& params) {
			Load(params);
		}

		inline void Clear() {
			mData.clear();
			mSubDescs.clear();
		}

		inline RawTexture(RawTexture&& other) :
			mDesc(other.mDesc),
			mData(std::move(other.mData)),
			mSubDescs(std::move(other.mSubDescs)),
			mIntensity(other.mIntensity),
			bIsLoaded(other.bIsLoaded.load()) {
		}

		RawTexture(const RawTexture& other) = delete;

		DG::ITexture* SpawnOnGPU(DG::IRenderDevice* device) const;

		friend class RawSampler;
		friend class TextureIterator;
	};
}