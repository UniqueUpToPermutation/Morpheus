#pragma once

#include <vector>

#include <Engine/Resources/Resource.hpp>
#include <Engine/Geometry.hpp>
#include <Engine/Engine2D/Base2D.hpp>

#define TILE_NONE -1

namespace Morpheus {
	struct Tileset {
		RefHandle<TextureResource> mTexture;
		std::vector<SpriteRect> mSubTiles;
		DG::float2 mTileOrigin;
	};

	struct TilemapData {
		std::vector<int> mTileIds;
		std::vector<uint8_t> mTileTilesets;
		std::vector<float> mTileZOffsets;
		uint mLayerWidth = 0;
		uint mLayerHeight = 0;
		bool bHasMultipleTilesets = false;
		bool bHasZOffsets = false;
	};

	struct TilemapLayer {
		RenderLayerId mRenderLayerId = RENDER_LAYER_NONE;
		DG::float2 mTileRenderSize;
		DG::float2 mTileAxisX;
		DG::float2 mTileAxisY;
		DG::float3 mLayerOffset;
	};

	enum class TilemapType {
		ORTHOGRAPHIC,
		ISOMETRIC
	};

	struct TilemapComponent {
		TilemapData mData;
		std::vector<TilemapLayer> mLayers;
		std::vector<Tileset> mTilesets;
		TilemapType mType = TilemapType::ORTHOGRAPHIC;
	};

	class TileView {
	private:
		TilemapComponent* mTilemap;
		int mX;
		int mY;
		int mLayer;

		inline size_t Idx() const {
			return mX + mY * mTilemap->mData.mLayerWidth + 
				mLayer * mTilemap->mData.mLayerWidth * mTilemap->mData.mLayerHeight;
		}

	public:
		inline TileView(TilemapComponent* tilemap, int x, int y, int layer) :
			mTilemap(tilemap),
			mX(x),
			mY(y),
			mLayer(layer) {
		}

		inline DG::float3 GetPosition() const {
			auto& layer = mTilemap->mLayers[mLayer];
			DG::float3 axisX(layer.mTileAxisX.x, layer.mTileAxisX.y, 0.0f);
			DG::float3 axisY(layer.mTileAxisY.x, layer.mTileAxisY.y, 0.0f);
			return axisX * mX + axisY * mY + layer.mLayerOffset;
		}

		inline DG::float2 GetSize() const {
			return mTilemap->mLayers[mLayer].mTileRenderSize;
		}

		inline int GetTilesetId() const {
			return mTilemap->mData.mTileTilesets.size() > 0 ? 0 : 
				mTilemap->mData.mTileTilesets[Idx()];
		}

		inline const Tileset& GetTileset() const {
			return mTilemap->mTilesets[GetTilesetId()];
		}

		inline Tileset& GetTileset() {
			return mTilemap->mTilesets[GetTilesetId()];
		}

		inline int GetTileId() const {
			return mTilemap->mData.mTileIds[Idx()];
		}

		inline float GetZOffset() const {
			return mTilemap->mData.mTileZOffsets.size() > 0 ? 0.0f :
				mTilemap->mData.mTileZOffsets[Idx()];
		}

		inline SpriteRect GetRect() {
			return GetTileset().mSubTiles[GetTileId()];
		}

		inline TextureResource* GetTexture() {
			return GetTileset().mTexture.RawPtr();
		}

		inline void SetTilesetId(int tilesetId) {
			if (mTilemap->mData.mTileTilesets.size() > 0)
				mTilemap->mData.mTileTilesets[Idx()] = tilesetId;
		}
		inline void SetTileId(int tileId) {
			mTilemap->mData.mTileIds[Idx()] = tileId;
		}
		inline void SetZOffset(float offsetZ) {
			mTilemap->mData.mTileZOffsets[Idx()] = offsetZ;
		}

		inline int GetIndexX() const {
			return mX;
		}
		inline int GetIndexY() const {
			return mY;
		}
		inline int GetLayerIndex() const {
			return mLayer;
		}
	};

	class TilemapLayerView {
	private:
		TilemapComponent* mTilemap;
		int mLayer;

	public:
		inline TilemapLayerView(TilemapComponent* tilemap, int layer) :
			mTilemap(tilemap), mLayer(layer) {	
		}

		void Fill(int tileId = TILE_NONE, int tilesetId = 0, float zOffset = 0.0f);

		inline TileView At(uint x, uint y) {
			return TileView(mTilemap, x, y, mLayer);
		}

		inline TileView operator()(uint x, uint y) {
			return At(x, y);
		}

		RenderLayerId GetRenderLayer() {
			return mTilemap->mLayers[mLayer].mRenderLayerId;
		}

		void SetRenderLayer(RenderLayerId layer) {
			mTilemap->mLayers[mLayer].mRenderLayerId = layer;
		}

		DG::float2 GetTileRenderSize() const {
			return mTilemap->mLayers[mLayer].mTileRenderSize;
		}

		void SetTileRenderSize(const DG::float2 size) {
			mTilemap->mLayers[mLayer].mTileRenderSize = size;
		}

		DG::float3 GetOffset() const {
			return mTilemap->mLayers[mLayer].mLayerOffset;
		}

		void SetOffset(const DG::float3& offset) {
			mTilemap->mLayers[mLayer].mLayerOffset = offset;
		}
	};

	class TilemapView {
	private:
		TilemapComponent* mTilemap;

	public:
		inline TilemapView(TilemapComponent* tilemap) : mTilemap(tilemap) {
		}

		inline TilemapView(TilemapComponent& tilemap) : mTilemap(&tilemap) {
		}

		inline TilemapLayerView operator[](const uint layer) {
			return TilemapLayerView(mTilemap, layer);
		}

		inline TileView At(uint layer, uint x, uint y) {
			return TileView(mTilemap, x, y, layer);
		}

		inline TileView operator()( uint layer, uint x, uint y) {
			return At(layer, x, y);
		}

		inline uint GetWidth() const {
			return mTilemap->mData.mLayerWidth;
		}

		inline uint GetHeight() const {
			return mTilemap->mData.mLayerHeight;
		}

		inline uint GetLayerCount() const {
			return mTilemap->mLayers.size();
		}

		inline uint GetTilesetCount() const {
			return mTilemap->mTilesets.size();
		}

		inline TilemapType GetType() const {
			return mTilemap->mType;
		}

		inline void SetType(TilemapType type) const {
			mTilemap->mType = type;
		}

		void Fill(int tileId = TILE_NONE, int tilesetId = 0, float zOffset = 0.0f);
		void SetDimensions(uint width, uint height, bool bPreserveData = true);
		void SetMultipleTilesetsEnabled(bool value);
		void SetZOffsetsEnabled(bool value);
		bool IsMultipleTilesetsEnabled() const;
		bool IsZOffsetsEnabled() const;

		void CreateNewTileset(TextureResource* texture, 
			const DG::float2& tileSize,
			const DG::float2& tileOrigin,
			const DG::float2& padding = DG::float2(0.0f, 0.0f));
		TilemapLayerView CreateNewLayer(const DG::float2& tileDisplaySize, 
			const DG::float2& spacing,
			const RenderLayerId renderLayerId = RENDER_LAYER_NONE,
			const DG::float3& layerOffset = DG::float3(0.0f, 0.0f, 0.0f));

		void SwapLayers(int layer1, int layer2);
		void SwapTilesets(int tileset1, int tileset2);

		void RemoveLayer(int laye, bool bPreserveLayerOrder = true);
		void RemoveTileset(int tileset, bool bPreserveTilesetOrder = true);
	};
}