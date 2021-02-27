#include <Engine/Resources/RawSampler.hpp>
#include <Engine/Resources/ResourceData.hpp>

namespace Morpheus {
		template <typename ReturnType,
		typename IndexType, 
		typename SourceType,
		bool bSourceNormalized>
	class ArraySurfaceIndexer {
	private:
		static_assert(std::is_same_v<ReturnType, float> || 
			std::is_same_v<ReturnType, double>, 
			"ReturnType must be floating point type!");

		SourceType* mBackend;
		std::vector<size_t> mMipSizes;
		std::vector<uint> mMipWidths;
		std::vector<uint> mMipHeights;
		std::vector<uint> mMipDepths;
		std::vector<size_t> mMipOffsets;
		std::vector<IndexType> mMipPixelArea;
		uint mWidth;
		uint mHeight;
		uint mDepth;
		uint mMips;
		uint mSlices;
		uint mChannelCount;
		size_t mSliceSize;

		SurfaceWrapping mWrapTypeX;
		SurfaceWrapping mWrapTypeY;
		SurfaceWrapping mWrapTypeZ;
		DG::RESOURCE_DIMENSION mResourceDim;

		struct CubemapBoundary {
			uint32_t mFace;
			uint32_t mSide;
			bool bFlip;
		};

		CubemapBoundary mCubemapBoundaries[6][4];

		constexpr static int BORDER_TOP = 		CubemapHelper<IndexType>::BORDER_TOP;
		constexpr static int BORDER_BOTTOM = 	CubemapHelper<IndexType>::BORDER_BOTTOM;
		constexpr static int BORDER_LEFT = 		CubemapHelper<IndexType>::BORDER_LEFT;
		constexpr static int BORDER_RIGHT = 	CubemapHelper<IndexType>::BORDER_RIGHT;
		constexpr static int BORDER_NONE = -1;

		constexpr static int FACE_POSITIVE_X = 	CubemapHelper<IndexType>::FACE_POSITIVE_X;
		constexpr static int FACE_NEGATIVE_X = 	CubemapHelper<IndexType>::FACE_NEGATIVE_X;
		constexpr static int FACE_POSITIVE_Y = 	CubemapHelper<IndexType>::FACE_POSITIVE_Y;
		constexpr static int FACE_NEGATIVE_Y = 	CubemapHelper<IndexType>::FACE_NEGATIVE_Y;
		constexpr static int FACE_POSITIVE_Z = 	CubemapHelper<IndexType>::FACE_POSITIVE_Z;
		constexpr static int FACE_NEGATIVE_Z = 	CubemapHelper<IndexType>::FACE_NEGATIVE_Z;
		constexpr static int FACE_NONE = -1;

	public:
		inline uint GetMipCount() const {
			return mMips;
		}

		inline uint GetMipWidth(uint mip) const {
			return mMipWidths[mip];
		}

		inline uint GetMipHeight(uint mip) const {
			return mMipHeights[mip];
		}
	
		inline uint GetMipDepth(uint mip) const {
			return mMipDepths[mip];
		}

		inline IndexType GetMipTexelArea(uint mip) const {
			return mMipPixelArea[mip];
		}

		inline ReturnType Convert(SourceType value) const {
			if constexpr (std::is_same_v<double, SourceType> || std::is_same_v<float, SourceType>) {
				return (ReturnType)value;
			} else if constexpr (bSourceNormalized) {
				return (ReturnType)value / (ReturnType) std::numeric_limits<SourceType>::max();
			} else {
				return (ReturnType)value;
			}
		}

		ArraySurfaceIndexer(const SourceType* backend, 
			uint width, 
			uint height, 
			uint depth,
			uint slices,
			uint mips,
			uint channelCount,
			SurfaceWrapping wrapX,
			SurfaceWrapping wrapY,
			SurfaceWrapping wrapZ,
			DG::RESOURCE_DIMENSION resourceDim) :
			mWidth(width),
			mHeight(height),
			mDepth(depth),
			mSlices(slices),
			mMips(mips),
			mChannelCount(channelCount),
			mWrapTypeX(wrapX),
			mWrapTypeY(wrapY),
			mWrapTypeZ(wrapZ),
			mResourceDim(resourceDim) {
			
			mMipSizes.reserve(mMips);
			mMipWidths.reserve(mMips);
			mMipHeights.reserve(mMips);
			mMipDepths.reserve(mMips);
			mMipOffsets.reserve(mMips);
			mMipPixelArea.reserve(mMips);

			for (uint imip = 0; imip < mMips; ++imip) {
				size_t mip_width = std::max(width >> imip, 1u);
				size_t mip_height = std::max(height >> imip, 1u);
				size_t mip_depth = std::max(depth >> imip, 1u);

				size_t mip_size = mChannelCount * mip_width * mip_height * mip_depth;

				IndexType mip_width_inv = 1.0 / (IndexType)mip_width;
				IndexType mip_height_inv = 1.0 / (IndexType)mip_height;
				IndexType mip_depth_inv = 1.0 / (IndexType)mip_depth;

				mMipPixelArea.emplace_back(mip_width_inv * mip_height_inv * mip_depth_inv);
				mMipSizes.emplace_back(mip_size);
				mMipWidths.emplace_back((uint)mip_width);
				mMipHeights.emplace_back((uint)mip_height);
				mMipDepths.emplace_back((uint)mip_depth);
			}

			mSliceSize = 0;
			for (uint imip = 0; imip < mMips; ++imip) {
				mMipOffsets.emplace_back(mSliceSize);
				mSliceSize += mMipSizes[imip];
			}

			// Create the edge map
			mCubemapBoundaries[FACE_POSITIVE_X][BORDER_TOP] =
				{ FACE_POSITIVE_Y, BORDER_RIGHT, true };
			mCubemapBoundaries[FACE_POSITIVE_X][BORDER_BOTTOM] = 
				{ FACE_NEGATIVE_Y, BORDER_RIGHT, false };
			mCubemapBoundaries[FACE_POSITIVE_X][BORDER_LEFT] = 
				{ FACE_POSITIVE_Z, BORDER_RIGHT, false };
			mCubemapBoundaries[FACE_POSITIVE_X][BORDER_RIGHT] = 
				{ FACE_NEGATIVE_Z, BORDER_LEFT, false };

			mCubemapBoundaries[FACE_NEGATIVE_X][BORDER_TOP] = 
				{ FACE_POSITIVE_Y, BORDER_LEFT, false };
			mCubemapBoundaries[FACE_NEGATIVE_X][BORDER_BOTTOM] = 
				{ FACE_NEGATIVE_Y, BORDER_LEFT, true };
			mCubemapBoundaries[FACE_NEGATIVE_X][BORDER_LEFT] = 
				{ FACE_NEGATIVE_Z, BORDER_RIGHT, false };
			mCubemapBoundaries[FACE_NEGATIVE_X][BORDER_RIGHT] = 
				{ FACE_POSITIVE_Z, BORDER_LEFT, false };

			mCubemapBoundaries[FACE_POSITIVE_Y][BORDER_TOP] = 
				{ FACE_NEGATIVE_Z, BORDER_TOP, true };
			mCubemapBoundaries[FACE_POSITIVE_Y][BORDER_BOTTOM] = 
				{ FACE_POSITIVE_Z, BORDER_TOP, false };
			mCubemapBoundaries[FACE_POSITIVE_Y][BORDER_LEFT] = 
				{ FACE_NEGATIVE_X, BORDER_TOP, false };
			mCubemapBoundaries[FACE_POSITIVE_Y][BORDER_RIGHT] = 
				{ FACE_POSITIVE_X, BORDER_TOP, true};

			mCubemapBoundaries[FACE_NEGATIVE_Y][BORDER_TOP] = 
				{ FACE_POSITIVE_Z, BORDER_BOTTOM, false };
			mCubemapBoundaries[FACE_NEGATIVE_Y][BORDER_BOTTOM] = 
				{ FACE_NEGATIVE_Z, BORDER_BOTTOM, true };
			mCubemapBoundaries[FACE_NEGATIVE_Y][BORDER_LEFT] = 
				{ FACE_NEGATIVE_X, BORDER_BOTTOM, true, };
			mCubemapBoundaries[FACE_NEGATIVE_Y][BORDER_RIGHT] = 
				{ FACE_POSITIVE_X, BORDER_BOTTOM, false };

			mCubemapBoundaries[FACE_POSITIVE_Z][BORDER_TOP] = 
				{ FACE_POSITIVE_Y, BORDER_BOTTOM, false };
			mCubemapBoundaries[FACE_POSITIVE_Z][BORDER_BOTTOM] = 
				{ FACE_NEGATIVE_Y, BORDER_TOP, false };
			mCubemapBoundaries[FACE_POSITIVE_Z][BORDER_LEFT] = 
				{ FACE_NEGATIVE_X, BORDER_RIGHT, false };
			mCubemapBoundaries[FACE_POSITIVE_Z][BORDER_RIGHT] = 
				{ FACE_POSITIVE_X, BORDER_LEFT, false };

			mCubemapBoundaries[FACE_NEGATIVE_Z][BORDER_TOP] = 
				{ FACE_POSITIVE_Y, BORDER_TOP, true };
			mCubemapBoundaries[FACE_NEGATIVE_Z][BORDER_BOTTOM] = 
				{ FACE_NEGATIVE_Y, BORDER_BOTTOM, true };
			mCubemapBoundaries[FACE_NEGATIVE_Z][BORDER_LEFT] = 
				{ FACE_POSITIVE_X, BORDER_RIGHT, false };
			mCubemapBoundaries[FACE_NEGATIVE_Z][BORDER_RIGHT] = 
				{ FACE_NEGATIVE_X, BORDER_LEFT, false };
		}

		inline void RemapClamp(int& x, const uint width) const  {
			x = std::max<int>(std::min<int>(x, width - 1), 0);
		}

		inline void RemapWrap(int& x, const uint width) const {
			if (x < 0)
				x += width;
			else if (x >= width) 
				x -= width;
		}

		inline void RemapSingle(int& x, const uint width, const SurfaceWrapping wrapType) const {
			if (wrapType == SurfaceWrapping::CLAMP) {
				RemapClamp(x, width);
			} else {
				RemapWrap(x, width);
			}
		}

		inline void Remap(int& x, const uint width) const {
			RemapSingle(x, width, mWrapTypeX);
		}

		inline void Remap(int& x, int& y, const uint width, const uint height) const {
			RemapSingle(x, width, mWrapTypeX);
			RemapSingle(y, height, mWrapTypeY);
		}

		inline void Remap(int& x, int& y, int& z, const uint width, const uint height, const uint depth) const {
			RemapSingle(x, width, mWrapTypeX);
			RemapSingle(y, height, mWrapTypeY);
			RemapSingle(z, depth, mWrapTypeZ);
		}

		void Get(int x, uint slice, uint mip, ReturnType out[]) const {
			if (mResourceDim == DG::RESOURCE_DIM_TEX_1D || mResourceDim == DG::RESOURCE_DIM_TEX_1D_ARRAY) {
				Remap(x, mMipWidths[mip]);
				size_t ptr = mSliceSize * slice + mMipOffsets[mip] + mChannelCount * x;
				for (int i = 0; i < mChannelCount; ++i) {
					out[i] = Convert(mBackend[ptr++]);
				}
			} else {
				throw std::runtime_error("1D Get is not supported for these template parameters!");
			}
		}

		inline void TransformAcrossBoundary(int& sampleX, int& sampleY, int& sampleFace, uint mip, uint face, int border) const {
			int border_index;
			switch (border) {
			case BORDER_TOP:
				border_index = sampleX;
				break;
			case BORDER_BOTTOM:
				border_index = sampleX;
				break;
			case BORDER_LEFT:
				border_index = sampleY;
				break;
			case BORDER_RIGHT:
				border_index = sampleY;
				break;
			}

			auto& boundaryInfo = mCubemapBoundaries[face][border];

			if (boundaryInfo.bFlip) {
				border_index = mMipWidths[mip] - 1 - border_index;
			}

			sampleFace = boundaryInfo.mFace;

			switch (boundaryInfo.mSide) {
			case BORDER_TOP:
				sampleX = border_index;
				sampleY = 0;
				break;
			case BORDER_BOTTOM:
				sampleX = border_index;
				sampleY = mMipHeights[mip] - 1;
				break;
			case BORDER_LEFT:
				sampleX = 0;
				sampleY = border_index;
				break;
			case BORDER_RIGHT:
				sampleX = mMipWidths[mip] - 1;
			}
		}

		void GetCube(int x, int y, uint slice, uint face, uint mip, ReturnType out[]) const {
			if (mResourceDim == DG::RESOURCE_DIM_TEX_CUBE || mResourceDim == DG::RESOURCE_DIM_TEX_CUBE_ARRAY) {
				int borderH = BORDER_NONE;
				int borderV = BORDER_NONE;

				int sampleXH = x;
				int sampleXV = x;
				int sampleYH = y;
				int sampleYV = y;
				int sampleFaceH = face;
				int sampleFaceV = face; 

				if (x < 0) {
					borderH = BORDER_LEFT;
					TransformAcrossBoundary(sampleXH, sampleYH, sampleFaceH, mip, face, borderH);
				} else if (x >= mMipWidths[mip]) {
					borderH = BORDER_RIGHT;
					TransformAcrossBoundary(sampleXH, sampleYH, sampleFaceH, mip, face, borderH);
				}

				if (y < 0) {
					borderV = BORDER_TOP;
					TransformAcrossBoundary(sampleXV, sampleYV, sampleFaceV, mip, face, borderV);
				} else if (y >= mMipHeights[mip]) {
					borderV = BORDER_BOTTOM;
					TransformAcrossBoundary(sampleXV, sampleYV, sampleFaceV, mip, face, borderV);
				}

				for (int i = 0; i < mChannelCount; ++i) {
					out[i] = 0.0;
				}

				if (borderH != BORDER_NONE) {
					size_t ptrH = mSliceSize * (slice * 6 + sampleFaceH) + mMipOffsets[mip] + 
						mChannelCount * mMipWidths[mip] * sampleYH + mChannelCount * sampleXH;
					for (int i = 0; i < mChannelCount; ++i) {
						out[i] += Convert(mBackend[ptrH++]);
					}
				}
				if (borderV != BORDER_NONE) {
					size_t ptrV = mSliceSize * (slice * 6 + sampleFaceV) + mMipOffsets[mip] + 
						mChannelCount * mMipWidths[mip] * sampleYV + mChannelCount * sampleXV;
					for (int i = 0; i < mChannelCount; ++i) {
						out[i] += Convert(mBackend[ptrV++]);
					}
				}

				// Average boundary contributions at corners
				if (borderV != BORDER_NONE && borderH != BORDER_NONE) {
					for (int i = 0; i < mChannelCount; ++i) {
						out[i] *= 0.5;
					}
				}

			} else {
				throw std::runtime_error("2D Get is not supported for these template parameters!");
			}
		}

		void Get(int x, int y, uint slice, uint mip, ReturnType out[]) const {
			if (mResourceDim == DG::RESOURCE_DIM_TEX_2D || mResourceDim == DG::RESOURCE_DIM_TEX_2D_ARRAY) {
				Remap(x, mMipWidths[mip]);
				Remap(y, mMipHeights[mip]);

				size_t ptr = mSliceSize * slice + mMipOffsets[mip] + mChannelCount * mMipWidths[mip] * y + mChannelCount * x;
				for (int i = 0; i < mChannelCount; ++i) {
					out[i] = Convert(mBackend[ptr++]);
				}

			} else {
				throw std::runtime_error("2D Get is not supported for these template parameters!");
			}
		}

		void Get(int x, int y, int z, uint slice, uint mip, ReturnType out[]) const {
			if (mResourceDim == DG::RESOURCE_DIM_TEX_3D) {
				size_t ptr = mSliceSize * slice + mMipOffsets[mip] 
					+ mChannelCount * mMipWidths[mip] * mMipHeights[mip] * z  
					+ mChannelCount * mMipWidths[mip] * y + mChannelCount * x;
				for (int i = 0; i < mChannelCount; ++i) {
					out[i] = Convert(mBackend[ptr++]);
				}
			} else {
				throw std::runtime_error("3D Get is not supported for these template parameters!");
			}
		}
	};

	template <typename T, uint ChannelCount>
	void Lerp(T x1[], T x2[], T t, T out[]) {
		for (uint i = 0; i < ChannelCount; ++i) {
			out[i] = (1.0 - t) * x1[i] + t * x2[i];
		}
	}

	template <typename ReturnType,
		typename IndexType, 
		typename SourceType,
		bool bSourceNormed>
	class ArraySurfaceAdaptor : 
		public ISurfaceAdaptor<ReturnType, IndexType> {
	private:
		static_assert(std::is_same_v<IndexType, double> || 
			std::is_same_v<IndexType, float>, "IndexType must be floating point!");

		typedef ArraySurfaceIndexer<
			ReturnType, IndexType, SourceType, 
			bSourceNormed> IndexerType;

		uint mChannelCount;
		IndexerType mIndexer;
		SurfaceWrapping mWrapX;
		SurfaceWrapping mWrapY;
		SurfaceWrapping mWrapZ;

		inline void InternalSampleLinear(IndexType x, uint mip, uint slice, ReturnType out[]) const {
			WrapFloat(x, mWrapX);

			x = x * mIndexer.GetMipWidth(mip) - 0.5;

			IndexType x_floor = std::floor(x);
			IndexType x_ceil = std::ceil(x);
			IndexType x_interp = x - x_floor;

			int x_0 = (int)x_floor;
			int x_1 = (int)x_ceil;

			ReturnType out0[4];
			ReturnType out1[4];

			mIndexer.Get(x_0, slice, mip, out0);
			mIndexer.Get(x_1, slice, mip, out1);

			Lerp<ReturnType, 4>(out0, out1, x_interp, out);
		}

		inline void InternalSampleLinear(IndexType x, IndexType y, uint mip, uint slice, ReturnType out[]) const {
			WrapFloat(x, mWrapX);
			WrapFloat(y, mWrapY);

			x = x * mIndexer.GetMipWidth(mip) - 0.5;
			y = y * mIndexer.GetMipHeight(mip) - 0.5;

			IndexType x_floor = std::floor(x);
			IndexType x_ceil = std::ceil(x);
			IndexType x_interp = x - x_floor;

			IndexType y_floor = std::floor(y);
			IndexType y_ceil = std::ceil(y);
			IndexType y_interp = y - y_floor;

			int x_0 = (int)x_floor;
			int x_1 = (int)x_ceil;

			int y_0 = (int)y_floor;
			int y_1 = (int)y_ceil;

			ReturnType out00[4];
			ReturnType out01[4];
			ReturnType out10[4];
			ReturnType out11[4];
			ReturnType out0[4];
			ReturnType out1[4];

			mIndexer.Get(x_0, y_0, slice, mip, out00);
			mIndexer.Get(x_0, y_1, slice, mip, out01);
			mIndexer.Get(x_1, y_0, slice, mip, out10);
			mIndexer.Get(x_1, y_1, slice, mip, out11);

			Lerp<ReturnType, 4>(out00, out01, y_interp, out0);
			Lerp<ReturnType, 4>(out10, out11, y_interp, out1);
			Lerp<ReturnType, 4>(out0, out1, x_interp, out);
		}

		inline void InternalSampleLinear(IndexType x, IndexType y, IndexType z, uint mip, ReturnType out[]) const {
			WrapFloat(x, mWrapX);
			WrapFloat(y, mWrapY);
			WrapFloat(z, mWrapZ);

			x = x * mIndexer.GetMipWidth(mip) - 0.5;
			y = y * mIndexer.GetMipHeight(mip) - 0.5;
			z = z * mIndexer.GetMipDepth(mip) - 0.5;

			IndexType x_floor = std::floor(x);
			IndexType x_ceil = std::ceil(x);
			IndexType x_interp = x - x_floor;

			IndexType y_floor = std::floor(y);
			IndexType y_ceil = std::ceil(y);
			IndexType y_interp = y - y_floor;

			IndexType z_floor = std::floor(z);
			IndexType z_ceil = std::ceil(z);
			IndexType z_interp = z - z_floor;

			int x_0 = (int)x_floor;
			int x_1 = (int)x_ceil;

			int y_0 = (int)y_floor;
			int y_1 = (int)y_ceil;

			int z_0 = (int)z_floor;
			int z_1 = (int)z_ceil;

			ReturnType out000[4];
			ReturnType out001[4];
			ReturnType out010[4];
			ReturnType out011[4];
			ReturnType out100[4];
			ReturnType out101[4];
			ReturnType out110[4];
			ReturnType out111[4];
			ReturnType out00[4];
			ReturnType out01[4];
			ReturnType out10[4];
			ReturnType out11[4];
			ReturnType out0[4];
			ReturnType out1[4];

			mIndexer.Get(x_0, y_0, z_0, mip, out000);
			mIndexer.Get(x_0, y_0, z_1, mip, out001);
			mIndexer.Get(x_0, y_1, z_0, mip, out010);
			mIndexer.Get(x_0, y_1, z_1, mip, out011);
			mIndexer.Get(x_1, y_0, z_0, mip, out100);
			mIndexer.Get(x_1, y_0, z_1, mip, out101);
			mIndexer.Get(x_1, y_1, z_0, mip, out110);
			mIndexer.Get(x_1, y_1, z_1, mip, out111);

			Lerp<ReturnType, 4>(out000, out001, z_interp, out00);
			Lerp<ReturnType, 4>(out010, out011, z_interp, out01);
			Lerp<ReturnType, 4>(out100, out101, z_interp, out10);
			Lerp<ReturnType, 4>(out110, out111, z_interp, out11);
			Lerp<ReturnType, 4>(out00, out01, y_interp, out0);
			Lerp<ReturnType, 4>(out10, out11, y_interp, out1);
			Lerp<ReturnType, 4>(out0, out1, x_interp, out);
		}

		inline void InternalSampleLinearCube(IndexType u, IndexType v, uint face, uint mip, uint slice, ReturnType out[]) const {
			u = u * mIndexer.GetMipWidth(mip) - 0.5;
			v = v * mIndexer.GetMipHeight(mip) - 0.5;

			IndexType u_floor = std::floor(u);
			IndexType u_ceil = std::ceil(u);
			IndexType u_interp = u - u_floor;

			IndexType v_floor = std::floor(v);
			IndexType v_ceil = std::ceil(v);
			IndexType v_interp = v - v_floor;

			int u_0 = (int)u_floor;
			int u_1 = (int)u_ceil;

			int v_0 = (int)v_floor;
			int v_1 = (int)v_ceil;

			ReturnType out00[4];
			ReturnType out01[4];
			ReturnType out10[4];
			ReturnType out11[4];
			ReturnType out0[4];
			ReturnType out1[4];

			mIndexer.GetCube(u_0, v_0, slice, face, mip, out00);
			mIndexer.GetCube(u_0, v_1, slice, face, mip, out01);
			mIndexer.GetCube(u_1, v_0, slice, face, mip, out10);
			mIndexer.GetCube(u_1, v_1, slice, face, mip, out11);

			Lerp<ReturnType, 4>(out00, out01, v_interp, out0);
			Lerp<ReturnType, 4>(out10, out11, v_interp, out1);
			Lerp<ReturnType, 4>(out0, out1, u_interp, out);
		}

	public:
		ArraySurfaceAdaptor(const SourceType* backend, 
			uint width, 
			uint height, 
			uint depth,
			uint slices,
			uint mips,
			uint channelCount,
			SurfaceWrapping wrapX,
			SurfaceWrapping wrapY,
			SurfaceWrapping wrapZ,
			DG::RESOURCE_DIMENSION resourceDim) :
			mIndexer(backend, width, height, depth, slices, mips, 
				channelCount, wrapX, wrapY, wrapZ, resourceDim),
			mChannelCount(channelCount),
			mWrapX(wrapX),
			mWrapY(wrapY),
			mWrapZ(wrapZ) {
		}

		void SampleLinearMip(IndexType x, IndexType dA, uint slice, ReturnType out[]) const override {
			IndexType fmip = std::log2(dA / mIndexer.GetMipTexelArea(0));
			fmip = std::min<IndexType>(std::max<IndexType>(fmip, 0.0), (IndexType)(mIndexer.GetMipCount() - 1));
			IndexType fmip_floor = std::floor(fmip);

			uint mip_up = (uint)std::ceil(fmip);
			uint mip_down = (uint)fmip_floor;

			IndexType interp = fmip - fmip_floor;

			ReturnType out_up[4];
			ReturnType out_down[4];

			InternalSampleLinear(x, mip_up, slice, out_up);
			InternalSampleLinear(x, mip_down, slice, out_down);
			Lerp<ReturnType, 4>(out_down, out_up, interp, out);
		}

		void SampleLinearMip(IndexType x, IndexType y, IndexType dA, uint slice, ReturnType out[]) const override {
			IndexType fmip = std::log2(dA / mIndexer.GetMipTexelArea(0)) / 2.0;
			fmip = std::min<IndexType>(std::max<IndexType>(fmip, 0.0), (IndexType)(mIndexer.GetMipCount() - 1));
			IndexType fmip_floor = std::floor(fmip);

			uint mip_up = (uint)std::ceil(fmip);
			uint mip_down = (uint)fmip_floor;

			IndexType interp = fmip - fmip_floor;

			ReturnType out_up[4];
			ReturnType out_down[4];
			InternalSampleLinear(x, y, mip_up, slice, out_up);
			InternalSampleLinear(x, y, mip_down, slice, out_down);
			Lerp<ReturnType, 4>(out_down, out_up, interp, out);
		}

		void SampleLinearMip(IndexType x, IndexType y, IndexType z, IndexType dA, ReturnType out[]) const override {
			IndexType fmip = std::log2(dA / mIndexer.GetMipTexelArea(0)) / 3.0;
			fmip = std::min<IndexType>(std::max<IndexType>(fmip, 0.0), (IndexType)(mIndexer.GetMipCount() - 1));
			IndexType fmip_floor = std::floor(fmip);

			uint mip_up = (uint)std::ceil(fmip);
			uint mip_down = (uint)fmip_floor;

			IndexType interp = fmip - fmip_floor;

			ReturnType out_up[4];
			ReturnType out_down[4];
			InternalSampleLinear(x, y, z, mip_up, out_up);
			InternalSampleLinear(x, y, z, mip_down, out_down);
			Lerp<ReturnType, 4>(out_down, out_up, interp, out);
		}

		void SampleCubeLinearMip(IndexType x, IndexType y, IndexType z, IndexType dA, uint slice, ReturnType out[]) const override {
			IndexType u;
			IndexType v;
			uint face;

			CubemapHelper<IndexType>::ToUV(x, y, z, &u, &v, &face);

			// GetMipTexelArea returns the area of a pixel on [0, 1]^2, but the Jacobian is to [-1, 1]^2
			// Divide by 4 to compensate for this area discrepancy.
			auto jacobFactor = CubemapHelper<IndexType>::Jacobian(u, v) * 4.0;

			IndexType fmip = std::log2(dA / (mIndexer.GetMipTexelArea(0) * jacobFactor)) / 2.0;
			fmip = std::min<IndexType>(std::max<IndexType>(fmip, 0.0), (IndexType)(mIndexer.GetMipCount() - 1));
			IndexType fmip_floor = std::floor(fmip);

			uint mip_up = (uint)std::ceil(fmip);
			uint mip_down = (uint)fmip_floor;

			IndexType interp = fmip - fmip_floor;

			ReturnType out_up[4];
			ReturnType out_down[4];
			InternalSampleLinearCube(u, v, face, mip_up, slice, out_up);
			InternalSampleLinearCube(u, v, face, mip_down, slice, out_down);
			Lerp<ReturnType, 4>(out_down, out_up, interp, out);
		}

		void SampleLinear(IndexType x, uint mip, uint slice, ReturnType out[]) const override {
			InternalSampleLinear(x, mip, slice, out);
		}

		void SampleLinear(IndexType x, IndexType y, uint mip, uint slice, ReturnType out[]) const override {
			InternalSampleLinear(x, y, mip, slice, out);
		}

		void SampleLinear(IndexType x, IndexType y, IndexType z, uint mip, ReturnType out[]) const override {
			InternalSampleLinear(x, y, z, mip, out);
		}

		void SampleCubeLinear(IndexType x, IndexType y, IndexType z, uint mip, uint slice, ReturnType out[]) const override {
			InternalSampleLinearCube(x, y, z, mip, slice, out);
		}

		void SampleCubeNearest(IndexType x, IndexType y, IndexType z, uint mip, uint slice, ReturnType out[]) const override {
			IndexType u;
			IndexType v;
			uint face;

			CubemapHelper<IndexType>::ToUV(x, y, z, &u, &v, &face);

			mip = std::min(mip, mIndexer.GetMipCount() - 1);

			int u_nearest = (int)std::floor(u * mIndexer.GetMipWidth(mip));
			int v_nearest = (int)std::floor(v * mIndexer.GetMipHeight(mip));

			mIndexer.GetCube(u_nearest, v_nearest, slice, face, mip, out);
		}

		void SampleNearest(IndexType x, uint mip, uint slice, ReturnType out[]) const override {
			mip = std::min(mip, mIndexer.GetMipCount() - 1);

			WrapFloat(x, mWrapX);

			int x_nearest = (int)(std::floor(x * mIndexer.GetMipWidth(mip)));
			mIndexer.Get(x_nearest, slice, mip, out);
		}

		void SampleNearest(IndexType x, IndexType y, uint mip, uint slice, ReturnType out[]) const override {
			mip = std::min(mip, mIndexer.GetMipCount() - 1);

			WrapFloat(x, mWrapX);
			WrapFloat(y, mWrapY);

			int x_nearest = (int)(std::floor(x * mIndexer.GetMipWidth(mip)));
			int y_nearest = (int)(std::floor(y * mIndexer.GetMipHeight(mip)));
			mIndexer.Get(x_nearest, y_nearest, slice, mip, out);
		}

		void SampleNearest(IndexType x, IndexType y, IndexType z, uint mip, ReturnType out[]) const override {
			mip = std::min(mip, mIndexer.GetMipCount() - 1);

			WrapFloat(x, mWrapX);
			WrapFloat(y, mWrapY);
			WrapFloat(z, mWrapZ);

			int x_nearest = (int)(std::floor(x * mIndexer.GetMipWidth(mip)));
			int y_nearest = (int)(std::floor(y * mIndexer.GetMipHeight(mip)));
			int z_nearest = (int)(std::floor(z * mIndexer.GetMipDepth(mip)));
			mIndexer.Get(x_nearest, y_nearest, z_nearest, mip, out);
		}
	};

	template <typename ReturnType, typename IndexType>
	ISurfaceAdaptor<ReturnType, IndexType>* SpawnAdaptor(
		DG::TEXTURE_FORMAT sourceFormat,
		void* backend, 
		uint width, 
		uint height, 
		uint depth,
		uint slices,
		uint mips,
		SurfaceWrapping wrapX,
		SurfaceWrapping wrapY,
		SurfaceWrapping wrapZ,
		DG::RESOURCE_DIMENSION resourceDim) {

		int channels = GetComponentCount(sourceFormat);
		auto type = GetComponentType(sourceFormat);
		bool bNormed = GetIsNormalized(sourceFormat);

		if (channels < 0 || type < 0) {
			throw std::runtime_error("Texture format is not allowed for RawTextureSampler");
		}

		switch (type) {
			case DG::VT_FLOAT16:
				throw std::runtime_error("Float16 not supported by RawTxtureSampler!");
				break;
			case DG::VT_FLOAT32:
				return new ArraySurfaceAdaptor<ReturnType, IndexType, DG::Float32, false>(
					(DG::Float32*)backend, width, height, depth, slices, mips, 
					channels, wrapX, wrapY, wrapZ, resourceDim);
			case DG::VT_UINT8:
				if (bNormed)
					return new ArraySurfaceAdaptor<ReturnType, IndexType, DG::Uint8, true>(
						(DG::Uint8*)backend, width, height, depth, slices, 
						mips, channels, wrapX, wrapY, wrapZ, resourceDim);
				else 
					return new ArraySurfaceAdaptor<ReturnType, IndexType, DG::Uint8, false>(
						(DG::Uint8*)backend, width, height, depth, slices, 
						mips, channels, wrapX, wrapY, wrapZ, resourceDim);
			case DG::VT_UINT16:
				if (bNormed)
					return new ArraySurfaceAdaptor<ReturnType, IndexType, DG::Uint16, true>(
						(DG::Uint16*)backend, width, height, depth, slices, 
						mips, channels, wrapX, wrapY, wrapZ, resourceDim);
				else 
					return new ArraySurfaceAdaptor<ReturnType, IndexType, DG::Uint16, false>(
						(DG::Uint16*)backend, width, height, depth, slices, 
						mips, channels, wrapX, wrapY, wrapZ, resourceDim);
			case DG::VT_UINT32:
				return new ArraySurfaceAdaptor<ReturnType, IndexType, DG::Uint32, false>(
					(DG::Uint32*)backend, width, height, depth, slices, 
					mips, channels, wrapX, wrapY, wrapZ, resourceDim);
			case DG::VT_INT8:
				if (bNormed)
					return new ArraySurfaceAdaptor<ReturnType, IndexType, DG::Int8, true>(
						(DG::Int8*)backend, width, height, depth, slices, 
						mips, channels, wrapX, wrapY, wrapZ, resourceDim);
				else 
					return new ArraySurfaceAdaptor<ReturnType, IndexType, DG::Int8, false>(
						(DG::Int8*)backend, width, height, depth, slices, 
						mips, channels, wrapX, wrapY, wrapZ, resourceDim);
			case DG::VT_INT16:
				if (bNormed)
					return new ArraySurfaceAdaptor<ReturnType, IndexType, DG::Int16, true>(
						(DG::Int16*)backend, width, height, depth, slices, 
						mips, channels, wrapX, wrapY, wrapZ, resourceDim);
				else 
					return new ArraySurfaceAdaptor<ReturnType, IndexType, DG::Int16, false>(
						(DG::Int16*)backend, width, height, depth, slices, 
						mips, channels, wrapX, wrapY, wrapZ, resourceDim);
			case DG::VT_INT32:
				return new ArraySurfaceAdaptor<ReturnType, IndexType, DG::Uint32, false>(
					(DG::Uint32*)backend, width, height, depth, slices, 
					mips, channels, wrapX, wrapY, wrapZ, resourceDim);
			default:
				throw std::runtime_error("Unrecognized value type!");
		}
	}

	RawSampler::RawSampler(RawTexture* texture, 
		const WrapParameters& wrapping) :
		mAdapterF(SpawnAdaptor<float, float>(texture->mDesc.Format,
			&texture->mData[0],
			texture->mDesc.Width,
			texture->mDesc.Height,
			texture->mDesc.Depth,
			texture->mDesc.ArraySize,
			texture->mDesc.MipLevels,
			wrapping.mWrapTypeX,
			wrapping.mWrapTypeY,
			wrapping.mWrapTypeZ,
			texture->mDesc.Type)),
		mAdapterD(SpawnAdaptor<double, double>(texture->mDesc.Format,
			&texture->mData[0],
			texture->mDesc.Width,
			texture->mDesc.Height,
			texture->mDesc.Depth,
			texture->mDesc.ArraySize,
			texture->mDesc.MipLevels,
			wrapping.mWrapTypeX,
			wrapping.mWrapTypeY,
			wrapping.mWrapTypeZ,
			texture->mDesc.Type)) {
	}
}