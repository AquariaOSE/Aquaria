#include "Tileset.h"
#include "SimpleIStringStream.h"
#include "Base.h"
#include "ttvfs_stdio.h"
#include "TextureMgr.h"
#include "Core.h"

bool Tileset::loadFile(const char *fn, const unsigned char *usedIdx, size_t usedIdxLen)
{
	elementTemplates.clear();

	InStream in(fn);
	if(!in)
		return false;

	std::string line, gfx;
	while (std::getline(in, line))
	{
		int idx=-1, w=0, h=0;
		SimpleIStringStream is(line.c_str(), SimpleIStringStream::REUSE);
		is >> idx >> gfx >> w >> h;
		if(idx >= 0)
		{
			ElementTemplate t;
			t.idx = idx;
			t.gfx = gfx;
			t.w = w;
			t.h = h;
			elementTemplates.push_back(t);
		}
	}
	in.close();

	std::sort(elementTemplates.begin(), elementTemplates.end());

	// begin preloading textures

	std::vector<std::string> usedTex;
	usedTex.reserve(elementTemplates.size()); // optimistically assume all textures in the tileset are used

	for (size_t i = 0; i < elementTemplates.size(); i++)
	{
		size_t idx = elementTemplates[i].idx;
		if (!usedIdx || (idx < usedIdxLen && usedIdx[idx]))
			usedTex.push_back(elementTemplates[i].gfx);
	}
	std::sort(usedTex.begin(), usedTex.end());
	// drop duplicates
	usedTex.resize(std::distance(usedTex.begin(), std::unique(usedTex.begin(), usedTex.end())));

	std::ostringstream os;
	os << "Loading " << usedTex.size()
		<< " used textures out of the " << elementTemplates.size() << " tileset entries";
	debugLog(os.str());

	// preload all used textures
	if(usedTex.size())
		core->texmgr.loadBatch(NULL, &usedTex[0], usedTex.size());

	return true;
}

void Tileset::clear()
{
	elementTemplates.clear();
}

ElementTemplate *Tileset::getByIdx(size_t idx)
{
	for (size_t i = 0; i < elementTemplates.size(); i++)
	{
		if (elementTemplates[i].idx == idx)
		{
			return &elementTemplates[i];
		}
	}
	return 0;
}

Texture* ElementTemplate::getTexture()
{
	if(tex)
		return tex.content();

	tex = core->getTexture(gfx);
	if(!w)
		w = tex->width;
	if(!h)
		h = tex->height;

	return tex.content();

}

