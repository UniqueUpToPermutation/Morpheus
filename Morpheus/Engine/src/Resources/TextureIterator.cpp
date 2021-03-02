#include <Engine/Resources/TextureIterator.hpp>
#include <Engine/Resources/ResourceData.hpp>
#include <Engine/Resources/RawSampler.hpp>

namespace Morpheus {

 	DG::double3 GetPosition(const DG::uint4& coords, const DG::double3& scale) {
		return DG::double3(((double)coords.x + 0.5) * scale.x,
			((double)coords.y + 0.5) * scale.y,
			((double)coords.z + 0.5) * scale.z);
	}

	DG::double3 GetPositionCube(const DG::uint4& coords, const DG::double3& scale) {
		DG::double2 uv(((double)coords.x + 0.5) * scale.x,
			((double)coords.y + 0.5) * scale.y);

		uint face = coords.w % 6;

		DG::double3 result;
		CubemapHelper<double>::FromUV(uv.x, uv.y, face, &result.x, &result.y, &result.z);
		return result;
	}

	template <typename memT, bool bNormed, typename usrT>
	void WriteValue(uint8_t* dest, const usrT* src, int count) {
		memT* dest_ = (memT*)dest;
		for (int i = 0; i < count; ++i) {
			if constexpr (bNormed) {
				dest_[i] = (memT)(std::min<usrT>(std::max<usrT>(src[i], 0.0), 1.0) * (usrT)std::numeric_limits<memT>::max());
			} else {
				dest_[i] = (memT)(src[i]);
			}
		}
	}

	template <typename memT, bool bNormed, typename usrT>
	void ReadValue(const uint8_t* mem, usrT* dest, int count) {
		memT* mem_ = (memT*)mem;
		for (int i = 0; i < count; ++i) {
			if constexpr (bNormed) {
				dest[i] = (usrT)mem_[i] / (usrT)std::numeric_limits<memT>::max();
			} else {
				dest[i] = (usrT)mem_[i];
			}
		}
	}

	TextureIterator::TextureIterator(RawTexture* texture, 
		const DG::uint3& subBegin, const DG::uint3& subEnd, 
		uint sliceBegin, uint sliceEnd, uint mip) :
		mUnderlying(&texture->mData[0]),
		mMip(mip),
		mIterationBegin(subBegin.x, subBegin.y, subBegin.z, sliceBegin),
		mIterationEnd(subEnd.x, subEnd.y, subEnd.z, sliceEnd) {

		mIndexCoords = mIterationBegin;

		auto mip_levels = texture->GetMipCount();

		mPixelSize = GetPixelByteSize(texture->mDesc.Format);

		size_t currentOffset = 0;
		for (uint imip = 0; imip < mip_levels; ++imip) {
			if (imip == mip) {
				mMipOffset = currentOffset;
			}
		
			size_t mip_width = std::max(texture->mDesc.Width >> imip, 1u);
			size_t mip_height = std::max(texture->mDesc.Height >> imip, 1u);
			size_t mip_depth = std::max(texture->mDesc.Depth >> imip, 1u);

			currentOffset += mip_width * mip_height * mip_depth * mPixelSize;
		}

		mSliceStride = currentOffset;

		size_t mip_width = std::max(texture->mDesc.Width >> mip, 1u);
		size_t mip_height = std::max(texture->mDesc.Height >> mip, 1u);
		size_t mip_depth = std::max(texture->mDesc.Depth >> mip, 1u);

		mYStride = mip_width * mPixelSize;
		mZStride = mip_height * mip_width * mPixelSize;

		mScale.x = 1.0 / (double)mip_width;
		mScale.y = 1.0 / (double)mip_height;
		mScale.z = 1.0 / (double)mip_depth;

		UpdateGridValue(mIndexCoords);

		if (texture->mDesc.Type == DG::RESOURCE_DIM_TEX_CUBE || 
			texture->mDesc.Type == DG::RESOURCE_DIM_TEX_CUBE_ARRAY) {
			mIndexToPosition = &GetPositionCube;
		} else {
			mIndexToPosition = &GetPosition;
		}

		DG::VALUE_TYPE val = GetComponentType(texture->mDesc.Format);
		bool bNormed = GetIsNormalized(texture->mDesc.Format);
		mValue.mChannelCount = GetComponentCount(texture->mDesc.Format);

		switch (val) {
			case DG::VT_FLOAT32:
				mValue.mReadFuncD = &ReadValue<DG::Float32, false, double>;
				mValue.mWriteFuncD = &WriteValue<DG::Float32, false, double>;
				mValue.mReadFuncF = &ReadValue<DG::Float32, false, float>;
				mValue.mWriteFuncF = &WriteValue<DG::Float32, false, float>;
				break;
			case DG::VT_FLOAT16:
				throw std::runtime_error("VT_FLOAT16 formats not allowed");
			case DG::VT_UINT8:
				if (!bNormed) {
					mValue.mReadFuncD = &ReadValue<DG::Uint8, false, double>;
					mValue.mWriteFuncD = &WriteValue<DG::Uint8, false, double>;
					mValue.mReadFuncF = &ReadValue<DG::Uint8, false, float>;
					mValue.mWriteFuncF = &WriteValue<DG::Uint8, false, float>;
				} else {
					mValue.mReadFuncD = &ReadValue<DG::Uint8, true, double>;
					mValue.mWriteFuncD = &WriteValue<DG::Uint8, true, double>;
					mValue.mReadFuncF = &ReadValue<DG::Uint8, true, float>;
					mValue.mWriteFuncF = &WriteValue<DG::Uint8, true, float>;
				}
				break;
			case DG::VT_INT8:
				if (!bNormed) {
					mValue.mReadFuncD = &ReadValue<DG::Int8, false, double>;
					mValue.mWriteFuncD = &WriteValue<DG::Int8, false, double>;
					mValue.mReadFuncF = &ReadValue<DG::Int8, false, float>;
					mValue.mWriteFuncF = &WriteValue<DG::Int8, false, float>;
				} else {
					mValue.mReadFuncD = &ReadValue<DG::Int8, true, double>;
					mValue.mWriteFuncD = &WriteValue<DG::Int8, true, double>;
					mValue.mReadFuncF = &ReadValue<DG::Int8, true, float>;
					mValue.mWriteFuncF = &WriteValue<DG::Int8, true, float>;
				}
				break;
			case DG::VT_UINT16:
				if (!bNormed) {
					mValue.mReadFuncD = &ReadValue<DG::Uint16, false, double>;
					mValue.mWriteFuncD = &WriteValue<DG::Uint16, false, double>;
					mValue.mReadFuncF = &ReadValue<DG::Uint16, false, float>;
					mValue.mWriteFuncF = &WriteValue<DG::Uint16, false, float>;
				} else {
					mValue.mReadFuncD = &ReadValue<DG::Uint16, true, double>;
					mValue.mWriteFuncD = &WriteValue<DG::Uint16, true, double>;
					mValue.mReadFuncF = &ReadValue<DG::Uint16, true, float>;
					mValue.mWriteFuncF = &WriteValue<DG::Uint16, true, float>;
				}
				break;
			case DG::VT_INT16:
				if (!bNormed) {
					mValue.mReadFuncD = &ReadValue<DG::Int16, false, double>;
					mValue.mWriteFuncD = &WriteValue<DG::Int16, false, double>;
					mValue.mReadFuncF = &ReadValue<DG::Int16, false, float>;
					mValue.mWriteFuncF = &WriteValue<DG::Int16, false, float>;
				} else {
					mValue.mReadFuncD = &ReadValue<DG::Int16, true, double>;
					mValue.mWriteFuncD = &WriteValue<DG::Int16, true, double>;
					mValue.mReadFuncF = &ReadValue<DG::Int16, true, float>;
					mValue.mWriteFuncF = &WriteValue<DG::Int16, true, float>;
				}
				break;
			case DG::VT_UINT32:
				mValue.mReadFuncD = &ReadValue<DG::Uint32, false, double>;
				mValue.mWriteFuncD = &WriteValue<DG::Uint32, false, double>;
				mValue.mReadFuncF = &ReadValue<DG::Uint32, false, float>;
				mValue.mWriteFuncF = &WriteValue<DG::Uint32, false, float>;
				break;
			case DG::VT_INT32:
				mValue.mReadFuncD = &ReadValue<DG::Int32, false, double>;
				mValue.mWriteFuncD = &WriteValue<DG::Int32, false, double>;
				mValue.mReadFuncF = &ReadValue<DG::Int32, false, float>;
				mValue.mWriteFuncF = &WriteValue<DG::Int32, false, float>;
				break;
			default:
				throw std::runtime_error("TextureIterator does not support given format!");
		}
	}

	void TextureIterator::UpdateGridValue(const DG::uint4& coords) {
		size_t index = coords.w * mSliceStride +
			mMipOffset + coords.z * mZStride + coords.y * mYStride + coords.x * mPixelSize;
		mValue.mMemory = &mUnderlying[index];
	}

	void TextureIterator::Next() {
		mIndexCoords.x++;
		if (mIndexCoords.x == mIterationEnd.x) {

			mIndexCoords.x = mIterationBegin.x;
			mIndexCoords.y++;

			if (mIndexCoords.y == mIterationEnd.y) {

				mIndexCoords.y = mIterationBegin.y;
				mIndexCoords.z++;

				if (mIndexCoords.z == mIterationEnd.z) {

					mIndexCoords.z = mIterationBegin.z;
					mIndexCoords.w++;

					if (mIndexCoords.w == mIterationEnd.w) {
						bFinished = true;
						mIndexCoords = mIterationBegin;
					}
				} 
			}
		} 

		UpdateGridValue(mIndexCoords);
	}
}