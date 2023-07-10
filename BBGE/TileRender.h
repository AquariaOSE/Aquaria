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
	virtual void onRender(const RenderState& rs) const;
	virtual void onUpdate(float dt);

private:
};


#endif // BBGE_TILERENDER_H
