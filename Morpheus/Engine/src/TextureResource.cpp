#include <Engine/TextureResource.hpp>
#include <Engine/ResourceManager.hpp>
#include <Engine/Engine.hpp>

#include "TextureUtilities.h"

#include <gli/gli.hpp>

#include <lodepng.h>
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

		if (ext == ".ktx" || ext == ".dds") {
			LoadGli(source, resource);
		} else {
			LoadDiligent(source, resource);
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

	void TextureLoader::LoadGli(const std::string& source, TextureResource* resource) {
		auto device = mManager->GetParent()->GetDevice();

		gli::texture tex = gli::load(source);
		if (tex.empty()) {
			std::cout << "Failed to load texture " << source << "!" << std::endl;
			throw std::runtime_error("Failed to load texture!");
		}

		DG::TextureDesc desc;
		desc.Name = source.c_str();

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

		resource->mSource = source;
		resource->mTexture = out_texture;

		for (auto& data : expanded_datas) {
			delete[] data;
		}
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

	template <uint channels>
	void ImCpy(uint8_t* dest, uint8_t* src, uint pixel_count) {

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
				dest[ptr_dest++] = 1;
			}
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
}