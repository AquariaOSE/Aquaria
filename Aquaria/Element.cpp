/*
Copyright (C) 2007, 2010 - Bit-Blot

This file is part of Aquaria.

Aquaria is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "Element.h"
#include "Game.h"
#include "RenderGrid.h"

ElementEffectData::ElementEffectData()
	: elementEffectType(EFX_NONE)
	, wavyAngleOffset(0)
	, wavyMagnitude(0)
	, wavyLerpIn(0)
	, wavyMin(0)
	, wavyMax(0)
	, hitPerc(0)
	, effectMult(0)
	, wavyWaving(false)
	, wavyFlip(false)
	, touching(false)
	, elementEffectIndex(-1)
{
}

Element::Element() : Quad()
{
	elementFlag = EF_NONE;
	bgLayer = 0;
	tag = 0;
	templateIdx = -1;
	eff = NULL;
}

void Element::ensureEffectData()
{
	if (!eff)
		eff = new ElementEffectData;
}

void Element::freeEffectData()
{
	if (eff)
	{
		delete eff;
		eff = NULL;
	}
}

void Element::doInteraction(Entity *ent, float mult, float touchWidth)
{
	ElementEffectData *eff = this->eff;
	Vector pos = position;
	pos.y -= (height*scale.y)/2;

	float hitPerc=0;
	Vector p = ent->position;
	if (p.x > position.x-touchWidth && p.x < position.x+touchWidth)
	{
		float h2 = (height*scale.y)/2.0f;
		if (p.y < position.y+h2 && p.y > position.y-h2)
		{
			eff->touching = true;
			eff->wavyWaving = true;
			hitPerc = pos.y - p.y;
			hitPerc /= float(height*scale.y);
			hitPerc = (1.0f-hitPerc)-1.0f;
			eff->hitPerc = hitPerc;
			eff->touchVel = ent->vel;
			eff->effectMult = mult;
		}
	}
}

void Element::updateEffects(float dt)
{
	switch (eff->elementEffectType)
	{
	case EFX_ALPHA:
		alpha.update(dt);
		break;
	case EFX_WAVY:
		//debugLog("EXF_WAVY update");
		/// check player position
	{
		// if a big wavy doesn't work, this is probably why
		{
			ElementEffectData *eff = this->eff;

			if (eff->touching)
			{
				eff->touching = false;
				float ramp = eff->touchVel.getLength2D()/800.0f;
				if (ramp < 0)	ramp = 0;
				if (ramp > 1)	ramp = 1;

				eff->wavyMagnitude = 100 * ramp + 16;

				if (eff->touchVel.x < 0)
					eff->wavyMagnitude = -eff->wavyMagnitude;

				eff->wavyAngleOffset = (eff->hitPerc-0.5f)*PI;

				eff->wavySave = eff->wavy;
				eff->wavyLerpIn = 0;
			}

			if (eff->wavyWaving)
			{
				float spd = PI*1.1f;
				float magRedSpd = 48;
				float lerpSpd = 5.0;
				float wavySz = float(eff->wavy.size());
				for (size_t i = 0; i < eff->wavy.size(); i++)
				{
					float weight = float(i)/wavySz;
					if (eff->wavyFlip)
						weight = 1.0f-weight;
					if (weight < 0.125f)
						weight *= 0.5f;
					eff->wavy[i].x = sinf(eff->wavyAngleOffset + (float(i)/wavySz)*PI)*float(eff->wavyMagnitude*eff->effectMult)*weight;
					if (!eff->wavySave.empty())
					{
						if (eff->wavyLerpIn < 1)
							eff->wavy[i].x = eff->wavy[i].x*eff->wavyLerpIn + (eff->wavySave[i].x*(1.0f-eff->wavyLerpIn));
					}
				}

				if (eff->wavyLerpIn < 1)
				{
					eff->wavyLerpIn += dt*lerpSpd;
					if (eff->wavyLerpIn > 1)
						eff->wavyLerpIn = 1;
				}
				eff->wavyAngleOffset += dt*spd;
				if (eff->wavyMagnitude > 0)
				{
					eff->wavyMagnitude -= magRedSpd*dt;
					if (eff->wavyMagnitude < 0)
						eff->wavyMagnitude = 0;
				}
				else
				{
					eff->wavyMagnitude += magRedSpd*dt;
					if (eff->wavyMagnitude > 0)
						eff->wavyMagnitude = 0;
				}


				setGridFromWavy();

			}
			else
			{
				//std::cout << "not waving";
				setGridFromWavy();
			}
		}
	}
	break;
	}
}

void Element::update(float dt)
{
	if (!core->particlesPaused)
	{
		updateLife(dt);
		if (eff)
			updateEffects(dt);
		if(grid)
			grid->update(dt);
	}
}

Element::~Element()
{
	freeEffectData();
}

void Element::destroy()
{
	Quad::destroy();
}

int Element::getElementEffectIndex()
{
	return eff ? eff->elementEffectIndex : -1;
}

void Element::setGridFromWavy()
{
	if(grid && eff->wavy.size())
		grid->setFromWavy(&eff->wavy[0], eff->wavy.size(), width);
}

void Element::setElementEffectByIndex(int eidx)
{
	deleteGrid();

	setBlendType(BLEND_DEFAULT);
	alpha.stop();
	alpha = 1;

	ElementEffect e = dsq->getElementEffectByIndex(eidx);
	if(e.type != EFX_NONE)
		ensureEffectData();

	switch(e.type)
	{
	case EFX_SEGS:
	{
		setSegs(e.segsx, e.segsy, e.segs_dgox, e.segs_dgoy, e.segs_dgmx, e.segs_dgmy, e.segs_dgtm, e.segs_dgo);
	}
	break;
	case EFX_ALPHA:
	{
		setBlendType(e.blendType);
		alpha = e.alpha;
	}
	break;
	case EFX_WAVY:
	{

		eff->wavy.resize(e.segsy);
		float bity = float(getHeight())/float(e.segsy);
		for (size_t i = 0; i < eff->wavy.size(); i++)
		{
			eff->wavy[i] = Vector(0, -(i*bity));
		}
		eff->wavyFlip = e.wavy_flip;
		eff->wavyMin = bity;
		eff->wavyMax = bity*1.2f;

		if(RenderGrid *g = createGrid(2, e.segsy))
		{
			g->gridType = GRID_UNDEFINED; // by default it's GRID_WAVY, but that would reset during update
			setGridFromWavy();
		}
	}
	break;
	default:
		freeEffectData();
	break;
	}

	if (eff)
	{
		eff->elementEffectIndex = eidx;
		eff->elementEffectType = e.type;
	}
}

// shamelessly ripped from paint.net default palette
static const Vector s_tagColors[] =
{
	/* 0 */ Vector(0.5f, 0.5f, 0.5f),
	/* 1 */ Vector(1,0,0),
	/* 2 */ Vector(1, 0.415686f, 0),
	/* 3 */ Vector(1,0.847059f, 0),
	/* 4 */ Vector(0.298039f,1,0),
	/* 5 */ Vector(0,1,1),
	/* 6 */ Vector(0,0.580392,1),
	/* 7 */ Vector(0,0.149020f,1),
	/* 8 */ Vector(0.282353f,0,1),
	/* 9 */ Vector(0.698039f,0,1),

	/* 10 */ Vector(1,0,1), // anything outside of the pretty range
};

static inline const Vector& getTagColor(int tag)
{
	const unsigned idx = std::min<unsigned>(unsigned(tag), Countof(s_tagColors)-1);
	return s_tagColors[idx];

}

void Element::render(const RenderState& rs) const
{
	// FIXME: this should be part of the layer render, not here
	// (not relevant until Elements are batched per-layer)
	if (game->isSceneEditorActive() && this->bgLayer == game->sceneEditor.bgLayer
		&& game->sceneEditor.editType == ET_ELEMENTS)
	{
		Vector tagColor = getTagColor(tag);
		bool hl = false;
		if (!game->sceneEditor.selectedElements.empty())
		{
			for (size_t i = 0; i < game->sceneEditor.selectedElements.size(); i++)
			{
				if (this == game->sceneEditor.selectedElements[i])
				{
					hl = true;
					break;
				}
			}
		}
		else
		{
			hl = game->sceneEditor.editingElement == this;
		}

		if(hl)
			tagColor += Vector(0.5f, 0.5f, 0.5f);

		RenderState rs2(rs);
		rs2.forceRenderBorder = true;
		rs2.forceRenderCenter = true;
		rs2.renderBorderColor = tagColor;
		Quad::render(rs2);
	}
	else // render normally
		Quad::render(rs);
}

void Element::fillGrid()
{
	if (life == 1 && !_hidden)
	{
		if (elementFlag == EF_SOLID)
		{
			game->fillGridFromQuad(this, OT_INVISIBLE, true);
		}
		else if (elementFlag == EF_HURT)
		{
			game->fillGridFromQuad(this, OT_HURT, false);
		}
		else if (elementFlag == EF_SOLID2)
		{
			game->fillGridFromQuad(this, OT_INVISIBLE, false);
		}
		else if (elementFlag == EF_SOLID3)
		{
			game->fillGridFromQuad(this, OT_INVISIBLEIN, false);
		}
	}
}

void Element::setTag(int tag)
{
	this->tag = tag;
}
