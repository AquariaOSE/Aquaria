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
#ifndef __element__
#define __element__

#include "../BBGE/Quad.h"

class Entity;


enum ElementFlag
{
	EF_NONE			= 0,
	EF_SOLID		= 1,
	EF_MOVABLE		= 2,
	EF_HURT			= 3,
	EF_SOLID2		= 4,
	EF_SOLID3		= 5,
	EF_MAX			= 6
	/*
	EF_GLASS		= 0x00000020,
	EF_FORCEBREAK	= 0x00000100,
	EF_HURT			= 0x00000200
	*/
};

struct ElementEffectData
{
	ElementEffectData();

	int elementEffectType;
	float wavyAngleOffset, wavyMagnitude, wavyLerpIn;
	float wavyMin, wavyMax;
	float hitPerc, effectMult;
	bool wavyWaving, wavyFlip, touching;
	Vector touchVel;
	std::vector<Vector> wavy, wavySave;
	int elementEffectIndex; // used by editor only
};

class Element : public Quad
{
public:
	Element();
	~Element();
	void destroy() OVERRIDE;
	void update(float dt) OVERRIDE;
	size_t templateIdx;
	int bgLayer;
	int tag;
	Element *bgLayerNext;
	void render(const RenderState& rs) const OVERRIDE;
	ElementFlag elementFlag;
	void fillGrid();
	bool isElementActive() const { return !_hidden; }
	int getElementEffectIndex();
	void setElementEffectByIndex(int e);
	void setElementActive(bool v) { _hidden = !v; }
	void doInteraction(Entity *ent, float mult, float touchWidth);
	void setTag(int tag);
protected:
	void ensureEffectData();
	void freeEffectData();
	void setGridFromWavy();
	ElementEffectData *eff;

	void updateEffects(float dt);};

typedef std::vector<Element*> ElementContainer;

#endif

