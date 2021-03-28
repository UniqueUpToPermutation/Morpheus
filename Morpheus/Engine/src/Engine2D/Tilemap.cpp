#include <Engine/Engine2D/Tilemap.hpp>
#include <Engine/Resources/TextureResource.hpp>

namespace Morpheus {
	void TilemapView::Fill(int tileId, int tilesetId, float zOffset) {
		std::fill(mTilemap->mData.mTileIds.begin(), mTilemap->mData.mTileIds.end(), tileId);
		std::fill(mTilemap->mData.mTileTilesets.begin(), mTilemap->mData.mTileTilesets.end(), tilesetId);
		std::fill(mTilemap->mData.mTileZOffsets.begin(), mTilemap->mData.mTileZOffsets.end(), zOffset);
	}

	void TilemapLayerView::Fill(int tileId, int tilesetId, float zOffset) {
		TilemapView view(mTilemap);
		size_t layerBegin = view.GetWidth() * view.GetHeight() * mLayer;
		size_t layerEnd = view.GetWidth() * view.GetHeight() * (mLayer + 1);

		std::fill(&mTilemap->mData.mTileIds[layerBegin], &mTilemap->mData.mTileIds[layerEnd], tileId);
		if (view.IsMultipleTilesetsEnabled())
			std::fill(&mTilemap->mData.mTileTilesets[layerBegin], &mTilemap->mData.mTileTilesets[layerEnd], tilesetId);
		if (view.IsZOffsetsEnabled())
			std::fill(&mTilemap->mData.mTileZOffsets[layerBegin], &mTilemap->mData.mTileZOffsets[layerEnd], zOffset);
	}

	void TilemapView::SetDimensions(uint width, uint height, bool bPreserveData) {
		std::vector<int> newTileIds;
		std::vector<uint8_t> newTileTilesets;
		std::vector<float> newTileZOffsets;

		uint layerCount = mTilemap->mLayers.size();

		newTileIds.resize(width * height * layerCount);

		if (IsMultipleTilesetsEnabled()) {
			newTileTilesets.resize(width * height * layerCount);
		}

		if (IsZOffsetsEnabled()) {
			newTileZOffsets.resize(width * height * layerCount);
		}

		std::fill(newTileIds.begin(), newTileIds.end(), TILE_NONE);
		std::fill(newTileTilesets.begin(), newTileTilesets.end(), 0);
		std::fill(newTileZOffsets.begin(), newTileZOffsets.end(), 0.0f);

		if (bPreserveData) {
			uint prevWidth = mTilemap->mData.mLayerWidth;
			uint prevHeight = mTilemap->mData.mLayerHeight;
			uint prevLayers = mTilemap->mLayers.size();

			uint minWidth = std::min(width, prevWidth);
			uint minHeight = std::min(height, prevHeight);
			uint minLayers = std::min(layerCount, prevLayers);

			for (uint z = 0; z < minLayers; ++z) {
				for (uint y = 0; y < minHeight; ++y) {
					for (uint x = 0; x < minWidth; ++x) {
						newTileIds[x + y * width + z * width * height] = mTilemap->mData.mTileIds[x + y * prevWidth + z * prevWidth * prevHeight];
					}
				}
			}

			if (IsMultipleTilesetsEnabled()) {
				for (uint z = 0; z < minLayers; ++z) {
					for (uint y = 0; y < minHeight; ++y) {
						for (uint x = 0; x < minWidth; ++x) {
							newTileTilesets[x + y * width + z * width * height] = mTilemap->mData.mTileTilesets[x + y * prevWidth + z * prevWidth * prevHeight];
						}
					}
				}
			}

			if (IsZOffsetsEnabled()) {
				for (uint z = 0; z < minLayers; ++z) {
					for (uint y = 0; y < minHeight; ++y) {
						for (uint x = 0; x < minWidth; ++x) {
							newTileZOffsets[x + y * width + z * width * height] = mTilemap->mData.mTileZOffsets[x + y * prevWidth + z * prevWidth * prevHeight];
						}
					}
				}
			}
		}

		mTilemap->mData.mLayerWidth = width;
		mTilemap->mData.mLayerHeight = height;
		
		std::swap(mTilemap->mData.mTileIds, newTileIds);
		std::swap(mTilemap->mData.mTileTilesets, newTileTilesets);
		std::swap(mTilemap->mData.mTileZOffsets, newTileZOffsets);
	}

	void TilemapView::SetMultipleTilesetsEnabled(bool value) {
		if (value) {
			mTilemap->mData.mTileTilesets.resize(mTilemap->mData.mTileIds.size());
			std::fill(mTilemap->mData.mTileTilesets.begin(), mTilemap->mData.mTileTilesets.end(), 0);
		} else {
			mTilemap->mData.mTileTilesets.resize(0);
		}

		mTilemap->mData.bHasMultipleTilesets = value;
	}

	void TilemapView::SetZOffsetsEnabled(bool value) {
		if (value) {
			mTilemap->mData.mTileZOffsets.resize(mTilemap->mData.mTileIds.size());
			std::fill(mTilemap->mData.mTileZOffsets.begin(), mTilemap->mData.mTileZOffsets.end(), 0.f);
		} else {
			mTilemap->mData.mTileZOffsets.resize(0);
		}

		mTilemap->mData.bHasZOffsets = value;
	}

	void TilemapView::CreateNewTileset(TextureResource* texture, 
		const DG::float2& tileSize,
		const DG::float2& tileOrigin,
		const DG::float2& padding) {

		Tileset tileset;
		tileset.mTexture = texture;
		
		uint tileCountX = (uint)((texture->GetWidth() + padding.x) / (tileSize.x + padding.x));
		uint tileCountY = (uint)((texture->GetHeight() + padding.y) / (tileSize.y + padding.y));

		tileset.mSubTiles.resize(tileCountX * tileCountY);
		tileset.mTileOrigin = tileOrigin;

		for (uint y = 0; y < tileCountY; ++y) {
			for (uint x = 0; x < tileCountX; ++x) {
				auto& tile = tileset.mSubTiles[x + y * tileCountX];
				tile.mPosition.x = x * (tileSize.x + padding.x);
				tile.mPosition.y = y * (tileSize.y + padding.y);
				tile.mSize = tileSize;
			}
		}

		mTilemap->mTilesets.emplace_back(std::move(tileset));
	}

	TilemapLayerView TilemapView::CreateNewLayer(
		const DG::float2& tileDisplaySize, 
		const DG::float2& spacing,
		const RenderLayerId renderLayerId,
		const DG::float3& layerOffset) {

		TilemapLayer layer;
		layer.mLayerOffset = layerOffset;
		layer.mRenderLayerId = renderLayerId;

		switch (mTilemap->mType) {
		case TilemapType::ORTHOGRAPHIC:
			layer.mTileAxisX = DG::float2(spacing.x, 0.0f);
			layer.mTileAxisY = DG::float2(0.0f, spacing.y);
			break;
		case TilemapType::ISOMETRIC:
			layer.mTileAxisX = DG::float2(spacing.x, -spacing.y);
			layer.mTileAxisY = DG::float2(spacing.x, spacing.y);
			break;
		}

		layer.mTileRenderSize = tileDisplaySize;

		mTilemap->mLayers.emplace_back(layer);

		auto width = mTilemap->mData.mLayerWidth;
		auto height = mTilemap->mData.mLayerHeight;
		auto layers = mTilemap->mLayers.size();

		mTilemap->mData.mTileIds.resize(width * height * layers);

		if (IsMultipleTilesetsEnabled()) {
			mTilemap->mData.mTileTilesets.resize(width * height * layers);
		}

		if (IsZOffsetsEnabled()) {
			mTilemap->mData.mTileZOffsets.resize(width * height * layers);
		}

		return TilemapLayerView(mTilemap, mTilemap->mLayers.size() - 1);
	}

	bool TilemapView::IsMultipleTilesetsEnabled() const {
		return mTilemap->mData.bHasMultipleTilesets;
	}

	bool TilemapView::IsZOffsetsEnabled() const {
		return mTilemap->mData.bHasZOffsets;
	}

	void TilemapView::SwapLayers(int layer1, int layer2) {
			std::swap(mTilemap->mLayers[layer1], mTilemap->mLayers[layer2]);

		size_t width = GetWidth();
		size_t height = GetHeight();

		size_t layer1Start = width * height * layer1;
		size_t layer1End = width * height * (layer1 + 1);
		size_t layer2Start = width * height * layer2;
		size_t layer2End = width * height * (layer2 + 1);

		for (size_t pos1 = layer1Start, pos2 = layer2Start; pos1 < layer1End; ++pos1, ++pos2) {
			std::swap(mTilemap->mData.mTileIds[pos1], mTilemap->mData.mTileIds[pos2]);
		}

		if (IsMultipleTilesetsEnabled()) {
			for (size_t pos1 = layer1Start, pos2 = layer2Start; pos1 < layer1End; ++pos1, ++pos2) {
				std::swap(mTilemap->mData.mTileTilesets[pos1], mTilemap->mData.mTileTilesets[pos2]);
			}
		}

		if (IsZOffsetsEnabled()) {
			for (size_t pos1 = layer1Start, pos2 = layer2Start; pos1 < layer1End; ++pos1, ++pos2) {
				std::swap(mTilemap->mData.mTileZOffsets[pos1], mTilemap->mData.mTileZOffsets[pos2]);
			}
		}
	}

	void TilemapView::SwapTilesets(int tileset1, int tileset2) {
		std::swap(mTilemap->mTilesets[tileset1], mTilemap->mTilesets[tileset2]);

		uint8_t tileset1u = (uint8_t)tileset1;
		uint8_t tileset2u = (uint8_t)tileset2;

		for (auto& dat : mTilemap->mData.mTileTilesets) {
			if (dat == tileset1u) 
				dat = tileset2u;
			else if (dat == tileset2u)
				dat = tileset1u;
		}
	}

	void TilemapView::RemoveLayer(int layer, bool bPreserveLayerOrder) {
		if (bPreserveLayerOrder) {
			// Move layer to the end
			for (int currentIdx = layer + 1; currentIdx < GetLayerCount(); ++currentIdx) {
				SwapLayers(currentIdx - 1, currentIdx);
			}
		} else {
			if (layer != GetLayerCount() - 1) {
				SwapLayers(layer, GetLayerCount() - 1);
			}
		}

		mTilemap->mLayers.pop_back();
		mTilemap->mData.mTileIds.resize(GetWidth() * GetHeight() * GetLayerCount());

		if (IsMultipleTilesetsEnabled()) {
			mTilemap->mData.mTileTilesets.resize(GetWidth() * GetHeight() * GetLayerCount());
		}

		if (IsZOffsetsEnabled()) {
			mTilemap->mData.mTileZOffsets.resize(GetWidth() * GetHeight() * GetLayerCount());
		}
	}

	void TilemapView::RemoveTileset(int tileset, bool bPreserveTilesetOrder) {
		if (bPreserveTilesetOrder) {
			// Move tileset to the end
			for (int currentIdx = tileset + 1; currentIdx < GetTilesetCount(); ++currentIdx) {
				SwapTilesets(currentIdx - 1, currentIdx);
			}
		} else {
			if (tileset != GetTilesetCount() - 1) {
				SwapTilesets(tileset, GetTilesetCount() - 1);
			}
		}

		mTilemap->mTilesets.pop_back();
	}
}