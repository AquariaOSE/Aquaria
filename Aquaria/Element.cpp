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
	elementActive = true;
	bgLayer = 0;
	templateIdx = -1;
	eff = NULL;

	setStatic(true);
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
	BBGE_PROF(Element_update);
	if (!core->particlesPaused)
	{
		updateLife(dt);
		if (eff)
			updateEffects(dt);
		if (drawGrid)
			updateGrid(dt);
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
	if (drawGrid)
	{

		const float w = float(getWidth());
		for (size_t x = 0; x < xDivs-1; x++)
		{
			for (size_t y = 0; y < yDivs; y++)
			{
				const int wavy_y = (yDivs - y)-1;
				const float tmp = eff->wavy[wavy_y].x / w;
				if (wavy_y < 0 || (size_t) wavy_y < eff->wavy.size())
				{

					drawGrid[x][y].x = tmp - 0.5f;
					drawGrid[x+1][y].x = tmp + 0.5f;
				}
			}
		}
	}
	else
	{
		//std::cout << "no drawgrid...\n";
	}
}

void Element::setElementEffectByIndex(int eidx)
{
	deleteGrid();

	setBlendType(RenderObject::BLEND_DEFAULT);
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
		setStatic(false);
	}
	break;
	case EFX_ALPHA:
	{
		setBlendType(e.blendType);
		alpha = e.alpha;
		setStatic(false);
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

		createGrid(2, e.segsy);
		setGridFromWavy();
		setStatic(false);
	}
	break;
	default:
		freeEffectData();
		setStatic(true);
	break;
	}

	if (eff)
	{
		eff->elementEffectIndex = eidx;
		eff->elementEffectType = e.type;
	}
}

void Element::render()
{
	if (!elementActive) return;
#ifdef AQUARIA_BUILD_SCENEEDITOR
	if (dsq->game->isSceneEditorActive() && this->bgLayer == dsq->game->sceneEditor.bgLayer
		&& dsq->game->sceneEditor.editType == ET_ELEMENTS)
	{
		renderBorderColor = Vector(0.5,0.5,0.5);
		if (!dsq->game->sceneEditor.selectedElements.empty())
		{
			for (size_t i = 0; i < dsq->game->sceneEditor.selectedElements.size(); i++)
			{
				if (this == dsq->game->sceneEditor.selectedElements[i])
					renderBorderColor = Vector(1,1,1);
			}
		}
		else
		{
			if (dsq->game->sceneEditor.editingElement == this)
				renderBorderColor = Vector(1,1,1);
		}
		renderBorder = true;

	}
#endif

	Quad::render();

	renderBorder = false;
}

void Element::fillGrid()
{
	if (life == 1 && elementActive)
	{
		if (elementFlag == EF_SOLID)
		{
			dsq->game->fillGridFromQuad(this, OT_INVISIBLE, true);
		}
		else if (elementFlag == EF_HURT)
		{
			dsq->game->fillGridFromQuad(this, OT_HURT, false);
		}
		else if (elementFlag == EF_SOLID2)
		{
			dsq->game->fillGridFromQuad(this, OT_INVISIBLE, false);
		}
		else if (elementFlag == EF_SOLID3)
		{
			dsq->game->fillGridFromQuad(this, OT_INVISIBLEIN, false);
		}
	}
}

