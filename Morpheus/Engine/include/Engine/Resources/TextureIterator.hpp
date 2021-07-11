#pragma once

#include <Engine/Resources/Texture.hpp>

#include "BasicMath.hpp"

namespace Morpheus {

	template <typename T>
	using grid_value_write_t = void(*)(uint8_t* dest, const T* src, int count);
	template <typename T>
	using grid_value_read_t = void(*)(const uint8_t* mem, T* dest, int count);

	typedef DG::double3(*grid_index_to_position_t)(const DG::uint4&, const DG::double3&);

	class GridValue {
	private:
		uint8_t* mMemory;
		uint mChannelCount;

		grid_value_write_t<float> mWriteFuncF;
		grid_value_read_t<float> mReadFuncF;
		grid_value_write_t<double> mWriteFuncD;
		grid_value_read_t<double> mReadFuncD;

		template <uint channelCount>
		inline void Write(const float value[]) {
			uint channels = std::min(channelCount, mChannelCount);
			mWriteFuncF(mMemory, value, channels);
		}

		template <uint channelCount>
		inline void Read(float value[]) {
			uint channels = std::min(channelCount, mChannelCount);
			mReadFuncF(mMemory, value, channels);

			for (uint i = channelCount; i < mChannelCount; i++) {
				value[i] = 0.0f;
			}
		}

		template <uint channelCount>
		inline void Write(const double value[]) {
			uint channels = std::min(channelCount, mChannelCount);
			mWriteFuncD(mMemory, value, channels);
		}

		template <uint channelCount>
		inline void Read(double value[]) {
			uint channels = std::min(channelCount, mChannelCount);
			mReadFuncD(mMemory, value, channels);

			for (uint i = channelCount; i < mChannelCount; i++) {
				value[i] = 0.0;
			}
		}

	public:
		inline void Write(const float v) {
			Write<1>(&v);
		}

		inline void Write(const DG::float2& v) {
			Write<2>(v.Data());
		}

		inline void Write(const DG::float3& v) {
			Write<3>(v.Data());
		}

		inline void Write(const DG::float4& v) {
			Write<4>(v.Data());
		}

		inline void Read(float* v) {
			Read<1>(v);
		}

		inline void Read(DG::float2* v) {
			Read<2>(v->Data());
		}

		inline void Read(DG::float3* v) {
			Read<3>(v->Data());
		}

		inline void Read(DG::float4* v) {
			Read<4>(v->Data());
		}

		inline void Write(const double v) {
			Write<1>(&v);
		}

		inline void Write(const DG::double2& v) {
			Write<2>(v.Data());
		}

		inline void Write(const DG::double3& v) {
			Write<3>(v.Data());
		}

		inline void Write(const DG::double4& v) {
			Write<4>(v.Data());
		}

		inline void Read(double* v) {
			Read<1>(v);
		}

		inline void Read(DG::double2* v) {
			Read<2>(v->Data());
		}

		inline void Read(DG::double3* v) {
			Read<3>(v->Data());
		}

		inline void Read(DG::double4* v) {
			Read<4>(v->Data());
		}

		friend class TextureIterator;
	};

	class TextureIterator {
	private:
		uint8_t* mUnderlying;
		GridValue mValue;
		DG::double3 mPosition;

		bool bFinished = false;
		
		DG::uint4 mIndexCoords;
		DG::uint4 mIterationBegin;
		DG::uint4 mIterationEnd;
		uint mMip;

		DG::double3 mScale;

		size_t mYStride;
		size_t mZStride;
		size_t mPixelSize;
		size_t mMipOffset;
		size_t mSliceStride;

		grid_index_to_position_t mIndexToPosition;

		void UpdateGridValue(const DG::uint4& coords);

	public:
		TextureIterator();

		TextureIterator(Texture* texture, 
			const DG::uint3& subBegin, const DG::uint3& subEnd, 
			uint sliceBegin, uint sliceEnd, uint mip = 0);

		TextureIterator(Texture* texture, 
			const DG::uint3& subBegin, const DG::uint3& subEnd, 
			uint mip = 0) : 
				TextureIterator(texture, subBegin, subEnd, 
					0, texture->mRawAspect.mDesc.ArraySize, mip) {
		}

		TextureIterator(Texture* texture, 
			const DG::uint2& subBegin, const DG::uint2& subEnd, 
			uint mip = 0) :
			TextureIterator(texture, 
				DG::uint3(subBegin.x, subBegin.y, 0), 
				DG::uint3(subEnd.x, subEnd.y, 
					std::max<uint>(texture->mRawAspect.mDesc.Depth >> mip, 1u)), 
				0, texture->mRawAspect.mDesc.ArraySize, mip) {
		}

		TextureIterator(Texture* texture, 
			uint subBegin, uint subEnd, 
			uint mip = 0) :
			TextureIterator(texture, 
				DG::uint3(subBegin, 0, 0), 
				DG::uint3(subEnd, std::max<uint>(texture->mRawAspect.mDesc.Height >> mip, 1u), 
					std::max<uint>(texture->mRawAspect.mDesc.Depth >> mip, 1u)), 
				0, texture->mRawAspect.mDesc.ArraySize, mip) {
		}

		TextureIterator(Texture* texture, 
			uint mip = 0) :
			TextureIterator(texture, 
				DG::uint3(0, 0, 0), 
				DG::uint3(std::max<uint>(texture->mRawAspect.mDesc.Width >> mip, 1u), 
					std::max<uint>(texture->mRawAspect.mDesc.Height >> mip, 1u), 
					std::max<uint>(texture->mRawAspect.mDesc.Depth >> mip, 1u)), 
				0, texture->mRawAspect.mDesc.ArraySize, mip) {
		}

		inline GridValue& Value() {
			return mValue;
		}

		inline DG::double3 Position() const {
			return mIndexToPosition(mIndexCoords, mScale);
		}

		inline DG::uint3 Index() const {
			return mIndexCoords;
		}

		inline uint Slice() const {
			return mIndexCoords.w;
		}

		inline bool IsValid() const {
			return !bFinished;
		}

		void Next();
	};
}