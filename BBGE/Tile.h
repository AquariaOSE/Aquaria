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

Assumptions:
- Most tiles that exist are going to be rendered
- Only few tiles have an effect attached

Gotaches:
- Keeping a pointer to a TileData is not safe.
- Tile indexes are not stable. Moving a tile changes the index it can be addressed with
*/

class ElementTemplate;
class Texture;
class RenderGrid;
class TileRender;

enum EFXType
{
	EFX_NONE,
	EFX_SEGS,
	EFX_ALPHA,
	EFX_WAVY
};

// static configuration for one effect type. POD.
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
	TILEFLAG_OWN_EFFDATA = 0x40, // tile owns its TileEffectData, can update, must delete
	TILEFLAG_HIDDEN      = 0x80, // don't render tile
	TILEFLAG_SELECTED    = 0x100, // ephemeral: selected in editor
	TILEFLAG_EDITOR_HIDDEN = 0x200  // tile is hidden for editor reasons. temporarily set when multi-selecting and moving. doesn't count as hidden externally and is only for rendering.
};

struct TileData;

struct TileEffectData
{
	TileEffectData(const TileEffectConfig& cfg);
	~TileEffectData();
	void update(float dt, const TileData *t); // optional t needed for EFX_WAVY
	void doInteraction(const TileData& t, const Vector& pos, const Vector& vel, float mult, float touchWidth);

	const EFXType efxtype;
	const unsigned efxidx; // index of TileEffect
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

private:
	TileEffectData(const TileEffectData&); // no-copy
};

// POD and as compact as possible. Intended for rendering as quickly as possible.
// the idea is that these are linearly adjacent in memory in the order they are rendered,
// to maximize cache & prefetch efficiency
struct TileData
{
	float x, y, scalex, scaley, texscaleX, texscaleY;
	float beforeScaleOffsetX, beforeScaleOffsetY; // almost always 0. // TODO: this is nasty, ideally get rid of this
	float rotation;
	unsigned flags; // TileFlags
	unsigned tag; // FIXME: make this int
	const ElementTemplate *et; // never NULL. texture, texcoords, etc is here. // TODO: maybe replace with unsigned tilesetID? but that's an extra indirection or two during rendering...
	TileEffectData *eff; // mostly NULL

	// helpers for external access
	inline void setVisible(bool on) { if(on) flags &= ~TILEFLAG_HIDDEN; else flags |= TILEFLAG_HIDDEN; }
	inline bool isVisible() const { return !(flags & TILEFLAG_HIDDEN); }
	bool isCoordinateInside(float cx, float cy, float minsize = 0) const;
};

class TileEffectStorage
{
public:
	TileEffectStorage();
	~TileEffectStorage();
	void finalize(); // first fill configs[], then call this
	void assignEffect(TileData& t, int index) const;
	void update(float dt);
	void clear(); // do NOT call this while there are tiles that may reference one in prepared[]

	std::vector<TileEffectConfig> configs;

private:
	void clearPrepared();
	std::vector<TileEffectData*> prepared;

	TileEffectStorage(const TileEffectStorage&); // no-copy
};

class TileStorage
{
	friend class TileRender;
public:
	TileStorage();
	~TileStorage();

	void moveToFront(const size_t *indices, size_t n);
	void moveToBack(const size_t *indices, size_t n);

	// returns starting index of new tiles. Since new tiles are always appended at the end,
	// the new indices corresponding to the moved tiles are [retn .. retn+n)
	size_t moveToOther(TileStorage& other, const size_t *indices, size_t n);
	size_t cloneSome(const TileEffectStorage& effstore, const size_t *indices, size_t n);

	void deleteSome(const size_t *indices, size_t n);

	void setTag(unsigned tag, const size_t *indices, size_t n);
	void setEffect(const TileEffectStorage& effstore, int idx, const size_t *indices, size_t n);

	void changeFlags(unsigned flagsToSet, unsigned flagsToUnset, const size_t *indices, size_t n);

	void update(float dt);
	void doInteraction(const Vector& pos, const Vector& vel, float mult, float touchWidth);
	void refreshAll(); // call this after changing properties or moving to front/back
	void destroyAll();

	void clearSelection();

	struct Sizes
	{
		size_t tiles, update, collide;
	};
	Sizes stats() const;
	size_t size() const { return tiles.size(); }


	std::vector<TileData> tiles; // must call refreshAll() after changing this

private:

	std::vector<size_t> indicesToUpdate;
	std::vector<size_t> indicesToCollide;

	void _refreshTile(const TileData& t);
	void _moveToFront(const size_t *indices, size_t n);
	void _moveToBack(const size_t *indices, size_t n);
	void _moveToPos(size_t where, const size_t *indices, size_t n);

	TileStorage(const TileStorage&); // no-copy
};



#endif // BBGE_TILE_H
