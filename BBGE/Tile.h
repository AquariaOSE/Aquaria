#ifndef BBGE_TILE_H
#define BBGE_TILE_H

#include <vector>
#include "Vector.h"

class ElementTemplate;
class Texture;

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
*/

enum TileFlags
{
    TILEFLAG_NONE        = 0,
    TILEFLAG_REPEAT      = 0x01, // texture repeats and uses texscale for the repeat factor
    TILEFLAG_SOLID       = 0x02, // generates OT_INVISIBLE
    TILEFLAG_SOLID_THICK = 0x04, // generates more OT_INVISIBLE
    TILEFLAG_SOLID_IN    = 0x08, // instead of OT_INVISIBLE, generate OT_INVISIBLEIN
    TILEFLAG_HURT        = 0x10, // always generate OT_HURT
    TILEFLAG_FH          = 0x20, // flipped horizontally
};

// sort-of-POD
struct TileData
{
    float x, y, rotation, texscale;
    Vector scale, beforeScaleOffset;
    int efx;
    unsigned flags; // TileFlags
    unsigned tag;
    ElementTemplate *et;
};


class TileStorage
{
public:
    std::vector<TileData> tiles;
    void refresh(); // call when adding/removing/reordering tiles or changing efx
};



#endif // BBGE_TILE_H
