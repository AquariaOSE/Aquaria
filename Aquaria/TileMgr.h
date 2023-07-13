#ifndef TILEMGR_H
#define TILEMGR_H

#include "Tile.h"
#include "Tileset.h"
#include "GameEnums.h"

enum { MAX_TILE_LAYERS = 16 };

// Legacy!
// These are needed as-is when loading and saving maps.
// The values are stored in the map and can't be changed.
enum ElementFlag
{
	EF_NONE			= 0,
	EF_SOLID		= 1,
	EF_MOVABLE		= 2, // unused
	EF_HURT			= 3,
	EF_SOLID2		= 4,
	EF_SOLID3		= 5,
	//EF_MAX			= 6
};

// temporary struct to fill grid with
struct GridFiller
{
	ObsType obs;
	bool trim;

	const Texture *texture;
	Vector scale, position;
	float width, height, rotation;
	bool fh;
};

// struct with all tile properties for batch creation of tiles
struct TileDef
{
	TileDef(unsigned lr);
	TileDef(unsigned lr, const TileData& t);

	unsigned layer, idx;
	int x, y, rot, fh, fv, ef, efxIdx, repeat, tag;
	float sx, sy, rsx, rsy;
};

class TileMgr
{
public:
	TileMgr();
	~TileMgr();

	void update(float dt);
	void destroy();

	void loadTileEffects(const char *fn);

	size_t getNumTiles() const;
	TileStorage::Sizes getStats() const;
	void clearTiles();
	void doTileInteraction(const Vector& pos, const Vector& vel, float mult, float touchWidth);

	// don't store the returned pointer anywhere! Use it to set extra things, but then drop it.
	// It will become invalid when more tiles are added.
	// This function is also quite ineffieient and is intended for editor use only!
	TileData *createOneTile(unsigned tilesetID, unsigned layer, float x, float y, ElementFlag ef = EF_NONE, int effidx = -1);

	void createTiles(const TileDef *defs, size_t n);
	void exportGridFillers(std::vector<GridFiller>& fillers) const;

	TileStorage tilestore[MAX_TILE_LAYERS];
	TileEffectStorage tileEffects;
	Tileset tileset;

	static TileFlags GetTileFlags(ElementFlag ef);
	static ElementFlag GetElementFlag(TileFlags tf);

private:
	TileData *_createTile(unsigned tilesetID, unsigned layer, float x, float y, ElementFlag ef = EF_NONE, int effidx = -1);

	TileMgr(const TileMgr&); // no-copy
};


#endif
