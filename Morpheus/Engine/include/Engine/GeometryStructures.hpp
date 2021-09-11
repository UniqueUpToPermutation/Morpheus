#pragma once

#include "BasicMath.hpp"
#include "InputLayout.h"

namespace DG = Diligent;

namespace Diligent {
	template <class Archive> 
	void serialize(Archive& archive, DG::LayoutElement& element) {
		archive(element.BufferSlot);
		archive(element.Frequency);
		archive(element.InputIndex);
		archive(element.InstanceDataStepRate);
		archive(element.IsNormalized);
		archive(element.NumComponents);
		archive(element.RelativeOffset);
		archive(element.Stride);
		archive(element.ValueType);
	}
}

namespace Morpheus {
	struct BoundingBox {
		DG::float3 mLower;
		DG::float3 mUpper;
	};

	struct BoundingBox2D {
		DG::float2 mLower;
		DG::float2 mUpper;
	};

	struct SpriteRect {
		DG::float2 mPosition;
		DG::float2 mSize;

		inline SpriteRect() {
		}

		inline SpriteRect(const DG::float2& position, const DG::float2& size) :
			mPosition(position),
			mSize(size) {
		}

		inline SpriteRect(float upperX, float upperY, float sizeX, float sizeY) :
			mPosition(upperX, upperY),
			mSize(sizeX, sizeY) {
		}
	};

	struct Ray {
		DG::float3 mStart;
		DG::float3 mDirection;
	};

	struct VertexLayout {
	public:
		std::vector<DG::LayoutElement> mElements;

		int mPosition	= -1;
		int mNormal 	= -1;
		int mTangent 	= -1;
		int mBitangent	= -1;

		std::vector<int> mUVs;
		std::vector<int> mColors;
		
		static VertexLayout PositionUVNormalTangent();
		static VertexLayout PositionUVNormal();
		static VertexLayout PositionUVNormalTangentBitangent();

		template <class Archive>
		void serialize(Archive& archive) {
			archive(mElements);

			archive(mPosition);
			archive(mNormal);
			archive(mTangent);
			archive(mBitangent);

			archive(mUVs);
			archive(mColors);
		}
	};

	enum class GeometryType {
		STATIC_MESH,
		UNSPECIFIED
	};

	class IVertexFormatProvider {
	public:
		virtual const VertexLayout& GetStaticMeshLayout() const = 0;

		inline const VertexLayout& GetLayout(GeometryType type) const {
			switch (type) {
				case GeometryType::STATIC_MESH:
					return GetStaticMeshLayout();
					break;
				default:
					return GetStaticMeshLayout();
					break;
			}
		}
	};
}