#include <Engine/Resources/Texture.hpp>
#include <Engine/Resources/ResourceData.hpp>
#include <Engine/Resources/ResourceSerialization.hpp>
#include <Engine/Resources/ImageCopy.hpp>

#include <cereal/archives/portable_binary.hpp>

#include "TextureUtilities.h"
#include "Image.h"

#include <gli/gli.hpp>
#include <lodepng.h>

#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
namespace Morpheus {

	DG::TEXTURE_FORMAT GliToDG(gli::format format) {
		switch (format) {
			case gli::FORMAT_RGBA8_SRGB_PACK8:
				return DG::TEX_FORMAT_RGBA8_UNORM_SRGB;
			case gli::FORMAT_RGB8_SRGB_PACK8:
				return DG::TEX_FORMAT_RGBA8_UNORM_SRGB;
			case gli::FORMAT_RGB8_UNORM_PACK8:
				return DG::TEX_FORMAT_RGBA8_UNORM;
			case gli::FORMAT_RGBA8_UNORM_PACK8:
				return DG::TEX_FORMAT_RGBA8_UNORM;
			case gli::FORMAT_R8_UNORM_PACK8:
				return DG::TEX_FORMAT_R8_UNORM;
			case gli::FORMAT_RG8_UNORM_PACK8:
				return DG::TEX_FORMAT_RG8_UNORM;
			case gli::FORMAT_RGBA16_UNORM_PACK16:
				return DG::TEX_FORMAT_RGBA16_UNORM;
			case gli::FORMAT_RG16_UNORM_PACK16:
				return DG::TEX_FORMAT_RG16_UNORM;
			case gli::FORMAT_R16_UNORM_PACK16:
				return DG::TEX_FORMAT_R16_UNORM;
			case gli::FORMAT_RGBA16_SFLOAT_PACK16:
				return DG::TEX_FORMAT_RGBA16_FLOAT;
			case gli::FORMAT_RG16_SFLOAT_PACK16:
				return DG::TEX_FORMAT_RG16_FLOAT;
			case gli::FORMAT_R16_SFLOAT_PACK16:
				return DG::TEX_FORMAT_R16_FLOAT;
			case gli::FORMAT_RGBA32_SFLOAT_PACK32:
				return DG::TEX_FORMAT_RGBA32_FLOAT;
			case gli::FORMAT_RG32_SFLOAT_PACK32:
				return DG::TEX_FORMAT_RG32_FLOAT;
			case gli::FORMAT_R32_SFLOAT_PACK32:
				return DG::TEX_FORMAT_R32_FLOAT;
			default:
				throw std::runtime_error("Could not recognize format!");
		}
	}

	DG::RESOURCE_DIMENSION GliToDG(gli::target target) {
		switch (target) {
			case gli::TARGET_1D:
				return DG::RESOURCE_DIM_TEX_1D;
			case gli::TARGET_1D_ARRAY:
				return DG::RESOURCE_DIM_TEX_1D_ARRAY;
			case gli::TARGET_2D:
				return DG::RESOURCE_DIM_TEX_2D;
			case gli::TARGET_2D_ARRAY:
				return DG::RESOURCE_DIM_TEX_2D_ARRAY;
			case gli::TARGET_3D:
				return DG::RESOURCE_DIM_TEX_3D;
			case gli::TARGET_CUBE:
				return DG::RESOURCE_DIM_TEX_CUBE;
			case gli::TARGET_CUBE_ARRAY:
				return DG::RESOURCE_DIM_TEX_CUBE_ARRAY;
			default:
				throw std::runtime_error("Could not recognize dimension type!");
		}
	}

	gli::target DGToGli(DG::RESOURCE_DIMENSION dim) {
		switch (dim) {
			case DG::RESOURCE_DIM_TEX_1D:
				return gli::TARGET_1D;
			case DG::RESOURCE_DIM_TEX_1D_ARRAY:
				return gli::TARGET_1D_ARRAY;
			case DG::RESOURCE_DIM_TEX_2D:
				return gli::TARGET_2D;
			case DG::RESOURCE_DIM_TEX_2D_ARRAY:
				return gli::TARGET_2D_ARRAY;
			case DG::RESOURCE_DIM_TEX_3D:
				return gli::TARGET_3D;
			case DG::RESOURCE_DIM_TEX_CUBE:
				return gli::TARGET_CUBE;
			case DG::RESOURCE_DIM_TEX_CUBE_ARRAY:
				return gli::TARGET_CUBE_ARRAY;
			default:
				throw std::runtime_error("Resource dimension unrecognized!");
		}
	}

	gli::format DGToGli(DG::TEXTURE_FORMAT format) {
		switch (format) {
			case DG::TEX_FORMAT_RGBA8_UNORM_SRGB:
				return gli::FORMAT_RGBA8_SRGB_PACK8;
				
			case DG::TEX_FORMAT_RGBA8_UNORM:
				return gli::FORMAT_RGB8_UNORM_PACK8;
				
			case DG::TEX_FORMAT_R8_UNORM:
				return gli::FORMAT_R8_UNORM_PACK8;
			
			case DG::TEX_FORMAT_RG8_UNORM:
				return gli::FORMAT_RG8_UNORM_PACK8;
				
			case DG::TEX_FORMAT_RGBA16_UNORM:
				return gli::FORMAT_RGBA16_UNORM_PACK16;
				
			case DG::TEX_FORMAT_RG16_UNORM:
				return gli::FORMAT_RG16_UNORM_PACK16;
				
			case DG::TEX_FORMAT_R16_UNORM:
				return gli::FORMAT_R16_UNORM_PACK16;
				
			case DG::TEX_FORMAT_RGBA16_FLOAT:
				return gli::FORMAT_RGBA16_SFLOAT_PACK16;
				
			case DG::TEX_FORMAT_RG16_FLOAT:
				return gli::FORMAT_RG16_SFLOAT_PACK16;
				
			case DG::TEX_FORMAT_R16_FLOAT:
				return gli::FORMAT_R16_SFLOAT_PACK16;
				
			case DG::TEX_FORMAT_RGBA32_FLOAT:
				return gli::FORMAT_RGBA32_SFLOAT_PACK32;
				
			case DG::TEX_FORMAT_RG32_FLOAT:
				return gli::FORMAT_RG32_SFLOAT_PACK32;
				
			case DG::TEX_FORMAT_R32_FLOAT:
				return gli::FORMAT_R32_SFLOAT_PACK32;
				
			default:
				throw std::runtime_error("Could not recognize format!");
		}
	}

	size_t Texture::GetMipCount() const {
		auto& desc = GetDesc();
		if (desc.MipLevels == 0) {

			size_t mip_width = desc.Width;
			size_t mip_height = desc.Height;
			size_t mip_depth = desc.Depth;

			size_t mip_count = 1;
			while (mip_width > 1 || mip_height > 1 || mip_depth > 1) {
				++mip_count;

				mip_width = std::max<size_t>(mip_width >> 1, 1u);
				mip_height = std::max<size_t>(mip_height >> 1, 1u);
				mip_depth = std::max<size_t>(mip_depth >> 1, 1u);
			}

			return mip_count;
		} else 
			return desc.MipLevels;
	}

	void ExpandDataUInt8(const uint8_t data[], uint8_t expanded_data[], uint blocks) {
		int src_idx = 0;
		int dest_idx = 0;
		for (uint i = 0; i < blocks; ++i) {
			expanded_data[dest_idx++] = data[src_idx++];
			expanded_data[dest_idx++] = data[src_idx++];
			expanded_data[dest_idx++] = data[src_idx++];
			expanded_data[dest_idx++] = 255u;
		}
	}

	inline float LinearToSRGB(float x)
	{
		return x <= 0.0031308 ? x * 12.92f : 1.055f * std::pow(x, 1.f / 2.4f) - 0.055f;
	}

	inline float SRGBToLinear(float x)
	{
		return x <= 0.04045f ? x / 12.92f : std::pow((x + 0.055f) / 1.055f, 2.4f);
	}

	class LinearToSRGBMap
	{
	public:
		LinearToSRGBMap() noexcept
		{
			for (uint i = 0; i < m_ToSRBG.size(); ++i)
			{
				m_ToSRBG[i] = LinearToSRGB(static_cast<float>(i) / 255.f);
			}
		}

		float operator[](uint8_t x) const
		{
			return m_ToSRBG[x];
		}

	private:
		std::array<float, 256> m_ToSRBG;
	};

	class SRGBToLinearMap
	{
	public:
		SRGBToLinearMap() noexcept
		{
			for (uint i = 0; i < m_ToLinear.size(); ++i)
			{
				m_ToLinear[i] = SRGBToLinear(static_cast<float>(i) / 255.f);
			}
		}

		float operator[](uint8_t x) const
		{
			return m_ToLinear[x];
		}

	private:
		std::array<float, 256> m_ToLinear;
	};

	float LinearToSRGB(uint8_t x)
	{
		static const LinearToSRGBMap map;
		return map[x];
	}

	float SRGBToLinear(uint8_t x)
	{
		static const SRGBToLinearMap map;
		return map[x];
	}

	template <typename ChannelType>
	ChannelType SRGBAverage(ChannelType c0, ChannelType c1, ChannelType c2, ChannelType c3)
	{
		static constexpr float NormVal = static_cast<float>(std::numeric_limits<ChannelType>::max());

		float fc0 = static_cast<float>(c0) / NormVal;
		float fc1 = static_cast<float>(c1) / NormVal;
		float fc2 = static_cast<float>(c2) / NormVal;
		float fc3 = static_cast<float>(c3) / NormVal;

		float fLinearAverage = (SRGBToLinear(fc0) + SRGBToLinear(fc1) + SRGBToLinear(fc2) + SRGBToLinear(fc3)) / 4.f;
		float fSRGBAverage   = LinearToSRGB(fLinearAverage) * NormVal;

		static constexpr float MinVal = static_cast<float>(std::numeric_limits<ChannelType>::min());
		static constexpr float MaxVal = static_cast<float>(std::numeric_limits<ChannelType>::max());

		fSRGBAverage = std::max(fSRGBAverage, MinVal);
		fSRGBAverage = std::min(fSRGBAverage, MaxVal);

		return static_cast<ChannelType>(fSRGBAverage);
	}

	template <typename ChannelType>
	ChannelType LinearAverage(ChannelType c0, ChannelType c1, ChannelType c2, ChannelType c3)
	{
		static_assert(std::numeric_limits<ChannelType>::is_integer && !std::numeric_limits<ChannelType>::is_signed, "Unsigned integers are expected");
		return static_cast<ChannelType>((static_cast<uint32_t>(c0) + static_cast<uint32_t>(c1) + static_cast<uint32_t>(c2) + static_cast<uint32_t>(c3)) / 4);
	}

	template <>
	float LinearAverage<float>(float c0, float c1, float c2, float c3) {
		return (c0 + c1 + c2 + c3) / 4.0f;
	}

	template <typename ChannelType>
	void ComputeCoarseMip2D(DG::Uint32      NumChannels,
						bool        IsSRGB,
						const void* pFineMip,
						DG::Uint32      FineMipStride,
						DG::Uint32      FineMipWidth,
						DG::Uint32      FineMipHeight,
						void*       pCoarseMip,
						DG::Uint32      CoarseMipStride,
						DG::Uint32      CoarseMipWidth,
						DG::Uint32      CoarseMipHeight)
	{
		VERIFY_EXPR(FineMipWidth > 0 && FineMipHeight > 0 && FineMipStride > 0);
		VERIFY_EXPR(CoarseMipWidth > 0 && CoarseMipHeight > 0 && CoarseMipStride > 0);

		for (DG::Uint32 row = 0; row < CoarseMipHeight; ++row)
		{
			auto src_row0 = row * 2;
			auto src_row1 = std::min(row * 2 + 1, FineMipHeight - 1);

			auto pSrcRow0 = (reinterpret_cast<const ChannelType*>(pFineMip) + src_row0 * FineMipStride);
			auto pSrcRow1 = (reinterpret_cast<const ChannelType*>(pFineMip) + src_row1 * FineMipStride);

			for (DG::Uint32 col = 0; col < CoarseMipWidth; ++col)
			{
				auto src_col0 = col * 2;
				auto src_col1 = std::min(col * 2 + 1, FineMipWidth - 1);

				for (DG::Uint32 c = 0; c < NumChannels; ++c)
				{
					auto Chnl00 = pSrcRow0[src_col0 * NumChannels + c];
					auto Chnl01 = pSrcRow0[src_col1 * NumChannels + c];
					auto Chnl10 = pSrcRow1[src_col0 * NumChannels + c];
					auto Chnl11 = pSrcRow1[src_col1 * NumChannels + c];

					auto& DstCol = (reinterpret_cast<ChannelType*>(pCoarseMip) + row * CoarseMipStride)[col * NumChannels + c];
					if (IsSRGB)
						DstCol = SRGBAverage(Chnl00, Chnl01, Chnl10, Chnl11);
					else
						DstCol = LinearAverage(Chnl00, Chnl01, Chnl10, Chnl11);
				}
			}
		}
	}

	typedef void (*mip_generator_2d_t)(DG::Uint32 NumChannels,
		bool IsSRGB,
		const void* pFineMip,
		DG::Uint32 FineMipStride,
		DG::Uint32 FineMipWidth,
		DG::Uint32 FineMipHeight,
		void* pCoarseMip,
		DG::Uint32 CoarseMipStride,
		DG::Uint32 CoarseMipWidth,
		DG::Uint32 CoarseMipHeight);

	void Texture::GenerateMips() {
		if (!IsRaw())
			throw std::runtime_error("Texture must have raw aspect!");

		auto& desc = GetDesc();

		size_t mipCount = GetMipCount();
		bool isSRGB = GetIsSRGB();
		size_t pixelSize = GetPixelByteSize();
		uint channelCount = GetComponentCount();
		auto valueType = GetComponentType();

		uint iSubresource = 0;
		for (size_t arrayIndex = 0; arrayIndex < desc.ArraySize; ++arrayIndex) {
			auto& baseSubDesc = mRawAspect.mSubDescs[iSubresource];
			auto last_mip_data = &mRawAspect.mData[baseSubDesc.mSrcOffset];
			++iSubresource;

			for (size_t i = 1; i < mipCount; ++i) {
				auto& subDesc = mRawAspect.mSubDescs[iSubresource];
				auto new_mip_data = &mRawAspect.mData[subDesc.mSrcOffset];

				uint fineWidth = std::max<uint>(1u, desc.Width >> (i - 1));
				uint fineHeight = std::max<uint>(1u, desc.Height >> (i - 1));
				uint fineDepth = std::max<uint>(1u, desc.Depth >> (i - 1));
				uint coarseWidth = std::max<uint>(1u, desc.Width >> i);
				uint coarseHeight = std::max<uint>(1u, desc.Height >> i);
				uint coarseDepth = std::max<uint>(1u, desc.Depth >> i);

				uint fineStride = fineWidth * pixelSize;
				uint coarseStride = coarseWidth * pixelSize;

				mip_generator_2d_t mip_gen;

				switch (valueType) {
					case DG::VT_UINT8:
						mip_gen = &ComputeCoarseMip2D<DG::Uint8>;
						break;
					case DG::VT_UINT16:
						mip_gen = &ComputeCoarseMip2D<DG::Uint16>;
						break;
					case DG::VT_UINT32:
						mip_gen = &ComputeCoarseMip2D<DG::Uint32>;
						break;
					case DG::VT_FLOAT32:
						mip_gen = &ComputeCoarseMip2D<DG::Float32>;
						break;
					default:
						throw std::runtime_error("Mip generation for texture type is not supported!");
				}

				mip_gen(channelCount, isSRGB, last_mip_data, fineStride, 
					fineWidth, fineHeight, new_mip_data, 
					coarseStride, coarseWidth, coarseHeight);

				last_mip_data = new_mip_data;
				++iSubresource;
			}
		}
	}

	void Texture::AllocRaw(const DG::TextureDesc& desc) {
		mFlags |= RESOURCE_RAW_ASPECT;
		mFlags |= RESOURCE_CPU_RESIDENT;

		mRawAspect.mDesc = desc;
		auto pixelSize = GetPixelByteSize();

		if (pixelSize <= 0) {
			throw std::runtime_error("Format not supported!");
		}

		size_t mip_count = GetMipCount();

		// Compute subresources and sizes
		mRawAspect.mSubDescs.reserve(mRawAspect.mDesc.ArraySize * mip_count);
		size_t currentOffset = 0;
		for (size_t iarray = 0; iarray < mRawAspect.mDesc.ArraySize; ++iarray) {
			for (size_t imip = 0; imip < mip_count; ++imip) {
				size_t mip_width = mRawAspect.mDesc.Width;
				size_t mip_height = mRawAspect.mDesc.Height;
				size_t mip_depth = mRawAspect.mDesc.Depth;

				mip_width = std::max<size_t>(mip_width >> imip, 1u);
				mip_height = std::max<size_t>(mip_height >> imip, 1u);
				mip_depth = std::max<size_t>(mip_depth >> imip, 1u);

				TextureSubResDataDesc subDesc;
				subDesc.mSrcOffset = currentOffset;
				subDesc.mDepthStride = mip_width * mip_height * pixelSize;
				subDesc.mStride = mip_width * pixelSize;
				mRawAspect.mSubDescs.emplace_back(subDesc);

				currentOffset += mip_width * mip_height * mip_depth * pixelSize;
			}
		}

		mRawAspect.mData.resize(currentOffset);
	}

	void* Texture::GetSubresourcePtr(uint mip, uint arrayIndex) {
		assert(IsRaw());

		size_t subresourceIndex = arrayIndex * mRawAspect.mDesc.MipLevels + mip;
		return &mRawAspect.mData[mRawAspect.mSubDescs[subresourceIndex].mSrcOffset];
	}

	size_t Texture::GetSubresourceSize(uint mip, uint arrayIndex) {
		assert(mFlags & RESOURCE_RAW_ASPECT);

		size_t subresourceIndex = arrayIndex * mRawAspect.mDesc.MipLevels + mip;
		size_t depth = std::max<size_t>(1u, mRawAspect.mDesc.Depth >> mip);
		return mRawAspect.mSubDescs[subresourceIndex].mDepthStride * depth;
	}

	DG::VALUE_TYPE Texture::GetComponentType() const {
		return Morpheus::GetComponentType(GetDesc().Format);
	}

	int Texture::GetComponentCount() const {
		return Morpheus::GetComponentCount(GetDesc().Format);
	}

	bool Texture::GetIsSRGB() const {
		return Morpheus::GetIsSRGB(GetDesc().Format);
	}

	size_t Texture::GetPixelByteSize() const {
		return Morpheus::GetPixelByteSize(GetDesc().Format);
	}

	Texture::Texture(const DG::TextureDesc& desc) {
		AllocRaw(desc);		
	}

	Task Texture::SaveTask(const std::string& path) {
		return Task([this, path](const TaskParams& e) {
			std::ofstream f(path, std::ios::binary);

			if (f.is_open()) {
				cereal::PortableBinaryOutputArchive ar(f);
				Morpheus::Save(ar, this);
				f.close();
			} else {
				throw std::runtime_error("Could not open file for writing!");
			}
		}, std::string("Save Texture ") + path + " (Archive)", TaskType::FILE_IO);
	}

	Task Texture::SaveGliTask(const std::string& path) {
		if (!(mFlags & RESOURCE_RAW_ASPECT))
			throw std::runtime_error("Texture must have raw aspect to save!");

		return Task([this, path](const TaskParams& e) {
			
			auto& desc = GetDesc();

			auto target = DGToGli(desc.Type);
			auto format = DGToGli(desc.Format);

			std::unique_ptr<gli::texture> tex;

			size_t mip_count = GetMipCount();

			switch (target) {
			case gli::TARGET_1D: {
				gli::extent1d ex;
				ex.x = desc.Width;
				tex.reset(new gli::texture1d(format, ex, mip_count));
				break;
			}
			case gli::TARGET_1D_ARRAY: {
				gli::extent1d ex;
				ex.x = desc.Width;
				tex.reset(new gli::texture1d_array(format, ex, desc.ArraySize, mip_count));
				break;
			}
			case gli::TARGET_2D: {
				gli::extent2d ex;
				ex.x = desc.Width;
				ex.y = desc.Height;
				tex.reset(new gli::texture2d(format, ex, mip_count));
				break;
			}
			case gli::TARGET_2D_ARRAY: {
				gli::extent2d ex;
				ex.x = desc.Width;
				ex.y = desc.Height;
				tex.reset(new gli::texture2d_array(format, ex, desc.ArraySize, mip_count));
				break;
			}
			case gli::TARGET_3D: {
				gli::extent3d ex;
				ex.x = desc.Width;
				ex.y = desc.Height;
				ex.z = desc.Depth;
				tex.reset(new gli::texture3d(format, ex, mip_count));
				break;
			}
			case gli::TARGET_CUBE: {
				gli::extent2d ex;
				ex.x = desc.Width;
				ex.y = desc.Height;
				tex.reset(new gli::texture_cube(format, ex, mip_count));
				break;
			}
			case gli::TARGET_CUBE_ARRAY: {
				gli::extent2d ex;
				ex.x = desc.Width;
				ex.y = desc.Height;
				size_t faces = 6;
				size_t array_size = desc.ArraySize / faces;
				tex.reset(new gli::texture_cube_array(format, ex, array_size, mip_count));
				break;
			}
			}

			uint pixel_size = GetPixelByteSize();

			size_t face_count = target == gli::TARGET_CUBE || target == gli::TARGET_CUBE_ARRAY ? 6 : 1;

			for (size_t subResource = 0; 
				subResource < mRawAspect.mSubDescs.size(); 
				++subResource) {
				size_t Level = subResource % mip_count;
				size_t Slice = subResource / mip_count;
				size_t Face = subResource % face_count;
				size_t Layer = subResource / face_count;
				
				size_t subresource_width = std::max(1u, desc.Width >> Level);
				size_t subresource_height = std::max(1u, desc.Width >> Level);
				size_t subresource_depth = std::max(1u, desc.Depth >> Level);

				size_t subresource_data_size = subresource_width * subresource_height * 
						subresource_depth * pixel_size;

				size_t array_slice = Layer * tex->faces() + Face;

				std::memcpy(tex->data(Layer, Face, Level), 
					&mRawAspect.mData[mRawAspect.mSubDescs[subResource].mSrcOffset], 
					subresource_data_size);
			}

			gli::save_ktx(*tex, path);
		}, 
		std::string("Save Texture ") + path + " (GLI)", 
		TaskType::FILE_IO);
	}

	Task Texture::SavePngTask(const std::string& path, bool bSaveMips) {
		if (!IsRaw())
			throw std::runtime_error("Texture must have raw aspect to save!");

		return Task([this, path, bSaveMips](const TaskParams& e) {
			std::vector<uint8_t> buf;

			auto& desc = GetDesc();

			if (desc.Type == DG::RESOURCE_DIM_TEX_3D) {
				throw std::runtime_error("Cannot have 3D textures as PNG!");
			}

			auto type = GetComponentType();

			if (type == DG::VT_NUM_TYPES) {
				throw std::runtime_error("Invalid texture format!");
			}

			size_t mip_count = GetMipCount();

			size_t increment = bSaveMips ? 1 : mip_count;
			size_t channel_count = GetComponentCount();
			size_t face_count = desc.Type == DG::RESOURCE_DIM_TEX_CUBE 
				|| desc.Type == DG::RESOURCE_DIM_TEX_CUBE_ARRAY ? 6 : 1;
			
			size_t slices = mRawAspect.mSubDescs.size() / mip_count;

			std::string path_base;

			auto pos = path.find('.');
			if (pos != std::string::npos) {
				path_base = path.substr(0, pos);
			} else {
				path_base = path;
			}

			for (size_t subResource = 0; 
				subResource < mRawAspect.mSubDescs.size(); 
				subResource += increment) {
				size_t Level = subResource % mip_count;
				size_t Slice = subResource / mip_count;

				size_t subresource_width = std::max(1u, desc.Width >> Level);
				size_t subresource_height = std::max(1u, desc.Width >> Level);
				size_t subresource_depth = std::max(1u, desc.Depth >> Level);
				size_t buf_size = subresource_width * subresource_height * 
					subresource_depth * 4;

				std::vector<uint8_t> buf;
				buf.resize(buf_size);

				auto& sub = mRawAspect.mSubDescs[subResource];

				ImageCopy<uint8_t, 4>(&buf[0], &mRawAspect.mData[sub.mSrcOffset], 
					subresource_width * subresource_height, channel_count, type);

				std::stringstream ss;
				ss << path_base;
				if (slices > 1) {
					ss << "_slice_" << Slice;
				}
				if (bSaveMips) {
					ss << "_mip_" << Level;
				}
				ss << ".png";

				auto err = lodepng::encode(ss.str(), &buf[0], subresource_width, subresource_height);

				if (err) {
					std::cout << "Encoder error " << err << ": " << lodepng_error_text(err) << std::endl;
					throw std::runtime_error(lodepng_error_text(err));
				}
			}
		}, 
		std::string("Save Texture ") + path + " (PNG)", 
		TaskType::FILE_IO);
	}

	void Texture::RetrieveRawData(DG::ITexture* texture, 
		DG::IRenderDevice* device, DG::IDeviceContext* context) {
		RetrieveRawData(texture, device, context, texture->GetDesc());
	}

	void Texture::CopyTo(Texture* texture) const {
		if (mFlags & RESOURCE_RASTERIZER_ASPECT)
			throw std::runtime_error("Cannot copy raster texture!");

		texture->mRawAspect = mRawAspect;
		texture->mIntensity = mIntensity;
		texture->mFlags = mFlags;
	}

	void Texture::CopyFrom(const Texture& texture) {
		texture.CopyTo(this);
	}

	void Texture::RetrieveRawData(DG::ITexture* texture, 
		DG::IRenderDevice* device, 
		DG::IDeviceContext* context, 
		const DG::TextureDesc& texDesc) {

		mFlags |= RESOURCE_RAW_ASPECT;
		mFlags |= RESOURCE_CPU_RESIDENT;

		auto& desc = texture->GetDesc();

		if (desc.CPUAccessFlags & DG::CPU_ACCESS_READ) {

			mRawAspect.mDesc = texDesc;
			mRawAspect.mData.clear();
			mRawAspect.mSubDescs.clear();

			size_t layers = desc.MipLevels;
			size_t slices = desc.ArraySize;

			size_t pixel_size = GetPixelByteSize();

			size_t current_source_offset = 0;
			for (size_t slice = 0; slice < slices; ++slice) {
				for (size_t layer = 0; layer < layers; ++layer) {
					size_t subresource_width = std::max(1u, desc.Width >> layer);
					size_t subresource_height = std::max(1u, desc.Width >> layer);
					size_t subresource_depth = std::max(1u, desc.Depth >> layer);

					TextureSubResDataDesc sub;
					sub.mDepthStride = subresource_width * subresource_height * pixel_size;
					sub.mSrcOffset = current_source_offset;
					sub.mStride = subresource_width * pixel_size;

					mRawAspect.mSubDescs.emplace_back(sub);
					current_source_offset += subresource_width * subresource_depth * subresource_height * pixel_size;
				}
			}

			mRawAspect.mData.resize(current_source_offset);

			std::vector<DG::MappedTextureSubresource> mappedSubs;
			mappedSubs.reserve(slices * layers);

			// Map subresources and synchronize
			for (size_t slice = 0; slice < slices; ++slice) {
				for (size_t layer = 0; layer < layers; ++layer) {
					DG::MappedTextureSubresource texSub;
					context->MapTextureSubresource(texture, layer, slice, 
						DG::MAP_READ, DG::MAP_FLAG_DO_NOT_WAIT, nullptr, texSub);
					mappedSubs.emplace_back(texSub);
				}
			}

			DG::FenceDesc fence_desc;
			fence_desc.Name = "CPU Retrieval Fence";
			DG::IFence* fence = nullptr;
			device->CreateFence(fence_desc, &fence);
			context->SignalFence(fence, 1);
			context->WaitForFence(fence, 1, true);
			fence->Release();

			size_t subresource = 0;
			for (size_t slice = 0; slice < slices; ++slice) {
				for (size_t layer = 0; layer < layers; ++layer) {
					size_t subresource_width = std::max(1u, desc.Width >> layer);
					size_t subresource_height = std::max(1u, desc.Height >> layer);
					size_t subresource_depth = std::max(1u, desc.Depth >> layer);

					size_t subresource_data_size = subresource_width * subresource_height * 
						subresource_depth * pixel_size;

					auto desc = mRawAspect.mSubDescs[subresource];
					DG::MappedTextureSubresource texSub = mappedSubs[subresource];

					std::memcpy(&mRawAspect.mData[desc.mSrcOffset], 
						texSub.pData, subresource_data_size);
					context->UnmapTextureSubresource(texture, layer, slice);

					++subresource;
				}
			}
		} else {
			// Create staging texture
			auto stage_desc = texture->GetDesc();
			stage_desc.Name = "CPU Retrieval Texture";
			stage_desc.CPUAccessFlags = DG::CPU_ACCESS_READ;
			stage_desc.Usage = DG::USAGE_STAGING;
			stage_desc.BindFlags = DG::BIND_NONE;
			stage_desc.MiscFlags = DG::MISC_TEXTURE_FLAG_NONE;

			DG::ITexture* stage_tex = nullptr;
			device->CreateTexture(stage_desc, nullptr, &stage_tex);

			DG::CopyTextureAttribs copy_attribs;

			for (uint slice = 0; slice < desc.ArraySize; ++slice) {
				for (uint mip = 0; mip < desc.MipLevels; ++mip) {
					copy_attribs.DstSlice = slice;
					copy_attribs.DstMipLevel = mip;
					copy_attribs.pDstTexture = stage_tex;
					copy_attribs.DstTextureTransitionMode = DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

					copy_attribs.SrcSlice = slice;
					copy_attribs.SrcMipLevel = mip;
					copy_attribs.pSrcTexture = texture;
					copy_attribs.SrcTextureTransitionMode = DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

					context->CopyTexture(copy_attribs);
				}
			}

			// Retrieve data from staging texture
			RetrieveRawData(stage_tex, device, context, desc);

			stage_tex->Release();
		}
	}

	void LoadStbDataRaw(const LoadParams<Texture>& params,
		bool bIsHDR,
		int x,
		int y,
		int comp,
		unsigned char* pixel_data,
		Texture* rawTexture) {

		size_t currentIndx = 0;

		if (!pixel_data) {
			throw std::runtime_error("Failed to load image file: " + params.mSource);
        }

		DG::TEXTURE_FORMAT format = DG::TEX_FORMAT_UNKNOWN;
		bool bExpand = false;
		uint new_comp = comp;

		if (bIsHDR) {
			switch (comp) {
			case 1:
				format = DG::TEX_FORMAT_R32_FLOAT;
				break;
			case 2:
				format = DG::TEX_FORMAT_RG32_FLOAT;
				break;
			case 3:
				format = DG::TEX_FORMAT_RGBA32_FLOAT;
				bExpand = true;
				new_comp = 4;
				break;
			case 4:
				format = DG::TEX_FORMAT_RGBA32_FLOAT;
				break;
			}
		} else {
			switch (comp) {
			case 1:
				format = DG::TEX_FORMAT_R8_UNORM;
				break;
			case 2:
				format = DG::TEX_FORMAT_RG8_UNORM;
				break;
			case 3:
				format = DG::TEX_FORMAT_RGBA8_UNORM;
				bExpand = true;
				new_comp = 4;
				break;
			case 4:
				format = DG::TEX_FORMAT_RGBA8_UNORM;
				break;
			}
		}

		std::vector<TextureSubResDataDesc> subDatas;
		std::vector<uint8_t> rawData;

		size_t sz_multiplier = bIsHDR ?  sizeof(float) / sizeof(uint8_t) : 1;
		size_t sz = x * y * new_comp * sz_multiplier;

		rawData.resize(sz * 2);

		size_t mipCount = MipCount(x, y);

		if (bExpand && bIsHDR) {
			ImageCopyBasic<3, float>((float*)&rawData[0], (float*)pixel_data, x * y);
		}
		if (bExpand && !bIsHDR) {
			ImageCopyBasic<3, uint8_t>((uint8_t*)&rawData[0], pixel_data, x * y);
		}
		if (!bExpand && bIsHDR) {
			std::memcpy(&rawData[0], pixel_data, x * y * new_comp * sizeof(float));
		}
		if (!bExpand && !bIsHDR) {
			std::memcpy(&rawData[0], pixel_data, x * y * new_comp * sizeof(uint8_t));
		}

		auto last_mip_data = &rawData[0];
		currentIndx += x * y * new_comp * sz_multiplier; 

		TextureSubResDataDesc mip0;
		mip0.mDepthStride = x * y * new_comp * sz_multiplier;
		mip0.mStride = x * new_comp * sz_multiplier;
		mip0.mSrcOffset = 0;

		subDatas.emplace_back(mip0);

		for (size_t i = 1; i < mipCount; ++i) {
			auto mip_data = &rawData[currentIndx];

			uint fineWidth = std::max(1, x >> (i - 1));
			uint fineHeight = std::max(1, y >> (i - 1));
			uint coarseWidth = std::max(1, x >> i);
			uint coarseHeight = std::max(1, y >> i);

			uint fineStride = fineWidth * new_comp;
			uint coarseStride = coarseWidth * new_comp;

			if (bIsHDR) {
				ComputeCoarseMip2D<float>(new_comp, false,
					(float*)last_mip_data, 
					fineStride,
					fineWidth, fineHeight,
					(float*)mip_data, 
					coarseStride,
					coarseWidth, coarseHeight);
			} else {
				ComputeCoarseMip2D<uint8_t>(new_comp, false,
					(uint8_t*)last_mip_data,
					fineStride,
					fineWidth, fineHeight,
					(uint8_t*)mip_data,
					coarseStride,
					coarseWidth, coarseHeight);
			}

			TextureSubResDataDesc mip;
			mip.mDepthStride = coarseWidth * coarseHeight * new_comp * sz_multiplier;
			mip.mStride = coarseWidth * new_comp * sz_multiplier;
			mip.mSrcOffset = currentIndx;
			subDatas.emplace_back(mip);

			currentIndx += coarseWidth * coarseHeight * new_comp * sz_multiplier;
		}

		DG::TextureDesc desc;
		desc.BindFlags = DG::BIND_SHADER_RESOURCE;
		desc.Width = x;
		desc.Height = y;
		desc.MipLevels = 0;
		desc.Name = params.mSource.c_str();
		desc.Format = format;
		desc.Type = DG::RESOURCE_DIM_TEX_2D;
		desc.Usage = DG::USAGE_IMMUTABLE;
		desc.CPUAccessFlags = DG::CPU_ACCESS_NONE;
		desc.ArraySize = 1;

		rawTexture->Set(desc, std::move(rawData), subDatas);
	}

	void LoadGliDataRaw(const LoadParams<Texture>& params,
		gli::texture* tex,
		Texture* into) {
		if (tex->empty()) {
			std::cout << "Failed to load texture " << params.mSource << "!" << std::endl;
			throw std::runtime_error("Failed to load texture!");
		}

		DG::TextureDesc desc;
		desc.Name = params.mSource.c_str();

		auto Target = tex->target();
		auto Format = tex->format();
		
		desc.BindFlags = DG::BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = DG::CPU_ACCESS_NONE;
		desc.Format = GliToDG(Format);
		desc.Width = tex->extent().x;
		desc.Height = tex->extent().y;

		if (Target == gli::TARGET_3D) {
			desc.Depth = tex->extent().z;
		} else {
			desc.ArraySize = tex->layers() * tex->faces();
		}

		desc.MipLevels = tex->levels();
		desc.Usage = DG::USAGE_IMMUTABLE;
		desc.Type = GliToDG(Target);

		std::vector<TextureSubResDataDesc> subData;

		size_t block_size = gli::block_size(tex->format());
		
		bool bExpand = false;

		if (Format == gli::FORMAT_RGB8_UNORM_PACK8 ||
			Format == gli::FORMAT_RGB8_SRGB_PACK8) {
			bExpand = true;
			block_size = 4;
		}

		std::vector<uint8_t> datas;
		datas.resize(tex->extent().x * tex->extent().y * tex->extent().z * tex->layers() * tex->faces() * block_size);

		size_t offset = 0;

		for (std::size_t Layer = 0; Layer < tex->layers(); ++Layer) {
			for (std::size_t Face = 0; Face < tex->faces(); ++Face) {
				for (std::size_t Level = 0; Level < tex->levels(); ++Level)
				{
					size_t mip_width = std::max(desc.Width >> Level, 1u);
					size_t mip_height = std::max(desc.Height >> Level, 1u);
					size_t mip_depth = 1;

					if (Target == gli::TARGET_3D) {
						mip_depth = std::max(desc.Depth >> Level, 1u);
					}

					TextureSubResDataDesc data;
					data.mSrcOffset = offset;
					data.mStride = block_size * mip_width;
					data.mDepthStride = block_size * mip_width * mip_height;

					uint blocks = mip_width * mip_height * mip_depth;

					if (bExpand) {
						ExpandDataUInt8((uint8_t*)tex->data(Layer, Face, Level), &datas[offset], blocks);
					} else {
						std::memcpy(&datas[offset], tex->data(Layer, Face, Level), block_size * blocks);
					}

					offset += block_size * blocks;

					subData.emplace_back(data);
				}
			}
		}

		into->Set(desc, std::move(datas), subData);
	}

	void LoadPngDataRaw(const LoadParams<Texture>& params, 
		const std::vector<uint8_t>& image,
		uint32_t width, uint32_t height,
		Texture* into) {

		std::vector<uint8_t> rawData;
		std::vector<TextureSubResDataDesc> subDatas;
		rawData.resize(image.size() * 2);

		size_t currentIndx = 0;

		DG::TEXTURE_FORMAT format;
		if (params.bIsSRGB) {
			format = DG::TEX_FORMAT_RGBA8_UNORM_SRGB;
		} else {
			format = DG::TEX_FORMAT_RGBA8_UNORM;
		}

		DG::TextureDesc desc;
		desc.BindFlags = DG::BIND_SHADER_RESOURCE;
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = params.bGenerateMips ? 0 : 1;
		desc.Name = params.mSource.c_str();
		desc.Format = format;
		desc.Type = DG::RESOURCE_DIM_TEX_2D;
		desc.Usage = DG::USAGE_IMMUTABLE;
		desc.CPUAccessFlags = DG::CPU_ACCESS_NONE;
		desc.ArraySize = 1;

		into->AllocRaw(desc);
		std::memcpy(into->GetSubresourcePtr(), &image[0], into->GetSubresourceSize());

		if (params.bGenerateMips)
			into->GenerateMips();
	}

	void Texture::ReadArchive(const uint8_t* rawArchive, const size_t length) {
		MemoryInputStream stream(rawArchive, length);
		cereal::PortableBinaryInputArchive ar(stream);
		Morpheus::Load(ar, this);

		mFlags |= RESOURCE_CPU_RESIDENT;
		mFlags |= RESOURCE_RAW_ASPECT;
	}

	void Texture::ReadPng(const LoadParams<Texture>& params,
		const uint8_t* rawData, const size_t length) {
		std::vector<uint8_t> image;
		uint32_t width, height;
		uint32_t error = lodepng::decode(image, width, height, rawData, length);

		//if there's an error, display it
		if (error)
			throw std::runtime_error(lodepng_error_text(error));

		//the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
		//State state contains extra information about the PNG such as text chunks, ...
		else 
			LoadPngDataRaw(params, image, width, height, this);
	}

	Task Texture::ReadPngTask(const LoadParams<Texture>& params) {

		Task task([this, params](const TaskParams& e) {
			std::vector<uint8_t> data;
			ReadBinaryFile(params.mSource, data);
			ReadPng(params, &data[0], data.size());
		}, 
		std::string("Load Texture ") + params.mSource + " (PNG)", 
		TaskType::FILE_IO);

		return task;
	}

	enum class LoadType {
		PNG,
		GLI,
		STB,
		ARCHIVE
	};

	Task LoadDeferred(Texture* texture, const LoadParams<Texture>& params, LoadType type) {

		Task task([texture, type, params](const TaskParams& e) {
		
			std::vector<uint8_t> data;
			ReadBinaryFile(params.mSource, data);

			switch (type) {
				case LoadType::PNG:
					texture->ReadPng(params, &data[0], data.size());
					break;
				case LoadType::STB:
					texture->ReadStb(params, &data[0], data.size());
					break;
				case LoadType::GLI:
					texture->ReadGli(params, &data[0], data.size());
					break;
				case LoadType::ARCHIVE:
					texture->ReadArchive(&data[0], data.size());
					break;
			}
		}, std::string("Load Texture ") + params.mSource, TaskType::FILE_IO);

		return task;
	}

	Task Texture::ReadArchiveTask(const std::string& path) {
		Task task([this, path](const TaskParams& e) {
			std::vector<uint8_t> data;
			ReadBinaryFile(path, data);
			ReadArchive(&data[0], data.size());
		},
		std::string("Load Texture ") + path + " (Archive)",
		TaskType::FILE_IO);
	
		return task;
	}

	Task Texture::ReadStbTask(const LoadParams<Texture>& params) {
		Task task([this, params](const TaskParams& e) {
			std::vector<uint8_t> data;
			ReadBinaryFile(params.mSource, data);
			ReadStb(params, &data[0], data.size());
		}, 
		std::string("Load Texture ") + params.mSource + " (STB)",
		TaskType::FILE_IO);

		return task;
	}

	Task Texture::ReadTask(const LoadParams<Texture>& params) {
		auto pos = params.mSource.rfind('.');
		if (pos == std::string::npos) {
			throw std::runtime_error("Source does not have file extension!");
		}
		auto ext = params.mSource.substr(pos);

		if (ext == ".ktx" || ext == ".dds") {
			return ReadGliTask(params);
		} else if (ext == ".hdr") {
			return ReadStbTask(params);
		} else if (ext == ".png") {
			return ReadPngTask(params);
		} else if (ext == TEXTURE_ARCHIVE_EXTENSION) {
			return ReadArchiveTask(params.mSource);
		} else {
			throw std::runtime_error("Texture file format not supported!");
		}
	}

	void Texture::ReadGli(const LoadParams<Texture>& params, 
		const uint8_t* rawData, const size_t length) {
		gli::texture tex = gli::load((const char*)rawData, length);

		if (tex.empty()) {
			std::cout << "Failed to load texture " << params.mSource << "!" << std::endl;
			throw std::runtime_error("Failed to load texture!");
		}

		LoadGliDataRaw(params, &tex, this);
	}

	Task Texture::ReadGliTask(const LoadParams<Texture>& params) {
		Task task([this, params](const TaskParams& e) {
			std::vector<uint8_t> data;
			ReadBinaryFile(params.mSource, data);
			ReadGli(params, &data[0], data.size());
		}, 
		std::string("Load Texture ") + params.mSource + " (GLI)",
		TaskType::FILE_IO);

		return task;
	}

	void Texture::ReadStb(const LoadParams<Texture>& params,
		const uint8_t* data, size_t length) {
		unsigned char* pixel_data = nullptr;
		std::unique_ptr<unsigned char[]> pixels_uc = nullptr;
		std::unique_ptr<float[]> pixels_f = nullptr;
		bool b_hdr = false;
		int comp;
		int x;
		int y;

		if (stbi_is_hdr(params.mSource.c_str())) {
			pixels_f.reset(stbi_loadf_from_memory(data, length, &x, &y, &comp, 0));
			if (pixels_f) {
				pixel_data = reinterpret_cast<unsigned char*>(pixels_f.get());
				b_hdr = true;
			}
		}
		else {
			pixels_uc.reset(stbi_load_from_memory(data, length, &x, &y, &comp, 0));
			if (pixels_uc) {
				pixel_data = pixels_uc.get();
				b_hdr = false;
			} 
		}

		LoadStbDataRaw(params, b_hdr, x, y, comp, pixel_data, this);
	}

	DG::ITexture* Texture::SpawnOnGPU(DG::IRenderDevice* device) const {

		if (!(mFlags & RESOURCE_RAW_ASPECT))
			throw std::runtime_error("Spawning on GPU requires raw aspect!");

		DG::ITexture* texture = nullptr;
		DG::TextureData data;

		std::vector<DG::TextureSubResData> subs;
		subs.reserve(mRawAspect.mSubDescs.size());

		for (auto& subDesc : mRawAspect.mSubDescs) {
			DG::TextureSubResData subDG;
			subDG.DepthStride = subDesc.mDepthStride;
			subDG.Stride = subDesc.mStride;
			subDG.pData = &mRawAspect.mData[subDesc.mSrcOffset];
			subs.emplace_back(subDG);
		}

		data.NumSubresources = mRawAspect.mSubDescs.size();
		data.pSubResources = &subs[0];

		device->CreateTexture(mRawAspect.mDesc, &data, &texture);

		return texture;
	}

	template <typename R>
	ResourceTask<R> LoadTemplated(GraphicsDevice device, const LoadParams<Texture>& params) {
		Promise<R> promise;
		Future<R> future(promise);

		struct Data {
			Texture mRaw;
		};

		Task task([params, device, promise = std::move(promise), data = Data()](const TaskParams& e) mutable {
			if (e.mTask->BeginSubTask()) {
				data.mRaw.Read(params);
				e.mTask->EndSubTask();
			}

			if (device.mGpuDevice)
				if (e.mTask->RequestThreadSwitch(e, ASSIGN_THREAD_MAIN))
					return TaskResult::REQUEST_THREAD_SWITCH;

			if (e.mTask->BeginSubTask()) {
				Texture* texture = new Texture();
				texture->CreateDeviceAspect(device, &data.mRaw);

				promise.Set(texture, e.mQueue);

				if constexpr (std::is_same_v<R, Handle<Texture>>) {
					texture->Release();
				}

				e.mTask->EndSubTask();
			}

			return TaskResult::FINISHED;
		}, 
		std::string("Load ") + params.mSource, 
		TaskType::FILE_IO);

		ResourceTask<R> resourceTask;
		resourceTask.mTask = std::move(task);
		resourceTask.mFuture = future;

		return resourceTask;
	}

	template <typename R>
	ResourceTask<R> LoadTemplated(const LoadParams<Texture>& params) {
		Promise<R> promise;
		Future<R> future(promise);

		Task task([params, promise = std::move(promise)](const TaskParams& e) mutable {
			Texture* texture = new Texture();
			texture->Read(params);
			promise.Set(texture, e.mQueue);
		}, 
		std::string("Load ") + params.mSource, 
		TaskType::FILE_IO);

		ResourceTask<R> resourceTask;
		resourceTask.mTask = std::move(task);
		resourceTask.mFuture = future;

		return resourceTask;
	}

	ResourceTask<Handle<Texture>> Texture::Load(GraphicsDevice device, 
		const LoadParams<Texture>& params) {
		return LoadTemplated<Handle<Texture>>(device, params);
	}

	ResourceTask<Texture*> Texture::LoadPointer(GraphicsDevice device, 
		const LoadParams<Texture>& params) {
		return LoadTemplated<Texture*>(device, params);
	}

	ResourceTask<Texture*> Texture::LoadPointer(
		const LoadParams<Texture>& params) {
		return LoadTemplated<Texture*>(params);
	}

	ResourceTask<Handle<Texture>> Texture::Load(
		const LoadParams<Texture>& params) {
		return LoadTemplated<Handle<Texture>>(params);
	}

	void Texture::CreateRasterAspect(DG::IRenderDevice* device, 
		const Texture* source) {
		mFlags |= RESOURCE_RASTERIZER_ASPECT;
		mFlags |= RESOURCE_GPU_RESIDENT;
		mRasterAspect.mTexture = source->SpawnOnGPU(device);
	}

	void Texture::CreateRasterAspect(DG::ITexture* texture) {
		mFlags |= RESOURCE_RASTERIZER_ASPECT;
		mFlags |= RESOURCE_GPU_RESIDENT;
		mRasterAspect.mTexture = texture;
	}

	void Texture::CreateDeviceAspect(GraphicsDevice device, 
		const Texture* source) {
		if (device.mGpuDevice)
			CreateRasterAspect(device.mGpuDevice, source);
		else
			throw std::runtime_error("Not implemented!");
	}

	void Texture::AdoptData(Texture&& other) {
		mRasterAspect = std::move(other.mRasterAspect);
		mRawAspect = std::move(other.mRawAspect);

		mIntensity = other.mIntensity;
		mFlags = other.mFlags;
	}

	Texture::Texture(Texture&& other) {
		AdoptData(std::move(other));
	}

	Texture& Texture::operator=(Texture&& other) {
		AdoptData(std::move(other));
		return *this;
	}

	void Texture::ToRaw(Texture* out) const {
		if (IsRaw()) {
			CopyTo(out);
		} else {
			throw std::runtime_error("Not implemented!");
		}
	}

	void Texture::ToRaw(Texture* out,
		DG::IRenderDevice* device, 
		DG::IDeviceContext* context) const {
		if (IsRaw()) {
			CopyTo(out);
		} else if (IsRasterResource()) {
			out->RetrieveRawData(GetRasterTexture(), device, context);
		} else {
			throw std::runtime_error("Not implemented!");
		}
	}

	void Texture::Clear() {
		mRawAspect = RawAspect();
		mRasterAspect = RasterizerAspect();

		mFlags = 0u;
	}
}