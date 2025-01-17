#ifndef WORLDMAP_H
#define WORLDMAP_H

#include <string>
#include "Vector.h"
#include "GameEnums.h"
#include "Image.h"
#include "Refcounted.h"
#include "Texture.h"
#include "SimpleIStringStream.h"

#define MAPVIS_SUBDIV 64
#define WORLDMAP_REVEALED_BUT_UNEXPLORED_ALPHA 0x60

class Quad;

struct WorldMapTile
{
	WorldMapTile();
	~WorldMapTile();

	void markVisited(int left, int top, int right, int bottom);
	void dataToString(std::ostringstream &os) const;
	void stringToData(SimpleIStringStream &is);

	ImageData generateAlphaImage(size_t w, size_t h); // free() data when no longer needed
	bool updateDiscoveredTex();

	std::string name;
	Vector gridPos;
	float scale, scale2;
	bool revealed, prerevealed, dirty;
	int layer, index;
	int stringIndex;


	Array2d<unsigned char> visited;

	CountedPtr<Texture> originalTex, generatedTex;
};

struct WorldMap
{
	WorldMap();
	void load();
	void save();
	WorldMapTile *getWorldMapTile(const std::string &name);
	WorldMapTile *getWorldMapTileByIndex(int index);
	bool revealMap(const std::string &name);
	bool revealMapIndex(int index);
	bool forgetMap(const std::string &name);

	int gw, gh;
	typedef std::vector<WorldMapTile> WorldMapTiles;
	WorldMapTiles worldMapTiles;

private:
	void _load(const std::string &file);
};

#endif
