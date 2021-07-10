#pragma once

#include "BasicMath.hpp"
#include "InputLayout.h"

namespace DG = Diligent;

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
		int mUV			= -1;
		int mNormal 	= -1;
		int mTangent 	= -1;
		int mBitangent	= -1;
		
		static VertexLayout PositionUVNormalTangent();
		static VertexLayout PositionUVNormal();
		static VertexLayout PositionUVNormalTangentBitangent();
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