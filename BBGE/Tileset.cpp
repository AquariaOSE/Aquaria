#include "Tileset.h"
#include "SimpleIStringStream.h"
#include "Base.h"
#include "ttvfs_stdio.h"
#include "TextureMgr.h"
#include "Core.h"

Tileset::Tileset()
{
}

Tileset::~Tileset()
{
	clear();
}

bool Tileset::loadFile(const char *fn, const unsigned char *usedIdx, size_t usedIdxLen)
{
	clear();

	InStream in(fn);
	if(!in)
		return false;

	bool warn = false;
	std::string line, gfx;
	while (std::getline(in, line))
	{
		gfx.clear();
		int idx=-1, w=0, h=0;
		SimpleIStringStream is(line.c_str(), SimpleIStringStream::REUSE);
		is >> idx >> gfx >> w >> h;
		if(idx >= 0 && !gfx.empty())
		{
			if(idx < 1024)
			{
				ElementTemplate *et = new ElementTemplate;
				et->idx = idx;
				et->gfx = gfx;
				et->w = w;
				et->h = h;
				elementTemplates.push_back(et);
			}
			else
				warn = true;
		}
	}
	in.close();

	if(warn)
		errorLog("Tileset indices of 1024 and above are reserved; ignored during load");

	std::sort(elementTemplates.begin(), elementTemplates.end());

	// begin preloading textures

	std::vector<std::string> usedTex;
	usedTex.reserve(elementTemplates.size()); // optimistically assume all textures in the tileset are used

	for (size_t i = 0; i < elementTemplates.size(); i++)
	{
		size_t idx = elementTemplates[i]->idx;
		if (!usedIdx || (idx < usedIdxLen && usedIdx[idx]))
			usedTex.push_back(elementTemplates[i]->gfx);
	}
	std::sort(usedTex.begin(), usedTex.end());
	// drop duplicates
	usedTex.resize(std::distance(usedTex.begin(), std::unique(usedTex.begin(), usedTex.end())));

	{
		std::ostringstream os;
		os << "Loading " << usedTex.size()
			<< " used textures out of the " << elementTemplates.size() << " tileset entries";
		debugLog(os.str());
	}

	// preload all used textures
	size_t loaded = 0;
	if(usedTex.size())
		loaded = core->texmgr.loadBatch(NULL, &usedTex[0], usedTex.size());

	{
		std::ostringstream os;
		os << "Loaded " << loaded << " textures successfully";
		debugLog(os.str());
	}

	// finalize
	size_t nfailed = 0;
	std::ostringstream failed;
	for (size_t i = 0; i < elementTemplates.size(); i++)
	{
		ElementTemplate *et = elementTemplates[i];
		// only check those that are actualy loaded; otherwise this would load in textures
		// that we didn't bother to batch-load above
		if(!usedIdx || (et->idx < usedIdxLen && usedIdx[et->idx]))
		{
			et->finalize(); // assigns width/height and caches texture pointer
			if(!et->tex)
			{
				++nfailed;
				failed << et->gfx << " ";
			}
		}
	}

	if(nfailed)
	{
		std::ostringstream os;
		os << "The following " << nfailed << " textures failed to load and would be used by tiles:";
		debugLog(os.str());
		debugLog(failed.str());
	}

	return true;
}

void Tileset::clear()
{
	for(size_t i = 0; i < dummies.size(); ++i)
		delete dummies[i];
	dummies.clear();

	for(size_t i = 0; i < elementTemplates.size(); ++i)
		delete elementTemplates[i];
	elementTemplates.clear();
}

const ElementTemplate *Tileset::getByIdx(size_t idx)
{
	for (size_t i = 0; i < elementTemplates.size(); i++)
	{
		ElementTemplate *et = elementTemplates[i];
		if (et->idx == idx)
		{
			et->finalize(); // HACK: make sure the texture is loaded before this gets used
			return et;
		}
	}

	// a tile that gets an ET attached must remember its tileset id even if the entry is not present
	// in the tileset. since the tile does not store the idx as an integer, we need to return a dummy element.
	for (size_t i = 0; i < dummies.size(); i++)
	{
		ElementTemplate *et = dummies[i];
		if (et->idx == idx)
			return et;
	}

	{
		std::ostringstream os;
		os << "Tileset idx " << idx << " not found, creating dummy";
		debugLog(os.str());
	}

	ElementTemplate *dummy = new ElementTemplate;
	dummy->idx = idx;
	dummy->finalize();
	dummies.push_back(dummy);

	return dummy;
}

const ElementTemplate* Tileset::getAdjacent(size_t idx, int direction, bool wraparound)
{
	ElementTemplate *et = _getAdjacent(idx, direction, wraparound);
	if(et)
		et->finalize(); // load just in case
	return et;
}


ElementTemplate::~ElementTemplate()
{
	delete grid;
}

void ElementTemplate::finalize()
{
	if(!gfx.empty())
		tex = core->getTexture(gfx); // may end up NULL
	if(tex)
	{
		if(!w)
			w = tex->width;
		if(!h)
			h = tex->height;
	}
	else
	{
		if(!w)
			w = 64;
		if(!h)
			h = 64;
	}

	if(tc.isStandard())
	{
		delete grid;
		grid = NULL;
	}
	else
	{
		if(!grid)
			grid = new RenderGrid;
		grid->init(2, 2, tc);
	}
}

ElementTemplate * Tileset::_getAdjacent(size_t idx, int direction, bool wraparound)
{
	assert(direction == 1 || direction == -1);
	const size_t maxn = elementTemplates.size();
	size_t closest = 0;
	int mindiff = 0;
	for (size_t i = 0; i < maxn; i++)
	{
		if (elementTemplates[i]->idx == idx)
		{
			if(wraparound)
			{
				if(!i && direction < 0)
					return elementTemplates.back();
				if(i + direction >= maxn)
					return elementTemplates[0];
			}
			else

			i += direction; // may underflow
			return i < maxn ? elementTemplates[i] : NULL;
		}
		int diff = labs((int)elementTemplates[i]->idx - (int)idx);
		if(diff < mindiff || !mindiff)
		{
			mindiff = diff;
			closest = i;
		}
	}

	// not found? pick whatever was closest to the non-existing idx, and go back/forward from there

	// avoid going "twice" in the given direction
	if(closest < idx && direction < 0)
		direction = 0; // this is already a step back, don't step again
	else if(closest > idx && direction > 0)
		direction = 0; // this is already a step forward, don't step again
	else if(wraparound)
	{
		if(!closest && direction < 0)
			return elementTemplates.back();
		if(closest + direction >= maxn)
			return elementTemplates[0];
	}

	size_t i = closest + direction;
	return i < maxn ? elementTemplates[i] : NULL;
}
