#include "Tile.h"
#include "RenderGrid.h"
#include "Tileset.h"
#include "Base.h"
#include <algorithm>

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

static void dropAttachments(TileData& t)
{
	if(t.flags & TILEFLAG_OWN_EFFDATA)
	{
		delete t.eff;
		t.flags &= ~TILEFLAG_OWN_EFFDATA;
	}
	t.eff = NULL;
}

void TileStorage::deleteSome(const size_t* indices, size_t n)
{
	std::vector<TileData> tmp;
	tmp.swap(tiles);
	tiles.reserve(tmp.size() - n);

	for(size_t i = 0; i < tmp.size(); ++i)
	{
		for(size_t k = 0; k < n; ++i) // not particularly efficient, could be much better by sorting first but eh
			if(indices[k] == i)
			{
				dropAttachments(tmp[i]);
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
		dropAttachments(tiles[i]);
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
		const TileData& t = tiles[i];
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

TileEffectData::TileEffectData(const TileEffectConfig& cfg)
	: efxtype(cfg.type), efxidx(cfg.index)
	, grid(NULL), alpha(1), blend(BLEND_DEFAULT)
{
	switch(cfg.type)
	{
		case EFX_NONE:
			assert(false);
			break;

		case EFX_WAVY:
		{
			float bity = 20; // FIXME
			wavy.wavy.resize(cfg.u.wavy.segsy, 0.0f);
			wavy.flip = cfg.u.wavy.flip;
			wavy.min = bity;
			wavy.max = bity*1.2f;

			RenderGrid *g = new RenderGrid(2, cfg.u.wavy.segsy);
			grid = g;
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
			RenderGrid *g = new RenderGrid(cfg.u.segs.x, cfg.u.segs.y);
			grid = g;
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

TileEffectData::~TileEffectData()
{
	delete grid;
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
	}

	if (waving)
	{
		// TODO: set waving=false if magnitude==0 ?
		float spd = PI*1.1f;
		float magRedSpd = 48;
		float lerpSpd = 5.0;
		float wavySz = float(wavy.size());
		for (size_t i = 0; i < wavy.size(); i++)
		{
			float weight = float(i)/wavySz;
			if (flip)
				weight = 1.0f-weight;
			if (weight < 0.125f)
				weight *= 0.5f;
			wavy[i] = sinf(angleOffset + (float(i)/wavySz)*PI)*(magnitude*effectMult)*weight;
			if (!wavySave.empty())
			{
				if (lerpIn < 1)
					wavy[i] = wavy[i] * lerpIn + (wavySave[i] * (1.0f-lerpIn));
			}
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
				magnitude = 0;
		}
		else
		{
			magnitude += magRedSpd*dt;
			if (magnitude > 0)
				magnitude = 0;
		}
	}
}

void TileEffectData::update(float dt, const TileData *t)
{
	switch(efxtype)
	{
		case EFX_WAVY:
			wavy.update(dt);
			if(const size_t N = wavy.wavy.size())
				grid->setFromWavy(&wavy.wavy[0], N, t->et->w);
		break;

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
	dropAttachments(t);

	if(index < 0)
		return;

	size_t idx = size_t(index);

	if(idx < prepared.size() && prepared[idx])
	{
		t.eff = prepared[idx];
	}
	else if(idx < configs.size())
	{
		if(configs[idx].type == EFX_NONE)
			return;

		t.eff = new TileEffectData(configs[idx]);
		t.flags |= TILEFLAG_OWN_EFFDATA;
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
			prepared[i] = new TileEffectData(c);
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
