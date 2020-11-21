#include <Engine/TextureResource.hpp>
#include <Engine/ResourceManager.hpp>
#include <Engine/Engine.hpp>

#include "TextureUtilities.h"

#include <gli/gli.hpp>
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

	DG::TEXTURE_FORMAT ConvertInternalTexFormat(gli::gl::internal_format format) {
		switch (format) {
			case gli::gl::INTERNAL_SRGB8_ALPHA8:
				return DG::TEX_FORMAT_RGBA8_UNORM_SRGB;
			case gli::gl::INTERNAL_SRGB8:
				return DG::TEX_FORMAT_RGBA8_UNORM_SRGB;
			case gli::gl::INTERNAL_RGB8_UNORM:
				return DG::TEX_FORMAT_RGBA8_UNORM;
			case gli::gl::INTERNAL_RGBA8_UNORM:
				return DG::TEX_FORMAT_RGBA8_UNORM;
			case gli::gl::INTERNAL_RGBA8_SNORM:
				return DG::TEX_FORMAT_RGBA8_SNORM;
			case gli::gl::INTERNAL_RG8_SNORM:
				return DG::TEX_FORMAT_RG8_SNORM;
			case gli::gl::INTERNAL_RG8_UNORM:
				return DG::TEX_FORMAT_RG8_UNORM;
			case gli::gl::INTERNAL_R8_SNORM:
				return DG::TEX_FORMAT_R8_SNORM;
			case gli::gl::INTERNAL_R8_UNORM:
				return DG::TEX_FORMAT_R8_UNORM;
			case gli::gl::INTERNAL_RGBA16_SNORM:
				return DG::TEX_FORMAT_RGBA16_SNORM;
			case gli::gl::INTERNAL_RGBA16_UNORM:
				return DG::TEX_FORMAT_RGBA16_UNORM;
			case gli::gl::INTERNAL_RGBA16F:
				return DG::TEX_FORMAT_RGBA16_FLOAT;
			case gli::gl::INTERNAL_RG16_SNORM:
				return DG::TEX_FORMAT_RG16_SNORM;
			case gli::gl::INTERNAL_RG16_UNORM:
				return DG::TEX_FORMAT_RG16_UNORM;
			case gli::gl::INTERNAL_RG16F:
				return DG::TEX_FORMAT_RG16_FLOAT;
			case gli::gl::INTERNAL_R16_SNORM:
				return DG::TEX_FORMAT_R16_SNORM;
			case gli::gl::INTERNAL_R16_UNORM:
				return DG::TEX_FORMAT_R16_UNORM;
			case gli::gl::INTERNAL_R16F:
				return DG::TEX_FORMAT_R16_FLOAT;
			case gli::gl::INTERNAL_RGBA32F:
				return DG::TEX_FORMAT_RGBA32_FLOAT;
			case gli::gl::INTERNAL_RG32F:
				return DG::TEX_FORMAT_RG32_FLOAT;
			case gli::gl::INTERNAL_R32F:
				return DG::TEX_FORMAT_R32_FLOAT;
			default:
				throw std::runtime_error("Unsupported KTX format!");
		}
	}

	DG::RESOURCE_DIMENSION ConvertResourceDimension(gli::gl::target target) {
		switch (target) {
			case gli::gl::TARGET_1D:
				return DG::RESOURCE_DIM_TEX_1D;
			case gli::gl::TARGET_1D_ARRAY:
				return DG::RESOURCE_DIM_TEX_1D_ARRAY;
			case gli::gl::TARGET_2D:
				return DG::RESOURCE_DIM_TEX_2D;
			case gli::gl::TARGET_2D_ARRAY:
				return DG::RESOURCE_DIM_TEX_2D_ARRAY;
			case gli::gl::TARGET_CUBE:
				return DG::RESOURCE_DIM_TEX_CUBE;
			case gli::gl::TARGET_CUBE_ARRAY:
				return DG::RESOURCE_DIM_TEX_CUBE_ARRAY;
			case gli::gl::TARGET_3D:
				return DG::RESOURCE_DIM_TEX_3D;
			default:
				throw std::runtime_error("Unsupported KTX target!");
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

		gli::gl GL(gli::gl::PROFILE_GL33);
		gli::gl::format Format = GL.translate(tex.format(), tex.swizzles());
		gli::gl::target Target = GL.translate(tex.target());

		DG::TEXTURE_FORMAT internal_format = ConvertInternalTexFormat(Format.Internal);

		DG::TextureDesc desc;
		desc.Name = source.c_str();
		
		desc.BindFlags = DG::BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = DG::CPU_ACCESS_NONE;
		desc.Format = internal_format;
		desc.Width = tex.extent().x;
		desc.Height = tex.extent().y;
		if (Target == gli::gl::TARGET_3D) {
			desc.Depth = tex.extent().z;
		} else {
			desc.ArraySize = tex.layers() * tex.faces();
		}
		desc.MipLevels = tex.levels();
		desc.Usage = DG::USAGE_IMMUTABLE;
		desc.Type = ConvertResourceDimension(Target);

		std::vector<DG::TextureSubResData> subData;

		size_t block_size = gli::block_size(tex.format());

		std::vector<uint8_t*> expanded_datas;

		bool bExpand = false;

		if (Format.Internal == gli::gl::INTERNAL_RGB8_UNORM ||
			Format.Internal == gli::gl::INTERNAL_SRGB8) {
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
					if (Target == gli::gl::TARGET_3D) {
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
}