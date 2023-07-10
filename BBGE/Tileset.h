#ifndef BBGE_TILESET_H
#define BBGE_TILESET_H

#include "Vector.h"
#include <vector>
#include "Texture.h"

class ElementTemplate
{
public:
	ElementTemplate() { w=0; h=0; idx=-1; tu1=tv1=0; tu2=tv2=1; }
	inline bool operator<(const ElementTemplate& o) const { return idx < o.idx; }

	Texture *getTexture(); // loads if not already loaded

	// lazily assigned when tex is loaded
	CountedPtr<Texture> tex;
	unsigned w,h; // custom size if used, otherwise texture size

	// fixed
	float tu1, tu2, tv1, tv2; // texcoords
	size_t idx;
	std::string gfx;
};

class Tileset
{
public:
	// pass usedIdx == NULL to preload all textures from tileset
	// pass usedIdx != NULL to preload only textures where usedIdx[i] != 0
	bool loadFile(const char *fn, const unsigned char *usedIdx, size_t usedIdxLen);
	void clear();

	ElementTemplate *getByIdx(size_t idx);

	std::vector<ElementTemplate> elementTemplates;
};


#endif
