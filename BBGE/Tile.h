#ifndef BBGE_TILE_H
#define BBGE_TILE_H

#include <vector>
#include "Vector.h"
#include "EngineEnums.h"

// A Tile is a very stripped down RenderObject that bypasses the default
// rendering pipeline for efficiency reasons.
// Use a TileRender to draw a list of Tiles.

/* Properties of tiles:
- Lots of these exist. Need to store & render efficiently
- Usually no dynamic behavior, BUT:
  * can have a TileEffect that requires updating (sometimes)
  * can react to entities nearby (rare)
  * may have a draw grid (wobbly plants etc)
- Only modified in the editor -> Can be slow to modify or reorder
- Render order must be strictly followed to get the correct visual overlapping
- Never part of a parent/child hierarchy, all tiles are standalone
- Does not have offset, internalOffset, gravity, etc etc that RenderObject has
- Parallax scroll factor is solely influenced by layer, not individually
- RGB is never tinted, alpha may come from efx

Further observations:
- Aside from EFX_WAVY, all tiles with the same effect index can share a global modulator,
  ie. the alpha value is the same per-tile unless the tile is spawned when the map is in progress.
  And on map reload everything is back to the same value for each tile with the same effect and params.
  So we can totally exclude the editor.

Gotaches:
- Keeping a pointer to a TileData is not safe.
*/

class ElementTemplate;
class Texture;
class RenderGrid;
class TileRender;

enum EFXType
{
	EFX_SEGS,
	EFX_ALPHA,
	EFX_WAVY
};

// static configuration for one effect type
struct TileEffectConfig
{
public:
	EFXType type;
	unsigned index;

	union
	{
		struct
		{
			int x, y;
			float dgox, dgoy, dgmx, dgmy, dgtm;
			bool dgo;
		} segs;

		struct
		{
			float radius, min, max;
			int segsy;
			bool flip;
		} wavy;

		struct
		{
			float val0, val1, time;
			bool pingpong, ease;
			BlendType blend;
		} alpha;
	} u;
};

enum TileFlags
{
	TILEFLAG_NONE        = 0,
	TILEFLAG_REPEAT      = 0x01, // texture repeats and uses texscale for the repeat factor
	TILEFLAG_SOLID       = 0x02, // generates OT_INVISIBLE
	TILEFLAG_SOLID_THICK = 0x04, // generates more OT_INVISIBLE
	TILEFLAG_SOLID_IN    = 0x08, // instead of OT_INVISIBLE, generate OT_INVISIBLEIN
	TILEFLAG_HURT        = 0x10, // always generate OT_HURT
	TILEFLAG_FH          = 0x20, // flipped horizontally
	TILEFLAG_OWN_EFFDATA = 0x40, // tile owns its TileEffectData, can modify & must delete
	TILEFLAG_HIDDEN      = 0x80, // don't render tile
	TILEFLAG_SELECTED    = 0x100
};

struct TileData;

struct TileEffectData
{
	TileEffectData(const TileEffectConfig& cfg);
	~TileEffectData();
	void update(float dt, const TileData *t); // optional t needed for EFX_WAVY
	void doInteraction(const TileData& t, const Vector& pos, const Vector& vel, float mult, float touchWidth);

	const EFXType efxtype;
	const unsigned efxidx; // index to ElementEffect
	RenderGrid *grid;
	InterpolatedVector alpha;
	BlendType blend;

	struct Wavy
	{
		std::vector<float> wavy, wavySave;
		Vector touchVel;
		float angleOffset, magnitude, lerpIn;
		float min, max;
		float hitPerc, effectMult;
		bool waving, flip, touching;
		void update(float dt);
	};
	Wavy wavy;
};

// POD and as compact as possible
// the idea is that these are linearly adjacent in memory in the order they are rendered,
// to maximize cache & prefetch efficiency
struct TileData
{
	float x, y, rotation, texscale;
	float scalex, scaley;
	float beforeScaleOffsetX, beforeScaleOffsetY;
	unsigned flags; // TileFlags
	unsigned tag;
	ElementTemplate *et; // texture, texcoords, etc is here
	TileEffectData *eff;
};

class TileEffectStorage
{
public:
	void assignEffect(TileData& t, int index) const;
	void update(float dt);

	std::vector<TileEffectData*> prepared;
	std::vector<TileEffectConfig> configs;
};

class TileStorage
{
	friend class TileRender;
public:
	TileStorage(const TileEffectStorage& eff);
	~TileStorage();

	void moveToFront(size_t idx);
	void moveToBack(size_t idx);
	void moveToOther(TileStorage& other, const size_t *indices, size_t n);
	void deleteSome(const size_t *indices, size_t n);

	void setTag(unsigned tag, const size_t *indices, size_t n);
	void setEffect(int idx, const size_t *indices, size_t n);

	void update(float dt);
	void doInteraction(const Vector& pos, const Vector& vel, float mult, float touchWidth);
	void refreshAll(); // call this after changing properties or moving to front/back
	void destroyAll();

	void clearSelection();

private:

	std::vector<TileData> tiles;
	std::vector<size_t> indicesToUpdate;
	std::vector<size_t> indicesToCollide;
	const TileEffectStorage& effstore;

	void _refreshTile(const TileData& t);
	void _moveToFront(size_t idx);
	void _moveToBack(size_t idx);
};



#endif // BBGE_TILE_H
