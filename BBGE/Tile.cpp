#include "Tile.h"
#include "RenderGrid.h"
#include "Tileset.h"
#include "Base.h"
#include <algorithm>
#include "Texture.h"

TileStorage::TileStorage()
{
}

TileStorage::~TileStorage()
{
	destroyAll();
}

TileStorage::Sizes TileStorage::stats() const
{
	Sizes sz;
	sz.tiles = tiles.size();
	sz.update = indicesToUpdate.size();
	sz.collide = indicesToCollide.size();
	return sz;
}

void TileStorage::moveToFront(const size_t *indices, size_t n)
{
	if(n)
	{
		_moveToFront(indices, n);
		refreshAll();
	}
}

void TileStorage::moveToBack(const size_t *indices, size_t n)
{
	if(n)
	{
		_moveToBack(indices, n);
		refreshAll();
	}
}

void TileStorage::update(float dt)
{
	for(size_t i = 0; i < indicesToUpdate.size(); ++i)
	{
		TileData& t = tiles[indicesToUpdate[i]];
		assert(t.flags & TILEFLAG_OWN_EFFDATA); // known to be set if this ends up on the list
		t.eff->update(dt, &t);
	}
}

void TileStorage::doInteraction(const Vector& pos, const Vector& vel, float mult, float touchWidth)
{
	for(size_t i = 0; i < indicesToCollide.size(); ++i)
	{
		TileData& t = tiles[indicesToCollide[i]];
		t.eff->doInteraction(t, pos, vel, mult, touchWidth);
	}
}

void TileStorage::_moveToFront(const size_t *indices, size_t n)
{
	// move tile to front -> move it to the back of the list, to be rendered last aka on top of everything else

	if(n == 1)
	{
		TileData tile = tiles[*indices];
		tiles.erase(tiles.begin() + *indices);
		tiles.push_back(tile);
		return;
	}

	_moveToPos(size(), indices, n);
}

void TileStorage::_moveToBack(const size_t *indices, size_t n)
{
	// move tile to back -> move it to the front of the list, to be rendered first aka underneath everything else

	if(n == 1)
	{
		TileData tile = tiles[*indices];
		tiles.erase(tiles.begin() + *indices);
		tiles.insert(tiles.begin(), tile);
		return;
	}

	_moveToPos(0, indices, n);
}

void TileStorage::_moveToPos(size_t where, const size_t * indices, size_t n)
{
	std::vector<size_t> tmp(indices, indices + n);
	std::sort(tmp.begin(), tmp.end());

	std::vector<TileData> tt(n);

	// sorted indices -> preserve relative order of tiles
	for(size_t i = 0; i < n; ++i)
		tt[i] = tiles[tmp[i]];

	// SORTED indices, erasing from the BACK -> we don't get a destructive index shift
	for(size_t i = tmp.size(); i --> 0; )
		tiles.erase(tiles.begin() + tmp[i]);

	tiles.insert(tiles.begin() + where, tt.begin(), tt.end());
}

size_t TileStorage::moveToOther(TileStorage& other, const size_t *indices, size_t n)
{
	const size_t firstNewIdx = other.tiles.size();

	for(size_t i = 0; i < n; ++i)
		other.tiles.push_back(tiles[indices[i]]);

	std::vector<TileData> tmp;
	tmp.swap(tiles);
	tiles.reserve(tmp.size() - n);
	for(size_t i = 0; i < tmp.size(); ++i)
	{
		for(size_t k = 0; k < n; ++i) // not particularly efficient, could be much better by sorting first but eh
			if(indices[k] == i)
				goto skip;

		tiles.push_back(tmp[i]);

		skip: ;
	}

	refreshAll();
	other.refreshAll();
	return firstNewIdx;
}

static void dropEffect(TileData& t)
{
	if(t.flags & TILEFLAG_OWN_EFFDATA)
	{
		delete t.eff;
		t.flags &= ~TILEFLAG_OWN_EFFDATA;
	}
	t.eff = NULL;
}

static void dropRepeat(TileData& t)
{
	if(t.rep)
	{
		delete t.rep;
		t.rep = NULL;
	}
}

static void dropAll(TileData& t)
{
	dropEffect(t);
	dropRepeat(t);
}


void TileStorage::deleteSome(const size_t* indices, size_t n)
{
	if(!n)
		return;

	std::vector<TileData> tmp;
	tmp.swap(tiles);
	tiles.reserve(tmp.size() - n);

	for(size_t i = 0; i < tmp.size(); ++i)
	{
		for(size_t k = 0; k < n; ++k) // not particularly efficient, could be much better by sorting first but eh
			if(indices[k] == i)
			{
				dropAll(tmp[i]);
				goto skip;
			}

		tiles.push_back(tmp[i]);

		skip: ;
	}

	refreshAll();
}

void TileStorage::destroyAll()
{
	const size_t n = tiles.size();
	for(size_t i = 0; i < n; ++i)
		dropAll(tiles[i]);
	tiles.clear();
	indicesToCollide.clear();
	indicesToUpdate.clear();
}

void TileStorage::setTag(unsigned tag, const size_t* indices, size_t n)
{
	for(size_t i = 0; i < n; ++i)
		tiles[indices[i]].tag = tag;
	// don't need to refresh here
}

void TileStorage::setEffect(const TileEffectStorage& effstore, int idx, const size_t* indices, size_t n)
{
	for(size_t i = 0; i < n; ++i)
		effstore.assignEffect(tiles[indices[i]], idx);
	refreshAll();
}

void TileStorage::changeFlags(unsigned flagsToSet, unsigned flagsToUnset, const size_t* indices, size_t n)
{
	for(size_t i = 0; i < n; ++i)
	{
		unsigned& f = tiles[indices[i]].flags;
		unsigned tmp = f & ~flagsToUnset;
		f = tmp | flagsToSet;
	}
}

void TileStorage::select(const size_t *indices, size_t n)
{
	changeFlags(TILEFLAG_SELECTED, 0, indices, n);
}

size_t TileStorage::cloneSome(const TileEffectStorage& effstore, const size_t* indices, size_t n)
{
	const size_t ret = tiles.size(); // new starting index of clone tiles

	// cloning tiles is very simple, but owned pointers will be duplicated and need to be fixed up
	const size_t N = ret + n;
	tiles.resize(N);
	for(size_t i = 0; i < n; ++i)
		tiles[ret + i] = tiles[indices[i]];

	// cleanup pointers
	for(size_t i = ret; i < N; ++i) // loop only over newly added tiles
	{
		TileData& t = tiles[i];
		if(t.rep)
		{
			t.rep = new TileRepeatData(*t.rep); // must be done BEFORE assigning eff
		}
		if((t.flags & TILEFLAG_OWN_EFFDATA) && t.eff)
		{
			int efx = t.eff->efxidx;
			t.eff = NULL; // not our pointer, just pretend it was never there
			t.flags &= TILEFLAG_OWN_EFFDATA;
			effstore.assignEffect(t, efx); // recreate effect properly
		}
	}

	refreshAll();

	return ret;
}

void TileStorage::refreshAll()
{
	indicesToCollide.clear();
	indicesToUpdate.clear();

	const size_t n = tiles.size();
	for(size_t i = 0; i < n; ++i)
	{
		TileData& t = tiles[i];
		t.refreshRepeat();
		if(!(t.flags & TILEFLAG_HIDDEN))
		{
			if(const TileEffectData *e = t.eff)
			{
				if(t.flags & TILEFLAG_OWN_EFFDATA)
				{
					indicesToUpdate.push_back(i);
					if(e->efxtype == EFX_WAVY)
						indicesToCollide.push_back(i);
				}
			}
		}
	}
}

void TileStorage::clearSelection()
{
	const size_t n = tiles.size();
	for(size_t i = 0; i < n; ++i)
		tiles[i].flags &= ~TILEFLAG_SELECTED;
}

TileEffectData::TileEffectData(const TileEffectConfig& cfg, const TileData *t)
	: efxtype(cfg.type), efxidx(cfg.index)
	, grid(NULL), alpha(1), blend(BLEND_DEFAULT)
	, ownGrid(false), shared(false)

{
	switch(cfg.type)
	{
		case EFX_NONE:
			assert(false);
			break;

		case EFX_WAVY:
		{
			assert(t);
			float bity = t->et->h/float(cfg.u.wavy.segsy);
			wavy.wavy.resize(cfg.u.wavy.segsy, 0.0f);
			wavy.flip = cfg.u.wavy.flip;
			wavy.min = bity;
			wavy.max = bity*1.2f;

			DynamicRenderGrid *g = _ensureGrid(2, cfg.u.wavy.segsy, t);
			g->gridType = GRID_UNDEFINED; // we do the grid update manually

			wavy.angleOffset = 0;
			wavy.magnitude = 0;
			wavy.lerpIn = 0;
			wavy.hitPerc = 0;
			wavy.effectMult = 0;
			wavy.waving = false;
			wavy.flip = false;
			wavy.touching = false;
		}
		break;

		case EFX_SEGS:
		{
			DynamicRenderGrid *g = _ensureGrid(cfg.u.segs.x, cfg.u.segs.y, t);
			g->setSegs(cfg.u.segs.dgox, cfg.u.segs.dgoy, cfg.u.segs.dgmx, cfg.u.segs.dgmy, cfg.u.segs.dgtm, cfg.u.segs.dgo);
		}
		break;

		case EFX_ALPHA:
		{
			alpha.x = cfg.u.alpha.val0;
			alpha.interpolateTo(cfg.u.alpha.val1, cfg.u.alpha.time, -1, cfg.u.alpha.pingpong, cfg.u.alpha.ease);
			blend = cfg.u.alpha.blend;
		}
		break;
	}
}

TileEffectData::TileEffectData(const TileEffectData& o)
	: efxtype(o.efxtype), efxidx(o.efxidx), grid(NULL)
	, alpha(o.alpha), blend(o.blend)
	, ownGrid(false), shared(false), wavy(o.wavy)
{
}

void TileEffectData::deleteGrid()
{
	if(ownGrid)
	{
		ownGrid = false;
		delete grid;
	}
}

TileEffectData::~TileEffectData()
{
	deleteGrid();
}

DynamicRenderGrid *TileEffectData::_ensureGrid(size_t w, size_t h, const TileData *t)
{
	DynamicRenderGrid *g = grid;
	if(ownGrid)
	{
		assert(g);
		return g;
	}

	if(t && t->rep)
	{
		assert(!shared); // a shared instance MUST have its own grid and MUST NOT refer to the grid of any tile
		deleteGrid();
		g = &t->rep->grid;
	}

	if(!g)
	{
		g = new DynamicRenderGrid;
		ownGrid = true;
	}
	grid = g;
	TexCoordBox tc;
	if(t)
		tc = t->getTexcoords();
	else
		tc.setStandard();
	g->init(w, h, tc);
	if(t && t->rep)
		t->rep->refresh(*t);
	return g;
}

void TileEffectData::Wavy::update(float dt)
{
	if (touching)
	{
		touching = false;
		float ramp = touchVel.getLength2D()/800.0f;
		if (ramp < 0)	ramp = 0;
		if (ramp > 1)	ramp = 1;

		magnitude = 100 * ramp + 16;

		if (touchVel.x < 0)
			magnitude = -magnitude;

		angleOffset = (hitPerc-0.5f)*PI;

		wavySave = wavy;
		lerpIn = 0;
		waving = true;
	}

	if (waving)
	{
		const float spd = PI*1.1f;
		const float magRedSpd = 48;
		const float lerpSpd = 5.0;
		const float wavySzInv = 1.0f / float(wavy.size());
		for (size_t i = 0; i < wavy.size(); i++)
		{
			const float m = float(i)*wavySzInv;
			float weight = m;
			if (flip)
				weight = 1.0f-weight;
			if (weight < 0.125f)
				weight *= 0.5f;
			float val = sinf(angleOffset + m*PI)*(magnitude*effectMult)*weight;
			if (!wavySave.empty())
				val = val * lerpIn + (wavySave[i] * (1.0f-lerpIn));
			wavy[i] = val;
		}

		if (lerpIn < 1)
		{
			lerpIn += dt*lerpSpd;
			if (lerpIn > 1)
				lerpIn = 1;
		}
		angleOffset += dt*spd;
		if (magnitude > 0)
		{
			magnitude -= magRedSpd*dt;
			if (magnitude < 0)
				stop();
		}
		else
		{
			magnitude += magRedSpd*dt;
			if (magnitude > 0)
				stop();
		}
	}
}

void TileEffectData::Wavy::stop()
{
	magnitude = 0;
	waving = false;
}

void TileEffectData::update(float dt, const TileData *t)
{
	switch(efxtype)
	{
		case EFX_WAVY:
			if(!(wavy.waving || wavy.touching))
				break;
			wavy.update(dt);
			if(const size_t N = wavy.wavy.size())
				grid->setFromWavy(&wavy.wavy[0], N, t->et->w);
			// fall through

		case EFX_SEGS:
			grid->update(dt);
		break;

		case EFX_ALPHA:
			alpha.update(dt);
		break;
	}
}

void TileEffectData::doInteraction(const TileData& t, const Vector& pos, const Vector& vel, float mult, float touchWidth)
{
	assert(efxtype == EFX_WAVY);

	const Vector tp(t.x, t.y);

	if (pos.x > tp.x-touchWidth && pos.x < tp.x+touchWidth)
	{
		float h = t.et->h*t.scaley;
		float h2 = h * 0.5f;
		if (pos.y < tp.y+h2 && pos.y > tp.y-h2)
		{
			wavy.touching = true;
			wavy.waving = true;
			float hitPerc = tp.y - h2 - pos.y;
			hitPerc /= h;
			hitPerc = (1.0f-hitPerc)-1.0f;
			wavy.hitPerc = hitPerc;
			wavy.touchVel = vel;
			wavy.effectMult = mult;
		}
	}
}

TileEffectStorage::TileEffectStorage()
{
}

TileEffectStorage::~TileEffectStorage()
{
	clear();
}

void TileEffectStorage::assignEffect(TileData& t, int index) const
{
	dropEffect(t);

	if(index < 0)
		return;

	size_t idx = size_t(index);
	if(idx >= configs.size())
		return;


	bool needinstance = false;
	if(idx < configs.size())
	{
		needinstance = configs[idx].needsOwnInstanceForTile(t);
	}

	if(needinstance)
	{
		if(configs[idx].type == EFX_NONE)
			return;

		t.eff = new TileEffectData(configs[idx], &t);
		t.flags |= TILEFLAG_OWN_EFFDATA;
	}
	else if(idx < prepared.size() && prepared[idx])
	{
		t.eff = prepared[idx];
	}
}

void TileEffectStorage::update(float dt)
{
	for(size_t i = 0; i < prepared.size(); ++i)
		if(TileEffectData *eff = prepared[i])
			eff->update(dt, NULL);
}

void TileEffectStorage::clear()
{
	clearPrepared();
	configs.clear();
}

void TileEffectStorage::clearPrepared()
{
	for(size_t i = 0; i < prepared.size(); ++i)
		delete prepared[i];
	prepared.clear();
}


void TileEffectStorage::finalize()
{
	clearPrepared();
	prepared.resize(configs.size(), (TileEffectData*)NULL);

	for(size_t i = 0; i < configs.size(); ++i)
	{
		TileEffectConfig& c = configs[i];

		c.index = unsigned(i); // just in case

		// segs and alpha are independent of the tile they are applied to,
		// so we can create shared instances of the effect.
		if(c.type == EFX_SEGS || c.type == EFX_ALPHA)
		{
			prepared[i] = new TileEffectData(c, NULL);
			prepared[i]->shared = true;
		}
	}
}

bool TileData::isCoordinateInside(float cx, float cy, float minsize) const
{

	float hw = fabsf(et->w * scalex)*0.5f;
	float hh = fabsf(et->h * scaley)*0.5f;
	if (hw < minsize)
		hw = minsize;
	if (hh < minsize)
		hh = minsize;

	return cx >= x - hw && cx <= x + hw
		&& cy >= y - hh && cy <= y + hh;
}

TileRepeatData::TileRepeatData()
	: texscaleX(1), texscaleY(1)
	, texOffX(0), texOffY(0)
{
}

TileRepeatData::TileRepeatData(const TileRepeatData& o)
	: texscaleX(o.texscaleX), texscaleY(o.texscaleY)
	, texOffX(o.texOffX), texOffY(o.texOffY)
{
}

TexCoordBox TileRepeatData::calcTexCoords(const TileData& t) const
{
	const ElementTemplate& et = *t.et;

	float tw, th;
	if(et.tex)
	{
		tw = et.tex->width;
		th = et.tex->height;
	}
	else
	{
		tw = et.w;
		th = et.h;
	}

	TexCoordBox tc;
	tc.u1 = texOffX;
	tc.v1 = texOffY;
	tc.u2 = (et.w*t.scalex*texscaleX)/tw + texOffX;
	tc.v2 = (et.h*t.scaley*texscaleY)/th + texOffY;

	// HACK: partially repeated textures have a weird Y axis. assuming a repeat factor of 0.4,
	// instead of texcoords from 0 -> 0.4 everything is biased towards the opposite end, ie. 0.6 -> 1.
	// This is especially true for partial repeats, we always need to bias towards the other end.
	// And NOTE: without this, maps may look deceivingly correct, but they really are not.
	tc.v2 = 1 - tc.v2;
	tc.v1 = 1 - tc.v1;
	std::swap(tc.v1, tc.v2);

	return tc;
}

void TileRepeatData::refresh(const TileData& t)
{
	TexCoordBox tc = calcTexCoords(t);

	/*if(t.eff)
		if(const DynamicRenderGrid *g = t.eff->grid)
		{
			grid.init(g->width(), g->height(), grid.getTexCoords());
			grid.gridType = g->gridType;
		}*/

	if(grid.empty())
		grid.init(2, 2, tc);
	else
	{
		grid.setTexCoords(tc);
		grid.reset();
		grid.updateVBO();
	}
}

TileRepeatData* TileData::setRepeatOn(float texscalex, float texscaley, float offx, float offy)
{
	flags |= TILEFLAG_REPEAT;
	if(!rep)
		rep = new TileRepeatData;
	rep->texscaleX = texscalex;
	rep->texscaleY = texscaley;
	rep->texOffX = offx;
	rep->texOffY = offy;
	rep->refresh(*this);

	// link eff->grid to rep->grid. create own instance if necessary.
	/*if(eff)
	{
		const unsigned char gridtype = eff->grid ? eff->grid->gridType : GRID_UNDEFINED;
		if(flags & TILEFLAG_OWN_EFFDATA)
		{
			assert(!eff->shared);
			eff->deleteGrid();
		}
		else
		{
			eff = new TileEffectData(*eff);
			flags |= TILEFLAG_OWN_EFFDATA;
		}
		assert(!eff->ownGrid);
		eff->grid = &rep->grid;
		eff->grid->gridType = gridtype;
	}*/

	return rep;
}

void TileData::setRepeatOff()
{
	flags &= ~TILEFLAG_REPEAT;
	// don't delete this->rep; if we're in editor mode we don't want to lose the repeat data just yet
	// also, a TileEffectData may point to rep->grid
}

void TileData::refreshRepeat()
{
	if(rep)
	{
		rep->refresh(*this);
	}
}

bool TileData::hasStandardTexcoords() const
{
	// repeat applies per-tile texcoords, so if that's set it's non-standard
	return !rep && et->tc.isStandard();
}

const TexCoordBox& TileData::getTexcoords() const
{
	return !(flags & TILEFLAG_REPEAT)
		? et->tc
		: rep->grid.getTexCoords();
}

const RenderGrid *TileData::getGrid() const
{
	if(eff && eff->grid)
		return eff->grid; // this points to rep.grid if eff is present and repeat is on

	if(flags & TILEFLAG_REPEAT)
		return &rep->getGrid();

	return et->grid;
}

bool TileEffectConfig::needsOwnInstanceForTile(const TileData& t) const
{
	const bool rep = !!(t.flags & TILEFLAG_REPEAT);

	switch(type)
	{
		case EFX_NONE:
		case EFX_ALPHA:
			return false;

		case EFX_WAVY:
			return true;

		case EFX_SEGS:
			return rep || !t.hasStandardTexcoords();
	}

	assert(false);
	return true; // uhhhh
}
