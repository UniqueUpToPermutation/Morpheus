#pragma once

#include <vector>

#include <Engine/Resources/Resource.hpp>
#include <Engine/Geometry.hpp>

namespace Morpheus {
	struct Tileset {
		TextureResource* mTexture;
		std::vector<SpriteRect> mSubTiles;
	};

	struct TilemapData {
		std::vector<int> mTileIds;
		std::vector<uint8_t> mTileTilesets;
		uint mLayerWidth;
		uint mLayerHeight;
		uint mLayers;
	};

	struct TilemapLayer {
		int mRenderLayerId;
		DG::float2 mTileRenderSize;
		DG::float2 mTileAxisX;
		DG::float2 mTileAxisY;
		DG::float2 mTileOrigin;
	};

	struct TilemapComponent {
		TilemapData mData;
		std::vector<TilemapLayer> mLayers;
		std::vector<Tileset> mTilesets;
	};
}