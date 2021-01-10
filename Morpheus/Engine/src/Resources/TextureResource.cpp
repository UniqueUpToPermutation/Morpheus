#include <Engine/Resources/TextureResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Engine.hpp>

#include "TextureUtilities.h"
#include "TextureLoader.h"
#include "Image.h"

#include <gli/gli.hpp>
#include <lodepng.h>

#include <type_traits>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
namespace Morpheus {

	template <uint channels, typename T>
	void ImCpy(T* dest, T* src, uint pixel_count) {

		uint ptr_dest = 0;
		uint ptr_src = 0;
		for (uint i = 0; i < pixel_count; ++i) {
			dest[ptr_dest++] = src[ptr_src++];

			if constexpr (channels > 1) {
				dest[ptr_dest++] = src[ptr_src++];
			}
			else {
				dest[ptr_dest++] = 0;
			}

			if constexpr (channels > 2) {
				dest[ptr_dest++] = src[ptr_src++];
			}
			else {
				dest[ptr_dest++] = 0;
			}

			if constexpr (channels > 3) {
				dest[ptr_dest++] = src[ptr_src++];
			}
			else {
				if constexpr (std::is_same<T, uint8_t>::value) {
					dest[ptr_dest++] = 255u;
				} else if constexpr (std::is_same<T, float>::value) {
					dest[ptr_dest++] = 1.0f;
				} else {
					dest[ptr_dest++] = 0;
				}
			}
		}
	}

	TextureResource* TextureResource::ToTexture() {
		return this;
	}

	TextureResource::~TextureResource() {
		mTexture->Release();	
	}

	TextureLoader::TextureLoader(ResourceManager* manager) : mManager(manager) {
	}

	void TextureLoader::Load(const LoadParams<TextureResource>& params, TextureResource* resource) {
		auto pos = params.mSource.rfind('.');
		if (pos == std::string::npos) {
			throw std::runtime_error("Source does not have file extension!");
		}
		auto ext = params.mSource.substr(pos);

		if (ext == ".ktx" || ext == ".dds") {
			LoadGli(params, resource);
		} else if (ext == ".hdr") {
			LoadStb(params, resource);
		} else {
			LoadDiligent(params, resource);
		}
	}

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

	void TextureLoader::LoadGli(const LoadParams<TextureResource>& params, TextureResource* resource) {
		auto device = mManager->GetParent()->GetDevice();

		gli::texture tex = gli::load(params.mSource);
		if (tex.empty()) {
			std::cout << "Failed to load texture " << params.mSource << "!" << std::endl;
			throw std::runtime_error("Failed to load texture!");
		}

		DG::TextureDesc desc;
		desc.Name = params.mSource.c_str();

		auto Target = tex.target();
		auto Format = tex.format();
		
		desc.BindFlags = DG::BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = DG::CPU_ACCESS_NONE;
		desc.Format = GliToDG(Format);
		desc.Width = tex.extent().x;
		desc.Height = tex.extent().y;

		if (Target == gli::TARGET_3D) {
			desc.Depth = tex.extent().z;
		} else {
			desc.ArraySize = tex.layers() * tex.faces();
		}

		desc.MipLevels = tex.levels();
		desc.Usage = DG::USAGE_IMMUTABLE;
		desc.Type = GliToDG(Target);

		std::vector<DG::TextureSubResData> subData;

		size_t block_size = gli::block_size(tex.format());

		std::vector<uint8_t*> expanded_datas;

		bool bExpand = false;

		if (Format == gli::FORMAT_RGB8_UNORM_PACK8 ||
			Format == gli::FORMAT_RGB8_SRGB_PACK8) {
			bExpand = true;
			block_size = 4;
		}

		for (std::size_t Layer = 0; Layer < tex.layers(); ++Layer) {
			for (std::size_t Face = 0; Face < tex.faces(); ++Face) {
				for (std::size_t Level = 0; Level < tex.levels(); ++Level)
				{
					size_t mip_width = std::max(desc.Width >> Level, 1u);
					size_t mip_height = std::max(desc.Height >> Level, 1u);
					size_t mip_depth = 1;

					if (Target == gli::TARGET_3D) {
						mip_depth = std::max(desc.Depth >> Level, 1u);
					}

					DG::TextureSubResData data;
					data.pData = tex.data(Layer, Face, Level);
					data.Stride = block_size * mip_width;
					data.DepthStride = block_size * mip_width * mip_height;

					if (bExpand) {
						uint blocks = mip_width * mip_height * mip_depth;

						uint8_t* expand_data = new uint8_t[4 * blocks];
						ExpandDataUInt8((uint8_t*)data.pData, expand_data, blocks);
						expanded_datas.emplace_back(expand_data);
						data.pData = expand_data;
					}

					subData.emplace_back(data);
				}
			}
		}

		DG::TextureData data;
		data.NumSubresources = subData.size();
		data.pSubResources = subData.data();

		DG::ITexture* out_texture = nullptr;
		device->CreateTexture(desc, &data, &out_texture);

		resource->mSource = params.mSource;
		resource->mTexture = out_texture;

		for (auto& data : expanded_datas) {
			delete[] data;
		}
	}

	void TextureLoader::LoadDiligent(const LoadParams<TextureResource>& params, TextureResource* texture) {
		DG::TextureLoadInfo loadInfo;
		loadInfo.IsSRGB = params.bIsSRGB;
		loadInfo.GenerateMips = params.bGenerateMips;
		DG::ITexture* tex = nullptr;

		std::cout << "Loading " << params.mSource << "..." << std::endl;

		CreateTextureFromFile(params.mSource.c_str(), loadInfo, mManager->GetParent()->GetDevice(), &tex);
		
		texture->mTexture = tex;
		texture->mSource = params.mSource;
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
	void ComputeCoarseMip(DG::Uint32      NumChannels,
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

	void TextureLoader::LoadStb(const LoadParams<TextureResource>& params, TextureResource* texture) {
		unsigned char* pixel_data = nullptr;
		bool b_hdr;
		int comp;
		int x;
		int y;

		if (stbi_is_hdr(params.mSource.c_str())) {
			float* pixels = stbi_loadf(params.mSource.c_str(), &x, &y, &comp, 0);
			if(pixels) {
				pixel_data = reinterpret_cast<unsigned char*>(pixels);
				b_hdr = true;
			}
        }
        else {
			unsigned char* pixels = stbi_load(params.mSource.c_str(), &x, &y, &comp, 0);
			if(pixels) {
				pixel_data = pixels;
				b_hdr = false;
			}
        }

        if (!pixel_data) {
			throw std::runtime_error("Failed to load image file: " + params.mSource);
        }

		DG::TEXTURE_FORMAT format;
		bool bExpand = false;
		uint new_comp = comp;

		if (b_hdr) {
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

		std::vector<DG::TextureSubResData> subDatas;
		std::vector<void*> rawDatas;
		size_t mipCount = MipCount(x, y);
		for (size_t i = 0; i < mipCount; ++i) {
			uint width = std::max(1, x >> i);
			uint height = std::max(1, y >> i);
			DG::TextureSubResData s;
			s.DepthStride = width * height * new_comp * (b_hdr ? sizeof(float) : sizeof(uint8_t));
			s.Stride = width * new_comp * (b_hdr ? sizeof(float) : sizeof(uint8_t));
			void* raw_data;
			if (b_hdr) {
				raw_data = new float[width * height * new_comp];
			} else {
				raw_data = new uint8_t[width * height * new_comp];
			}
			s.pData = raw_data;
			subDatas.emplace_back(s);
			rawDatas.emplace_back(raw_data);
		}

		if (bExpand && b_hdr) {
			ImCpy<3, float>((float*)rawDatas[0], (float*)pixel_data, x * y);
		}
		if (bExpand && !b_hdr) {
			ImCpy<3, uint8_t>((uint8_t*)rawDatas[0], pixel_data, x * y);
		}
		if (!bExpand && b_hdr) {
			memcpy(rawDatas[0], pixel_data, x * y * new_comp * sizeof(float));
		}
		if (!bExpand && !b_hdr) {
			memcpy(rawDatas[0], pixel_data, x * y * new_comp * sizeof(uint8_t));
		}

		for (size_t i = 1; i < mipCount; ++i) {
			uint fineWidth = std::max(1, x >> (i - 1));
			uint fineHeight = std::max(1, y >> (i - 1));
			uint coarseWidth = std::max(1, x >> i);
			uint coarseHeight = std::max(1, y >> i);

			uint fineStride = fineWidth * new_comp;
			uint coarseStride = coarseWidth * new_comp;

			if (b_hdr) {
				ComputeCoarseMip<float>(new_comp, false,
					(float*)rawDatas[i - 1], 
					fineStride,
					fineWidth, fineHeight,
					(float*)rawDatas[i], 
					coarseStride,
					coarseWidth, coarseHeight);
			} else {
				ComputeCoarseMip<uint8_t>(new_comp, false,
					(uint8_t*)rawDatas[i - 1],
					fineStride,
					fineWidth, fineHeight,
					(uint8_t*)rawDatas[i],
					coarseStride,
					coarseWidth, coarseHeight);
			}
		}

		DG::TextureDesc desc;
		desc.BindFlags = DG::BIND_SHADER_RESOURCE | DG::BIND_RENDER_TARGET;
		desc.Width = x;
		desc.Height = y;
		desc.MipLevels = 0;
		desc.Name = params.mSource.c_str();
		desc.Format = format;
		desc.Type = DG::RESOURCE_DIM_TEX_2D;
		desc.Usage = DG::USAGE_IMMUTABLE;
		desc.CPUAccessFlags = DG::CPU_ACCESS_NONE;
		desc.ArraySize = 1;

		DG::TextureData data;
		data.NumSubresources = subDatas.size();
		data.pSubResources = &subDatas[0];

		DG::ITexture* tex = nullptr;
		mManager->GetParent()->GetDevice()->CreateTexture(desc, &data, &tex);

		texture->mTexture = tex;
		texture->mSource = params.mSource;

		for (size_t i = 0; i < mipCount; ++i) {
			if (b_hdr) {
				delete[] (float*)rawDatas[i];
			} else {
				delete[] (uint8_t*)rawDatas[i];
			}
		}
	}

	ResourceCache<TextureResource>::ResourceCache(ResourceManager* manager) :
		mManager(manager), mLoader(manager) {
	}

	ResourceCache<TextureResource>::~ResourceCache() {
		Clear();
	}

	IResource* ResourceCache<TextureResource>::Load(const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<TextureResource>*>(params);
		
		auto it = mResourceMap.find(params_cast->mSource);

		if (it != mResourceMap.end()) {
			return it->second;
		} else {
			auto result = new TextureResource(mManager);
			mLoader.Load(*params_cast, result);
			mResourceMap[params_cast->mSource] = result;
			return result;
		}
	}

	IResource* ResourceCache<TextureResource>::DeferredLoad(const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<TextureResource>*>(params);
		
		auto it = mResourceMap.find(params_cast->mSource);

		if (it != mResourceMap.end()) {
			return it->second;
		} else {
			auto result = new TextureResource(mManager);
			mDeferredResources.emplace_back(std::make_pair(result, *params_cast));
			mResourceMap[params_cast->mSource] = result;
			return result;
		}
	}

	void ResourceCache<TextureResource>::ProcessDeferred() {
		for (auto resource : mDeferredResources) {
			mLoader.Load(resource.second, resource.first);
		}

		mDeferredResources.clear();
	}

	void ResourceCache<TextureResource>::Add(TextureResource* tex, const std::string& source) {
		tex->mSource = source;
		mResourceMap[source] = tex;
		mResourceSet.emplace(tex);
	}

	void ResourceCache<TextureResource>::Add(TextureResource* tex) {
		mResourceSet.emplace(tex);
	}

	void ResourceCache<TextureResource>::Add(IResource* resource, const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<TextureResource>*>(params);
		auto tex = resource->ToTexture();

		Add(tex, params_cast->mSource);
	}

	void ResourceCache<TextureResource>::Unload(IResource* resource) {
		auto tex = resource->ToTexture();

		auto it = mResourceMap.find(tex->GetSource());
		if (it != mResourceMap.end()) {
			if (it->second == tex) {
				mResourceMap.erase(it);
			}
		}

		mResourceSet.erase(tex);

		delete resource;
	}

	void ResourceCache<TextureResource>::Clear() {
		for (auto& item : mResourceSet) {
			item->ResetRefCount();
			delete item;
		}

		mResourceSet.clear();
		mResourceMap.clear();
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

	uint GetByteSize(DG::TEXTURE_FORMAT format) {
		switch (format) {
		case DG::TEX_FORMAT_RGBA8_UNORM_SRGB:
			return 4;
			
		case DG::TEX_FORMAT_RGBA8_UNORM:
			return 4;
			
		case DG::TEX_FORMAT_R8_UNORM:
			return 1;
		
		case DG::TEX_FORMAT_RG8_UNORM:
			return 2;
			
		case DG::TEX_FORMAT_RGBA16_UNORM:
			return 8;
			
		case DG::TEX_FORMAT_RG16_UNORM:
			return 4;
			
		case DG::TEX_FORMAT_R16_UNORM:
			return 2;
			
		case DG::TEX_FORMAT_RGBA16_FLOAT:
			return 8;
			
		case DG::TEX_FORMAT_RG16_FLOAT:
			return 4;
			
		case DG::TEX_FORMAT_R16_FLOAT:
			return 2;
			
		case DG::TEX_FORMAT_RGBA32_FLOAT:
			return 16;
			
		case DG::TEX_FORMAT_RG32_FLOAT:
			return 8;
			
		case DG::TEX_FORMAT_R32_FLOAT:
			return 4;
			
		default:
			throw std::runtime_error("Could not recognize format!");
		}
	}

	void SavePng(DG::ITexture* texture, const std::string& path, 
		DG::IDeviceContext* context, DG::IRenderDevice* device) {
		auto desc = texture->GetDesc();

		if (desc.Type != DG::RESOURCE_DIM_TEX_2D && 
			desc.Type != DG::RESOURCE_DIM_TEX_CUBE &&
			desc.Type != DG::RESOURCE_DIM_TEX_2D_ARRAY &&
			desc.Type != DG::RESOURCE_DIM_TEX_CUBE_ARRAY) {
			throw std::runtime_error("Cannot save non Texture2D or Cubemap to PNG!");
		}

		if (desc.Format != DG::TEX_FORMAT_RGBA8_UNORM &&
			desc.Format != DG::TEX_FORMAT_RGBA8_UNORM_SRGB &&
			desc.Format != DG::TEX_FORMAT_RG8_UNORM &&
			desc.Format != DG::TEX_FORMAT_R8_UNORM) {
			throw std::runtime_error("Texture format must be 8-bit byte type!");
		}

		size_t channels = 0;
		if (desc.Format == DG::TEX_FORMAT_RGBA8_UNORM ||
			desc.Format == DG::TEX_FORMAT_RGBA8_UNORM_SRGB) {
			channels = 4;
		} else if (desc.Format == DG::TEX_FORMAT_RG8_UNORM) {
			channels = 2;
		} else if (desc.Format == DG::TEX_FORMAT_R8_UNORM) {
			channels = 1;
		}

		size_t array_size = desc.ArraySize;

		// Create staging texture
		auto stage_desc = desc;
		stage_desc.Name = "CPU Retrieval Texture";
		stage_desc.CPUAccessFlags = DG::CPU_ACCESS_READ;
		stage_desc.Usage = DG::USAGE_STAGING;
		stage_desc.BindFlags = DG::BIND_NONE;
		stage_desc.MiscFlags = DG::MISC_TEXTURE_FLAG_NONE;
		stage_desc.MipLevels = 1;

		DG::ITexture* stage_tex = nullptr;
		device->CreateTexture(stage_desc, nullptr, &stage_tex);

		for (uint slice = 0; slice < array_size; ++slice) {
			DG::CopyTextureAttribs copy_attribs;
			copy_attribs.pDstTexture = stage_tex;
			copy_attribs.DstTextureTransitionMode = DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
			copy_attribs.DstSlice = slice;

			copy_attribs.pSrcTexture = texture;
			copy_attribs.SrcTextureTransitionMode = DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
			copy_attribs.SrcSlice = slice;

			context->CopyTexture(copy_attribs);
		}

		DG::FenceDesc fence_desc;
		fence_desc.Name = "CPU Retrieval Texture";
		DG::IFence* fence = nullptr;
		device->CreateFence(fence_desc, &fence);

		context->SignalFence(fence, 1);
		context->WaitForFence(fence, 1, true);

		size_t subresource_data_size = desc.Width * desc.Height * 4;
		size_t subresource_pixels = desc.Width * desc.Height;

		const char* side_names_append[] = {
			"_px",
			"_nx",
			"_py",
			"_ny",
			"_pz",
			"_nz"
		};

		for (uint slice = 0; slice < array_size; ++slice) {
			std::vector<uint8_t> buf;
			buf.resize(subresource_data_size);

			DG::MappedTextureSubresource texSub;
			context->MapTextureSubresource(stage_tex, 0, slice, 
				DG::MAP_READ, DG::MAP_FLAG_DO_NOT_WAIT, nullptr, texSub);

			switch (channels) {
				case 1:
					ImCpy<1>(&buf[0], (uint8_t*)texSub.pData, subresource_pixels);
					break;
				case 2:
					ImCpy<2>(&buf[0], (uint8_t*)texSub.pData, subresource_pixels);
					break;
				case 3:
					ImCpy<3>(&buf[0], (uint8_t*)texSub.pData, subresource_pixels);
					break;
				case 4:
					ImCpy<4>(&buf[0], (uint8_t*)texSub.pData, subresource_pixels);
					break;
			}

			context->UnmapTextureSubresource(stage_tex, 0, 0);

			std::string save_path;
			if (desc.Type == DG::RESOURCE_DIM_TEX_2D) {
				save_path = path;
			} else if (desc.Type == DG::RESOURCE_DIM_TEX_CUBE) {
				auto pos = path.rfind('.');
				if (pos == std::string::npos) {
					save_path = path + side_names_append[slice];
				} else {
					save_path = path.substr(0, pos) + side_names_append[slice] + path.substr(pos);
				}
			} else if (desc.Type == DG::RESOURCE_DIM_TEX_2D_ARRAY) {
				auto pos = path.rfind('.');
				if (pos == std::string::npos) {
					save_path = path + std::to_string(slice);
				} else {
					save_path = path.substr(0, pos) + std::to_string(slice) + path.substr(pos);
				}
			} else if (desc.Type == DG::RESOURCE_DIM_TEX_CUBE_ARRAY) {
				auto pos = path.rfind('.');
				uint face = slice % 6;
				uint idx = slice / 6;

				if (pos == std::string::npos) {
					save_path = path + std::to_string(idx) + side_names_append[face];
				} else {
					save_path = path.substr(0, pos) + std::to_string(slice) + side_names_append[face] + path.substr(pos);
				}
			}

			auto err = lodepng::encode(save_path, &buf[0], desc.Width, desc.Height);

			if (err) {
				std::cout << "Encoder error " << err << ": " << lodepng_error_text(err) << std::endl;
				throw std::runtime_error(lodepng_error_text(err));
			}
		}

		stage_tex->Release();
		fence->Release();
	}

	void SaveGli(DG::ITexture* texture, const std::string& path, 
		DG::IDeviceContext* context, DG::IRenderDevice* device) {
		auto target = DGToGli(texture->GetDesc().Type);
		auto format = DGToGli(texture->GetDesc().Format);
		auto desc = texture->GetDesc();

		std::unique_ptr<gli::texture> tex;

		switch (target) {
		case gli::TARGET_1D: {
			gli::extent1d ex;
			ex.x = desc.Width;
			tex.reset(new gli::texture1d(format, ex, desc.MipLevels));
			break;
		}
		case gli::TARGET_1D_ARRAY: {
			gli::extent1d ex;
			ex.x = desc.Width;
			tex.reset(new gli::texture1d_array(format, ex, desc.ArraySize, desc.MipLevels));
			break;
		}
		case gli::TARGET_2D: {
			gli::extent2d ex;
			ex.x = desc.Width;
			ex.y = desc.Height;
			tex.reset(new gli::texture2d(format, ex, desc.MipLevels));
			break;
		}
		case gli::TARGET_2D_ARRAY: {
			gli::extent2d ex;
			ex.x = desc.Width;
			ex.y = desc.Height;
			tex.reset(new gli::texture2d_array(format, ex, desc.ArraySize, desc.MipLevels));
			break;
		}
		case gli::TARGET_3D: {
			gli::extent3d ex;
			ex.x = desc.Width;
			ex.y = desc.Height;
			ex.z = desc.Depth;
			tex.reset(new gli::texture3d(format, ex, desc.MipLevels));
			break;
		}
		case gli::TARGET_CUBE: {
			gli::extent2d ex;
			ex.x = desc.Width;
			ex.y = desc.Height;
			tex.reset(new gli::texture_cube(format, ex, desc.MipLevels));
			break;
		}
		case gli::TARGET_CUBE_ARRAY: {
			gli::extent2d ex;
			ex.x = desc.Width;
			ex.y = desc.Height;
			size_t faces = 6;
			size_t array_size = desc.ArraySize / 6;
			tex.reset(new gli::texture_cube_array(format, ex, array_size, desc.MipLevels));
			break;
		}
		}

		uint pixel_size = GetByteSize(desc.Format);

		// Create staging texture
		auto stage_desc = desc;
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

		DG::FenceDesc fence_desc;
		fence_desc.Name = "CPU Retrieval Texture";
		DG::IFence* fence = nullptr;
		device->CreateFence(fence_desc, &fence);

		context->SignalFence(fence, 1);
		context->WaitForFence(fence, 1, true);

		for (std::size_t Layer = 0; Layer < tex->layers(); ++Layer) {
			for (std::size_t Face = 0; Face < tex->faces(); ++Face) {
				for (std::size_t Level = 0; Level < tex->levels(); ++Level) {

					size_t subresource_width = std::max(1u, desc.Width >> Level);
					size_t subresource_height = std::max(1u, desc.Width >> Level);
					size_t subresource_depth = std::max(1u, desc.Depth >> Level);

					size_t subresource_data_size = subresource_width * subresource_height * 
						subresource_depth * pixel_size;

					size_t array_slice = Layer * tex->faces() + Face;

					DG::MappedTextureSubresource texSub;

					context->MapTextureSubresource(stage_tex, Level, array_slice, 
						DG::MAP_READ, DG::MAP_FLAG_DO_NOT_WAIT, nullptr, texSub);

					std::memcpy(tex->data(Layer, Face, Level), texSub.pData, subresource_data_size);

					context->UnmapTextureSubresource(stage_tex, Level, array_slice);
				}
			}
		}

		stage_tex->Release();
		fence->Release();

		gli::save_ktx(*tex, path);
	}

	void TextureResource::SavePng(const std::string& path) {
		Morpheus::SavePng(mTexture, path, 
			GetManager()->GetParent()->GetImmediateContext(),
			GetManager()->GetParent()->GetDevice());
	}

	void TextureResource::SaveGli(const std::string& path) {
		Morpheus::SaveGli(mTexture, path,
			GetManager()->GetParent()->GetImmediateContext(),
			GetManager()->GetParent()->GetDevice());
	}

	TextureResource* ResourceCache<TextureResource>::MakeResource(
		DG::ITexture* texture, const std::string& source) {

		std::cout << "Creating texture resource " << source << "..." << std::endl;

		TextureResource* res = new TextureResource(mManager, texture);
		res->AddRef();
		Add(res, source);
		return res;
	}

	TextureResource* ResourceCache<TextureResource>::MakeResource(DG::ITexture* texture) {
		TextureResource* res = new TextureResource(mManager, texture);
		res->AddRef();
		Add(res);
		return res;
	}
}