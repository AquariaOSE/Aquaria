#ifndef BBGE_TILESET_H
#define BBGE_TILESET_H

#include "Vector.h"
#include <vector>
#include "Texture.h"
#include "RenderGrid.h"

class DynamicRenderGrid;

class ElementTemplate
{
public:
	ElementTemplate() { w=0; h=0; idx=-1; tc.setStandard(); grid = NULL; }
	~ElementTemplate();
	inline bool operator<(const ElementTemplate& o) const { return idx < o.idx; }

	void finalize(); // call after settings params

	// lazily assigned when tex is loaded
	CountedPtr<Texture> tex; // NULL if failed to load or not yet loaded
	float w,h; // custom size if used, otherwise texture size
	RenderGrid *grid; // NULL if default, otherwise we own this

	// fixed
	TexCoordBox tc;
	size_t idx;
	std::string gfx;

private:
	ElementTemplate(const ElementTemplate&); // no copy
	ElementTemplate& operator=(const ElementTemplate&); // no assign
};

class Tileset
{
public:
	Tileset();
	~Tileset();

	// pass usedIdx == NULL to preload all textures from tileset
	// pass usedIdx != NULL to preload only textures where usedIdx[i] != 0
	bool loadFile(const char *fn, const unsigned char *usedIdx, size_t usedIdxLen);
	void clear();

	// return valid ET if found, or creates a dummy if not. never returns NULL.
	const ElementTemplate *getByIdx(size_t idx);

	// search for non-dummy ET in a given direction. used to cycle through ETs.
	// never returns dummy ET. May return NULL.
	const ElementTemplate *getAdjacent(size_t idx, int direction, bool wraparound);

	std::vector<ElementTemplate*> elementTemplates;

private:
	ElementTemplate *_getAdjacent(size_t idx, int direction, bool wraparound);

	std::vector<ElementTemplate*> dummies;
};


#endif
