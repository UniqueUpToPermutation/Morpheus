#pragma once

#include <Engine/Resources/Resource.hpp>
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
		std::filesystem::path mPath;
		bool bIsSRGB = false;
		bool bGenerateMips = true;
		
		// Not necessary - but loading will be faster if
		// this is provided.
		ArchiveLoad mArchiveLoad;
		
		inline LoadParams() {
		}

		inline LoadParams(const std::filesystem::path& path, 
			bool isSRGB = false, 
			bool generateMips = true) :
			mPath(path),
			bIsSRGB(isSRGB),
			bGenerateMips(generateMips) {
		}

		inline LoadParams(const std::filesystem::path& path,
			const ArchiveBlobPointer& archivePosition) :
			mPath(path) {
		}

		bool operator==(const LoadParams<Texture>& t) const {
			return mPath == t.mPath;
		}

		template <class Archive>
		void save(Archive& archive) const {
			std::save<Archive>(archive, mPath);
			archive(bGenerateMips);
			archive(bIsSRGB);
		}

		template <class Archive>
		void load(Archive& archive) {
			std::load<Archive>(archive, mPath);
			archive(bGenerateMips);
			archive(bIsSRGB);
		}
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

		template <class Archive>
		void serialize(Archive& archive) {
			archive(mDepthStride);
			archive(mSrcOffset);
			archive(mStride);
		}
	};

	struct GPUTextureRead {
		Handle<DG::IFence> mFence;
		Handle<DG::ITexture> mStagingTexture;
		DG::TextureDesc mTextureDesc;
		DG::Uint64 mFenceCompletedValue;

		inline bool IsReady() const {
			return mFence->GetCompletedValue() == mFenceCompletedValue;
		}
	};

	class Texture : public IResource {
	private:
		LoadParams<Texture> mSource;

		struct RasterizerAspect {
			Handle<DG::ITexture> mTexture;
		} mRasterAspect;

		struct CpuAspect {
			// A description of the texture
			DG::TextureDesc mDesc;
			// The data of the texture, storred contiguously as byte data
			std::vector<uint8_t> mData;
			// A list of all of the texture subresources
			std::vector<TextureSubResDataDesc> mSubDescs;
		} mCpuAspect;

		// The intensity of the texture
		float mIntensity = 1.0f;

		Task ReadAsyncDeferred(const LoadParams<Texture>& params);
		Task ReadPngAsyncDeferred(const LoadParams<Texture>& params);
		Task ReadGliAsyncDeferred(const LoadParams<Texture>& params);

		// -------------------------------------------------------------
		// Texture Aspects
		// -------------------------------------------------------------

		void CreateRasterAspect(DG::IRenderDevice* device, 
			const Texture* source);
		void CreateRasterAspect(DG::IRenderDevice* device,
			DG::ITexture* texture);
		void CreateDeviceAspect(Device device, 
			const Texture* source);

		inline void CreateRasterAspect(DG::IRenderDevice* device) {
			CreateRasterAspect(device, this);
		}

	public:
		// -------------------------------------------------------------
		// Texture IO
		// -------------------------------------------------------------

		inline static Texture CopyToDevice(Device device, const Texture& in) {
			Texture tex;
			tex.CreateDeviceAspect(device, &in);
			return tex;
		}

		static UniqueFuture<Texture> ReadAsync(const LoadParams<Texture>& params);
		static UniqueFuture<Texture> ReadPngAsync(const LoadParams<Texture>& params);
		static UniqueFuture<Texture> ReadGliAsync(const LoadParams<Texture>& params);
		static UniqueFuture<Texture> ReadStbAsync(const LoadParams<Texture>& params);
		static UniqueFuture<Texture> ReadFrameAsync(const LoadParams<Texture>& params);

		static Texture ReadPng(const LoadParams<Texture>& params, 
			const uint8_t* rawData, 
			const size_t length);

		static Texture ReadGli(const LoadParams<Texture>& params, 
			const uint8_t* rawData, 
			const size_t length);

		static Texture ReadStb(const LoadParams<Texture>& params, 
			const uint8_t* rawData, 
			const size_t length);

		BarrierOut SaveGliAsync(const std::string& path);
		BarrierOut SavePngAsync(const std::string& path, 
			bool bSaveMips = false);

		static UniqueFuture<Texture> Load(
			Device device,
			const LoadParams<Texture>& params);

		static UniqueFuture<Texture> Load(
			const LoadParams<Texture>& params);

		static Future<Handle<Texture>> LoadHandle(
			Device device,
			const LoadParams<Texture>& params);

		static Future<Handle<Texture>> LoadHandle(
			const LoadParams<Texture>& params);

		inline static Texture Read(const LoadParams<Texture>& params) {
			return std::move(ReadAsync(params).Evaluate());
		}

		inline static Texture ReadPng(const LoadParams<Texture>& params) {
			return std::move(ReadPngAsync(params).Evaluate());
		}

		inline static Texture ReadGli(const LoadParams<Texture>& params) {
			return std::move(ReadGliAsync(params).Evaluate());
		}

		inline static Texture ReadStb(const LoadParams<Texture>& params) {
			return std::move(ReadStbAsync(params).Evaluate());
		}

		inline void SaveGli(const std::string& path) {
			SaveGliAsync(path).Evaluate();
		}

		inline void SavePng(const std::string& path, 
			bool bSaveMips = false) {
			SavePngAsync(path, bSaveMips).Evaluate();
		}

		// -------------------------------------------------------------
		// Texture Constructors / Creation
		// -------------------------------------------------------------

		inline Texture() {
		}
		
		inline Texture(DG::IRenderDevice* device, DG::ITexture* texture) {
			CreateRasterAspect(device, texture);
		}

		// Create an internal resource texture that is stored on disk in an archive
		Texture(Handle<Frame> frame, 
			entt::entity entity);

		// Automatically instances texture and allocates data and raw subresources on CPU
		Texture(const DG::TextureDesc& desc);
		Texture(Device device, const DG::TextureDesc& desc);

		inline void Set(const DG::TextureDesc& desc, std::vector<uint8_t>&& data,
			const std::vector<TextureSubResDataDesc>& subDescs) {
			mDevice = Device::CPU();
			mCpuAspect.mDesc = desc;
			mCpuAspect.mData = std::move(data);
			mCpuAspect.mSubDescs = subDescs;
		}

		inline Texture(const DG::TextureDesc& desc, 
			std::vector<uint8_t>&& data, 
			const std::vector<TextureSubResDataDesc>& subDescs) {
			Set(desc, std::move(data), subDescs);
		}

		inline Texture(const LoadParams<Texture>& params) {
			mDevice = Device::Disk();
			mSource = params;
		}

		inline Texture(const std::string& path) : 
			Texture(LoadParams<Texture>(std::filesystem::path(path))) {
		}

		inline Texture(Device device, 
			const LoadParams<Texture>& params, 
			Context context = Context()) : Texture(params) {
			Move(device, context);
		}

		inline Texture(Device device,
			const std::string& path,
			Context context = Context()) : 
			Texture(device, LoadParams<Texture>(std::filesystem::path(path)), context) {
		}

		inline Texture(Device device, const Texture* texture) {
			CreateDeviceAspect(device, texture);
		}

		Texture(Texture&& other) = default;
		Texture(const Texture& other) = delete;

		Texture& operator=(Texture&& other) = default;
		Texture& operator=(const Texture& other) = delete;

		Texture To(Device device, Context context = Context());
		BarrierOut MoveAsync(Device device, Context context = Context()) override;
		UniqueFuture<Texture> ToAsync(Device device, Context context = Context());
		UniqueFuture<Texture> GPUToCPUAsync(Device device, Context context) const;

		std::filesystem::path GetPath() const override;
		entt::meta_type GetType() const override;
		entt::meta_any GetSourceMeta() const override;

		inline const LoadParams<Texture>& GetSource() const {
			return mSource;
		}

		void BinarySerialize(std::ostream& output, IDependencyResolver* dependencies) override;
		void BinaryDeserialize(std::istream& input, const IDependencyResolver* dependencies) override;
		void BinarySerializeReference(
			const std::filesystem::path& workingPath, 
			cereal::PortableBinaryOutputArchive& output) override;
		void BinaryDeserializeReference(
			const std::filesystem::path& workingPath,
			cereal::PortableBinaryInputArchive& input) override;

		void CopyTo(Texture* texture) const;
		void CopyFrom(const Texture& texture);

		size_t GetMipCount() const;
		void GenerateMips();

		// Automatically instances texture and allocates data and raw subresources
		void AllocOnCPU(const DG::TextureDesc& desc);
		void Clear();

		void AdoptData(Texture&& other);
		Handle<IResource> MoveIntoHandle() override;

		DG::ITexture* ToDiligent(DG::IRenderDevice* device) const;

		// -------------------------------------------------------------
		// Texture Properties
		// -------------------------------------------------------------

		inline const std::vector<TextureSubResDataDesc>& GetSubDataDescs() const {
			assert(mDevice.IsCPU());

			return mCpuAspect.mSubDescs;
		}

		inline float GetIntensity() const {
			return mIntensity;
		}

		inline void SetIntensity(float intensity) {
			mIntensity = intensity;
		}

		inline const std::vector<uint8_t>& GetData() const {
			assert(mDevice.IsCPU());

			return mCpuAspect.mData;
		}

		inline DG::ITexture* GetRasterTexture() const {
			return mRasterAspect.mTexture;
		}

		inline const DG::TextureDesc& GetDesc() const {
			if (mDevice.IsGPU())
				return mRasterAspect.mTexture->GetDesc();
			else
				return mCpuAspect.mDesc;
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
			assert(mDevice.IsGPU());
			return mRasterAspect.mTexture->GetDefaultView(
				DG::TEXTURE_VIEW_SHADER_RESOURCE);
		}

		inline DG::ITextureView* GetRenderTargetView() {
			assert(mDevice.IsGPU());
			return mRasterAspect.mTexture->GetDefaultView(
				DG::TEXTURE_VIEW_RENDER_TARGET);
		}

		inline DG::ITextureView* GetUnorderedAccessView() {
			assert(mDevice.IsGPU());
			return mRasterAspect.mTexture->GetDefaultView(
				DG::TEXTURE_VIEW_UNORDERED_ACCESS);
		}

		void* GetSubresourcePtr(uint mip = 0, uint arrayIndex = 0);
		size_t GetSubresourceSize(uint mip = 0, uint arrayIndex = 0);

		DG::VALUE_TYPE GetComponentType() const;
		int GetComponentCount() const;
		bool GetIsSRGB() const;
		size_t GetPixelByteSize() const;

		static void RegisterMetaData();

		static GPUTextureRead BeginGPURead(
			DG::ITexture* texture, 
			DG::IRenderDevice* device, 
			DG::IDeviceContext* context);

		static void FinishGPURead(
			DG::IDeviceContext* context,
			const GPUTextureRead& read,
			Texture& textureOut);

		inline operator bool() const {
			return mDevice.mType != DeviceType::INVALID;
		}

		typedef LoadParams<Texture> LoadParameters;

		friend class TextureIterator;
		friend class RawSampler;
		friend class cereal::access;
	};
}