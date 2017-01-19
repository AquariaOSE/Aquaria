#ifndef WORLDMAP_H
#define WORLDMAP_H

#include <string>
#include "Vector.h"
#include "GameEnums.h"

#define MAPVIS_SUBDIV 64

class Quad;

struct WorldMapTile
{
	WorldMapTile();
	~WorldMapTile();

	void markVisited(int left, int top, int right, int bottom);
	void dataToString(std::ostringstream &os);
	void stringToData(std::istringstream &is);
	const unsigned char *getData() const {return data;}

	std::string name;
	Vector gridPos;
	float scale, scale2;
	bool revealed, prerevealed;
	int layer, index;
	int stringIndex;

	Quad *q;

protected:
	unsigned char *data;
};

struct WorldMap
{
	WorldMap();
	void load();
	void save();
	void hideMap();
	void revealMap(const std::string &name);
	WorldMapTile *getWorldMapTile(const std::string &name);
	size_t getNumWorldMapTiles();
	WorldMapTile *getWorldMapTile(size_t index);

	WorldMapTile *getWorldMapTileByIndex(int index);
	void revealMapIndex(int index);

	int gw, gh;
	typedef std::vector<WorldMapTile> WorldMapTiles;
	WorldMapTiles worldMapTiles;

private:
	void _load(const std::string &file);
};

#endif
