#ifndef BBGE_TILERENDER_H
#define BBGE_TILERENDER_H

#include "RenderObject.h"
#include "Tile.h"

class Tileset;

class TileRender : public RenderObject
{
private:
	const TileStorage& storage;
public:

	TileRender(const TileStorage& tiles);
	virtual ~TileRender();
	virtual void onRender(const RenderState& rs) const OVERRIDE;
	virtual void onUpdate(float dt) OVERRIDE;

	bool renderBorders;

	static Vector GetTagColor(int tag);
};


#endif // BBGE_TILERENDER_H
