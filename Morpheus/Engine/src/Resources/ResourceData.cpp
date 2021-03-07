#include <Engine/Resources/ResourceData.hpp>

namespace Morpheus {
	uint GetTypeSize(DG::VALUE_TYPE type) {
		switch (type) {
			case DG::VT_FLOAT16:
				return sizeof(DG::Uint16);
			case DG::VT_FLOAT32:
				return sizeof(DG::Float32);
			case DG::VT_INT32:
				return sizeof(DG::Int32);
			case DG::VT_INT16:
				return sizeof(DG::Int16);
			case DG::VT_INT8:
				return sizeof(DG::Int8);
			case DG::VT_UINT32:
				return sizeof(DG::Uint32);
			case DG::VT_UINT16:
				return sizeof(DG::Uint16);
			case DG::VT_UINT8:
				return sizeof(DG::Uint8);
			default:
				return 0;
		}
	}

	DG::VALUE_TYPE GetComponentType(DG::TEXTURE_FORMAT texFormat) {
		switch (texFormat) {
			case DG::TEX_FORMAT_RGBA32_FLOAT: 
				return DG::VT_FLOAT32;
			case DG::TEX_FORMAT_RGBA32_UINT:
				return DG::VT_UINT32;
			case DG::TEX_FORMAT_RGBA32_SINT:    								
				return DG::VT_INT32;
			case DG::TEX_FORMAT_RGB32_FLOAT:  
				return DG::VT_FLOAT32;
			case DG::TEX_FORMAT_RGB32_UINT:  
				return DG::VT_UINT32;
			case DG::TEX_FORMAT_RGB32_SINT:  
				return DG::VT_INT32;
			case DG::TEX_FORMAT_RGBA16_FLOAT: 
				return DG::VT_FLOAT16;
			case DG::TEX_FORMAT_RGBA16_UNORM: 
				return DG::VT_UINT16;
			case DG::TEX_FORMAT_RGBA16_UINT: 
				return DG::VT_UINT16;
			case DG::TEX_FORMAT_RGBA16_SNORM: 
				return DG::VT_UINT16;
			case DG::TEX_FORMAT_RGBA16_SINT: 
				return DG::VT_UINT16;
			case DG::TEX_FORMAT_RG32_FLOAT: 
				return DG::VT_FLOAT32;
			case DG::TEX_FORMAT_RG32_UINT: 
				return DG::VT_UINT32;
			case DG::TEX_FORMAT_RG32_SINT: 
				return DG::VT_INT32;
			case DG::TEX_FORMAT_RGBA8_UNORM:
				return DG::VT_UINT8;
			case DG::TEX_FORMAT_RGBA8_UNORM_SRGB:
				return DG::VT_UINT8;
			case DG::TEX_FORMAT_RGBA8_UINT: 
				return DG::VT_UINT8;
			case DG::TEX_FORMAT_RGBA8_SNORM: 
				return DG::VT_INT8;
			case DG::TEX_FORMAT_RGBA8_SINT: 
				return DG::VT_INT8;
			case DG::TEX_FORMAT_RG16_FLOAT: 
				return DG::VT_FLOAT16;
			case DG::TEX_FORMAT_RG16_UNORM: 
				return DG::VT_UINT16;
			case DG::TEX_FORMAT_RG16_UINT: 
				return DG::VT_UINT16;
			case DG::TEX_FORMAT_RG16_SNORM: 
				return DG::VT_INT16;
			case DG::TEX_FORMAT_RG16_SINT:
				return DG::VT_INT16;
			case DG::TEX_FORMAT_D32_FLOAT: 
				return DG::VT_FLOAT32;
			case DG::TEX_FORMAT_R32_FLOAT: 
				return DG::VT_FLOAT32;
			case DG::TEX_FORMAT_R32_UINT: 
				return DG::VT_UINT32;
			case DG::TEX_FORMAT_R32_SINT: 
				return DG::VT_INT32;
			case DG::TEX_FORMAT_RG8_UNORM: 
				return DG::VT_UINT8;
			case DG::TEX_FORMAT_RG8_UINT: 
				return DG::VT_UINT8;
			case DG::TEX_FORMAT_RG8_SNORM: 
				return DG::VT_INT8;
			case DG::TEX_FORMAT_RG8_SINT: 
				return DG::VT_INT8;
			case DG::TEX_FORMAT_R16_FLOAT: 
				return DG::VT_FLOAT16;
			case DG::TEX_FORMAT_D16_UNORM: 
				return DG::VT_UINT16;
			case DG::TEX_FORMAT_R16_UNORM: 
				return DG::VT_UINT16;
			case DG::TEX_FORMAT_R16_UINT: 
				return DG::VT_UINT16;
			case DG::TEX_FORMAT_R16_SNORM: 
				return DG::VT_INT16;
			case DG::TEX_FORMAT_R16_SINT: 
				return DG::VT_INT16;
			case DG::TEX_FORMAT_R8_UNORM: 
				return DG::VT_UINT8;
			case DG::TEX_FORMAT_R8_UINT: 
				return DG::VT_UINT8;
			case DG::TEX_FORMAT_R8_SNORM: 
				return DG::VT_INT8;
			case DG::TEX_FORMAT_R8_SINT: 
				return DG::VT_INT8;
			case DG::TEX_FORMAT_A8_UNORM:	
				return DG::VT_UINT8;
			case DG::TEX_FORMAT_RG8_B8G8_UNORM: 
				return DG::VT_UINT8;
			case DG::TEX_FORMAT_G8R8_G8B8_UNORM:
				return DG::VT_UINT8;
			default:
				return DG::VT_NUM_TYPES;
		}
	}

	int GetComponentCount(DG::TEXTURE_FORMAT texFormat) {
		switch (texFormat) {
			case DG::TEX_FORMAT_RGBA32_FLOAT: 
				return 4;
			case DG::TEX_FORMAT_RGBA32_UINT:
				return 4;
			case DG::TEX_FORMAT_RGBA32_SINT:    								
				return 4;
			case DG::TEX_FORMAT_RGB32_FLOAT:  
				return 3;
			case DG::TEX_FORMAT_RGB32_UINT:  
				return 3;
			case DG::TEX_FORMAT_RGB32_SINT:  
				return 3;
			case DG::TEX_FORMAT_RGBA16_FLOAT: 
				return 4;
			case DG::TEX_FORMAT_RGBA16_UNORM: 
				return 4;
			case DG::TEX_FORMAT_RGBA16_UINT: 
				return 4;
			case DG::TEX_FORMAT_RGBA16_SNORM: 
				return 4;
			case DG::TEX_FORMAT_RGBA16_SINT: 
				return 4;
			case DG::TEX_FORMAT_RG32_FLOAT: 
				return 2;
			case DG::TEX_FORMAT_RG32_UINT: 
				return 2;
			case DG::TEX_FORMAT_RG32_SINT: 
				return 2;
			case DG::TEX_FORMAT_RGBA8_UNORM:
				return 4;
			case DG::TEX_FORMAT_RGBA8_UNORM_SRGB:
				return 4;
			case DG::TEX_FORMAT_RGBA8_UINT: 
				return 4;
			case DG::TEX_FORMAT_RGBA8_SNORM: 
				return 4;
			case DG::TEX_FORMAT_RGBA8_SINT: 
				return 4;
			case DG::TEX_FORMAT_RG16_FLOAT: 
				return 2;
			case DG::TEX_FORMAT_RG16_UNORM: 
				return 2;
			case DG::TEX_FORMAT_RG16_UINT: 
				return 2;
			case DG::TEX_FORMAT_RG16_SNORM: 
				return 2;
			case DG::TEX_FORMAT_RG16_SINT:
				return 2;
			case DG::TEX_FORMAT_D32_FLOAT: 
				return 1;
			case DG::TEX_FORMAT_R32_FLOAT: 
				return 1;
			case DG::TEX_FORMAT_R32_UINT: 
				return 1;
			case DG::TEX_FORMAT_R32_SINT: 
				return 1;
			case DG::TEX_FORMAT_RG8_UNORM: 
				return 2;
			case DG::TEX_FORMAT_RG8_UINT: 
				return 2;
			case DG::TEX_FORMAT_RG8_SNORM: 
				return 2;
			case DG::TEX_FORMAT_RG8_SINT: 
				return 2;
			case DG::TEX_FORMAT_R16_FLOAT: 
				return 1;
			case DG::TEX_FORMAT_D16_UNORM: 
				return 1;
			case DG::TEX_FORMAT_R16_UNORM: 
				return 1;
			case DG::TEX_FORMAT_R16_UINT: 
				return 1;
			case DG::TEX_FORMAT_R16_SNORM: 
				return 1;
			case DG::TEX_FORMAT_R16_SINT: 
				return 1;
			case DG::TEX_FORMAT_R8_UNORM: 
				return 1;
			case DG::TEX_FORMAT_R8_UINT: 
				return 1;
			case DG::TEX_FORMAT_R8_SNORM: 
				return 1;
			case DG::TEX_FORMAT_R8_SINT: 
				return 1;
			case DG::TEX_FORMAT_A8_UNORM:	
				return 1;
			default:
				return -1;
		}
	}

	int GetSize(DG::VALUE_TYPE v) {
		switch (v) {
			case DG::VT_FLOAT32:
				return 4;
			case DG::VT_FLOAT16:
				return 2;
			case DG::VT_INT8:
				return 1;
			case DG::VT_INT16:
				return 2;
			case DG::VT_INT32:
				return 4;
			case DG::VT_UINT8:
				return 1;
			case DG::VT_UINT16:
				return 2;
			case DG::VT_UINT32:
				return 4;
			default:
				return -1;
		}
	}

	int GetPixelByteSize(DG::TEXTURE_FORMAT format) {
		auto count = GetComponentCount(format);
		if (count < 0)
			return -1;

		return GetTypeSize(GetComponentType(format)) * count;
	}

	bool GetIsNormalized(DG::TEXTURE_FORMAT format) {
		switch (format) {
			case DG::TEX_FORMAT_RGBA16_UNORM: 
				return true;
			case DG::TEX_FORMAT_RGBA16_SNORM: 
				return true;
			case DG::TEX_FORMAT_RGBA8_UNORM:
				return true;
			case DG::TEX_FORMAT_RGBA8_UNORM_SRGB:
				return true;
			case DG::TEX_FORMAT_RGBA8_SNORM: 
				return true;
			case DG::TEX_FORMAT_RG16_UNORM: 
				return true;
			case DG::TEX_FORMAT_RG16_SNORM: 
				return true;
			case DG::TEX_FORMAT_RG8_UNORM: 
				return true;
			case DG::TEX_FORMAT_RG8_SNORM: 
				return true;
			case DG::TEX_FORMAT_D16_UNORM: 
				return true;
			case DG::TEX_FORMAT_R16_UNORM: 
				return true;
			case DG::TEX_FORMAT_R16_SNORM: 
				return true;
			case DG::TEX_FORMAT_R8_UNORM: 
				return true;
			case DG::TEX_FORMAT_R8_SNORM: 
				return true;
			case DG::TEX_FORMAT_A8_UNORM:	
				return true;
			default:
				return false;
		}
	}

	bool GetIsSRGB(DG::TEXTURE_FORMAT format) {
		switch (format) {
			case DG::TEX_FORMAT_RGBA8_UNORM_SRGB:
				return true;
			default:
				return false;
		}
	}
}