#include "TileMgr.h"
#include <assert.h>
#include "Base.h"
#include "ttvfs_stdio.h"


static const unsigned s_tileFlags[] =
{
	/* EF_NONE   -> */ TILEFLAG_NONE,
	/* EF_SOLID	 -> */ TILEFLAG_SOLID,
	/* EF_MOVABLE-> */ TILEFLAG_NONE, /* unused */
	/* EF_HURT   -> */ TILEFLAG_SOLID | TILEFLAG_HURT,
	/* EF_SOLID2 -> */ TILEFLAG_SOLID | TILEFLAG_SOLID_THICK,
	/* EF_SOLID3 -> */ TILEFLAG_SOLID | TILEFLAG_SOLID_THICK | TILEFLAG_SOLID_IN
};


TileFlags TileMgr::GetTileFlags(ElementFlag ef)
{
	//compile_assert(Countof(s_tileFlags) == EF_MAX);
	unsigned tf = TILEFLAG_NONE;
	if(unsigned(ef) < Countof(s_tileFlags))
		tf = s_tileFlags[ef];
	return (TileFlags)tf;
}

ElementFlag TileMgr::GetElementFlag(TileFlags tf)
{
	unsigned ef = EF_NONE;
	if(tf & TILEFLAG_SOLID)
	{
		if(tf & TILEFLAG_HURT)
			ef = EF_HURT;
		else if(tf & TILEFLAG_SOLID_IN)
			ef = EF_SOLID3;
		else if(tf & TILEFLAG_SOLID_THICK)
			ef = EF_SOLID2;
		else
			ef = EF_SOLID;
	}
	return (ElementFlag)ef;
}


TileMgr::TileMgr()
{
}

TileMgr::~TileMgr()
{
	destroy();
}

void TileMgr::update(float dt)
{
	tileEffects.update(dt);
	for(size_t i = 0; i < Countof(tilestore); ++i)
		tilestore[i].update(dt);
}

void TileMgr::destroy()
{
	clearTiles();
	tileset.clear();
	tileEffects.clear();
}

size_t TileMgr::getNumTiles() const
{
	size_t num = 0;
	for(size_t i = 0; i < Countof(tilestore); ++i)
		num += tilestore[i].size();
	return num;
}

TileStorage::Sizes TileMgr::getStats() const
{
	TileStorage::Sizes tsz {};
	for(size_t i = 0; i < Countof(tilestore); ++i)
	{
		TileStorage::Sizes sz = tilestore[i].stats();
		tsz.tiles += sz.tiles;
		tsz.collide += sz.collide;
		tsz.update += sz.update;
	}
	return tsz;
}

void TileMgr::clearTiles()
{
	for(size_t i = 0; i < Countof(tilestore); ++i)
		tilestore[i].destroyAll();
}

void TileMgr::doTileInteraction(const Vector& pos, const Vector& vel, float mult, float touchWidth)
{
	for(size_t i = 0; i < Countof(tilestore); ++i)
		tilestore[i].doInteraction(pos, vel, mult, touchWidth);
}

TileData* TileMgr::createOneTile(unsigned tilesetID, unsigned layer, float x, float y, ElementFlag ef, int effidx)
{
	TileData *t = _createTile(tilesetID, layer, x, y, ef, effidx);
	if(t)
	{
		TileStorage& ts = tilestore[layer];
		ts.refreshAll();
	}
	return t;
}

void TileMgr::createTiles(const TileDef* defs, size_t n)
{
	char used[Countof(tilestore)] = {0};
	for(size_t i = 0; i < n; ++i)
	{
		const TileDef& d = defs[i];
		TileData *t = _createTile(d.idx, d.layer, d.x, d.y, (ElementFlag)d.ef, d.efxIdx);
		if(t)
		{
			used[d.layer] = 1;

			if(d.fh)
				t->flags |= TILEFLAG_FH;
			if(d.fv)
				t->flags |= TILEFLAG_FV;
			if(d.repeat)
				t->setRepeatOn(d.rsx, d.rsy);

			t->rotation = d.rot;
			t->tag = d.tag;
			t->scalex = d.sx;
			t->scaley = d.sy;
		}
	}

	for(size_t i = 0; i < Countof(used); ++i)
		if(used[i])
			tilestore[i].refreshAll();
}

void TileMgr::exportGridFillers(std::vector<GridFiller>& fillers) const
{
	for(size_t k = 0; k < Countof(tilestore); ++k)
	{
		const TileStorage& ts = tilestore[k];
		const size_t N = ts.size();
		for(size_t i = 0; i < N; ++i)
		{
			const TileData& t = ts.tiles[i];
			if((t.flags & TILEFLAG_SOLID) && !(t.flags & TILEFLAG_HIDDEN) && t.et)
			{
				GridFiller gf;
				gf.fh = !!(t.flags & TILEFLAG_FH);
				//gf.fv = !!(t.flags & TILEFLAG_FV); // doesn't exist; vertical flip is never considered for grid collision
				gf.position = Vector(t.x, t.y);
				gf.rotation = t.rotation;
				gf.scale = Vector(t.scalex, t.scaley);
				gf.texture = t.et->tex.content();
				gf.width = t.et->w;
				gf.height = t.et->h;
				gf.trim = !(t.flags & TILEFLAG_SOLID_THICK);
				if(t.flags & TILEFLAG_HURT)
					gf.obs = OT_HURT;
				else if(t.flags & TILEFLAG_SOLID_IN)
					gf.obs = OT_INVISIBLEIN;
				else
					gf.obs = OT_INVISIBLE;

				fillers.push_back(gf);
			}
		}
	}
}

TileData* TileMgr::_createTile(unsigned tilesetID, unsigned layer,  float x, float y, ElementFlag ef, int effidx)
{
	if(layer >= Countof(tilestore))
		return NULL;

	TileData t;
	t.x = x;
	t.y = y;
	t.rotation = 0;
	t.scalex = 1;
	t.scaley = 1;
	//t.beforeScaleOffsetX = 0;
	//t.beforeScaleOffsetY = 0;
	t.flags = GetTileFlags(ef);
	t.tag = 0;
	t.et = tileset.getByIdx(tilesetID);
	assert(t.et);
	/* t.eff = */ tileEffects.assignEffect(t, effidx);

	TileStorage& ts = tilestore[layer];
	ts.tiles.push_back(t);
	return &ts.tiles.back();
}

void TileMgr::loadTileEffects(const char *fn)
{
	debugLog(fn);
	InStream inFile(fn);
	if(!inFile)
	{
		errorLog("TileMgr::loadTileEffects: Failed to open file");
		return;
	}

	clearTiles();
	tileEffects.clear();

	std::string line;
	while (std::getline(inFile, line))
	{
		debugLog("Line: " + line);
		std::istringstream is(line);
		TileEffectConfig e;
		int efxType = EFX_NONE;

		std::string type;
		is >> e.index >> type;
		if (type == "EFX_SEGS")
		{
			efxType = EFX_SEGS;
			is >> e.u.segs.x >> e.u.segs.y >> e.u.segs.dgox >> e.u.segs.dgoy >> e.u.segs.dgmx >> e.u.segs.dgmy >> e.u.segs.dgtm >> e.u.segs.dgo;
		}
		else if (type == "EFX_WAVY")
		{
			debugLog("loading wavy");
			efxType = EFX_WAVY;
			is >> e.u.wavy.segsy >> e.u.wavy.radius >> e.u.wavy.flip;

		}
		else if (type == "EFX_ALPHA")
		{
			efxType = EFX_ALPHA;
			int loop_unused, blend; // loop is unused because we always loop forever
			is >> blend >> e.u.alpha.val0 >> e.u.alpha.val1 >> e.u.alpha.time >> loop_unused >> e.u.alpha.pingpong >> e.u.alpha.ease;
			e.u.alpha.blend = blend < _BLEND_MAXSIZE ? (BlendType)blend : BLEND_DISABLED;
		}
		if(efxType != EFX_NONE)
		{
			e.type = (EFXType)efxType;
			const size_t newsize = size_t(e.index) + 1;
			if(tileEffects.configs.size() < newsize)
				tileEffects.configs.resize(newsize);
			tileEffects.configs[e.index] = e;
		}
		else
			errorLog("elementeffects.txt: Error on this line:\n" + line);
	}
	inFile.close();

	tileEffects.finalize();
}

TileDef::TileDef(unsigned lr)
	: layer(lr), idx(0), x(0), y(0), rot(0), fh(0), fv(0), ef(0), efxIdx(-1), repeat(0)
	, tag(0), sx(1), sy(1), rsx(1), rsy(1)
{
}

TileDef::TileDef(unsigned lr, const TileData& t)
	: layer(lr), idx((unsigned)t.et->idx), x(t.x), y(t.y), rot(t.rotation)
	, fh(!!(t.flags & TILEFLAG_FH))
	, fv(!!(t.flags & TILEFLAG_FV))
	, ef(TileMgr::GetElementFlag((TileFlags)t.flags))
	, efxIdx(t.eff ? t.eff->efxidx : -1)
	, repeat(!!(t.flags & TILEFLAG_REPEAT))
	, tag(t.tag)
	, sx(t.scalex), sy(t.scaley)
	, rsx(1), rsy(1)
{
	if(t.flags & TILEFLAG_REPEAT)
	{
		rsx = t.rep->texscaleX;
		rsy = t.rep->texscaleY;
	}
}
