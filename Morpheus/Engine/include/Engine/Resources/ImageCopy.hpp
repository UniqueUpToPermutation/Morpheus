#pragma once

#include "GraphicsTypes.h"

#include <iostream>

namespace DG = Diligent;

namespace Morpheus {
	template <uint channels, typename T>
	void ImageCopyBasic(T* dest, T* src, size_t pixel_count) {

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

	template <typename destT, typename srcT>
	inline destT ImValueConvert(srcT t) {
		if constexpr (std::is_same<destT, float>::value !=
			std::is_same<srcT, float>::value) {
			if constexpr (std::is_same<destT, float>::value) {
				return ((destT)t / 255.0f);
			} else {
				return (destT)(std::max(t, 1.0f) * 255.0f);
			}
		} else {
			return (destT)t;
		}
	}

	template <typename destT, typename srcT, uint destChannels, uint srcChannels>
	void ImageCopy(destT* dest, srcT* src, size_t pixel_count) {

		uint ptr_dest = 0;
		uint ptr_src = 0;
		for (uint i = 0; i < pixel_count; ++i) {
			dest[ptr_dest++] = ImValueConvert<destT, srcT>(src[ptr_src++]);

			if constexpr (srcChannels > 1 && destChannels > 1) {
				dest[ptr_dest++] = ImValueConvert<destT, srcT>(src[ptr_src++]);
			}
			else if (destChannels > 1) {
				dest[ptr_dest++] = 0;
			}

			if constexpr (srcChannels > 2 && destChannels > 2) {
				dest[ptr_dest++] = ImValueConvert<destT, srcT>(src[ptr_src++]);
			}
			else if (destChannels > 2) {
				dest[ptr_dest++] = 0;
			}

			if constexpr (srcChannels > 3 && destChannels > 3) {
				dest[ptr_dest++] = ImValueConvert<destT, srcT>(src[ptr_src++]);
			}
			else if (destChannels > 3) {
				if constexpr (std::is_same<destT, uint8_t>::value) {
					dest[ptr_dest++] = 255u;
				} else if constexpr (std::is_same<destT, float>::value) {
					dest[ptr_dest++] = 1.0f;
				} else {
					dest[ptr_dest++] = 0;
				}
			}
		}
	}

	template <typename destT, typename srcT, uint destChannels>
	void ImageCopy(destT* dest, srcT* src, size_t pixel_count, uint srcChannels) {
		switch (srcChannels) {
		case 1:
			ImageCopy<destT, srcT, destChannels, 1>(dest, src, pixel_count);
			break;
		case 2:
			ImageCopy<destT, srcT, destChannels, 2>(dest, src, pixel_count);
			break;
		case 3:
			ImageCopy<destT, srcT, destChannels, 3>(dest, src, pixel_count);
			break;
		case 4:
			ImageCopy<destT, srcT, destChannels, 4>(dest, src, pixel_count);
			break;
		default:
			throw std::runtime_error("Incorrect number of channels!");
		}
	}

	template <typename destT, uint destChannels>
	void ImageCopy(destT* dest, void* src, size_t pixel_count, 
		uint srcChannels, DG::VALUE_TYPE srcType) {
		switch (srcType) {
		case DG::VT_FLOAT32:
			ImageCopy<destT, DG::Float32, destChannels>(dest, (DG::Float32*)src, pixel_count, srcChannels);
			break;
		case DG::VT_FLOAT16:
			if constexpr (std::is_same_v<destT, DG::Uint16>) {
				ImageCopy<destT, DG::Uint16, destChannels>(dest, (DG::Uint16*)src, pixel_count, srcChannels);
			} else if constexpr (std::is_same_v<destT, DG::Int16>) {
				ImageCopy<destT, DG::Int16, destChannels>(dest, (DG::Int16*)src, pixel_count, srcChannels);
			} else {
				throw std::runtime_error("For Float16 type, destination must be 16-bit integer");
			}
			break;
		case DG::VT_INT32:
			ImageCopy<destT, DG::Int32, destChannels>(dest, (DG::Int32*)src, pixel_count, srcChannels);
			break;
		case DG::VT_INT16:
			ImageCopy<destT, DG::Int16, destChannels>(dest, (DG::Int16*)src, pixel_count, srcChannels);
			break;
		case DG::VT_INT8:
			ImageCopy<destT, DG::Int8, destChannels>(dest, (DG::Int8*)src, pixel_count, srcChannels);
			break;
		case DG::VT_UINT32:
			ImageCopy<destT, DG::Uint32, destChannels>(dest, (DG::Uint32*)src, pixel_count, srcChannels);
			break;
		case DG::VT_UINT16:
			ImageCopy<destT, DG::Uint16, destChannels>(dest, (DG::Uint16*)src, pixel_count, srcChannels);
			break;
		case DG::VT_UINT8:
			ImageCopy<destT, DG::Uint8, destChannels>(dest, (DG::Uint8*)src, pixel_count, srcChannels);
			break;
		default:
			throw std::runtime_error("srcType not recognized!");
		}
	}

	template <uint destChannels>
	void ImageCopy(void* dest, void* src, size_t pixel_count, 
		uint srcChannels, DG::VALUE_TYPE destType, DG::VALUE_TYPE srcType) {
		switch (destType) {
			case DG::VT_FLOAT32:
			ImageCopy<DG::Float32, destChannels>((DG::Float32*)dest, src, pixel_count, srcChannels, srcType);
			break;
		case DG::VT_FLOAT16:
			if (destType != srcType) {
				throw std::runtime_error("Cannot auto convert between VT_FLOAT16 and other data types!");
			}
			ImageCopy<DG::Uint16, destChannels>((DG::Uint16*)dest, src, pixel_count, srcChannels, srcType);
			break;
		case DG::VT_INT32:
			ImageCopy<DG::Int32, destChannels>((DG::Int32*)dest, src, pixel_count, srcChannels, srcType);
			break;
		case DG::VT_INT16:
			ImageCopy<DG::Int16, destChannels>((DG::Int16*)dest, src, pixel_count, srcChannels, srcType);
			break;
		case DG::VT_INT8:
			ImageCopy<DG::Int8, destChannels>((DG::Int8*)dest, src, pixel_count, srcChannels, srcType);
			break;
		case DG::VT_UINT32:
			ImageCopy<DG::Uint32, destChannels>((DG::Uint32*)dest, src, pixel_count, srcChannels, srcType);
			break;
		case DG::VT_UINT16:
			ImageCopy<DG::Uint16, destChannels>((DG::Uint16*)dest, src, pixel_count, srcChannels, srcType);
			break;
		case DG::VT_UINT8:
			ImageCopy<DG::Uint8, destChannels>((DG::Uint8*)dest, src, pixel_count, srcChannels, srcType);
			break;
		default:
			throw std::runtime_error("destType not recognized!");
		}
	}

	void ImageCopy(void* dest, void* src, size_t pixel_count, uint destChannels, 
		uint srcChannels, DG::VALUE_TYPE destType, DG::VALUE_TYPE srcType) {
		switch (destChannels) {
		case 1:
			ImageCopy<1>(dest, src, pixel_count, srcChannels, destType, srcType);
			break;
		case 2:
			ImageCopy<2>(dest, src, pixel_count, srcChannels, destType, srcType);
			break;
		case 3:
			ImageCopy<3>(dest, src, pixel_count, srcChannels, destType, srcType);
			break;
		case 4:
			ImageCopy<4>(dest, src, pixel_count, srcChannels, destType, srcType);
			break;
		default:
			throw std::runtime_error("Incorrect number of channels!");
		}
	}
}