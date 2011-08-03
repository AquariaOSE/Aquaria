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

class Avatar;

class ElementActivationRange
{
public:
	ElementActivationRange();
	void isVectorInRange();
	enum Type
	{
		CIRCLE,
		RECT
	};
	float radius;
	Type getType();
protected:
	Type type;
};


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

class Element : public Quad
{
public:
	enum Type
	{
		UNDEFINED,
		BOX
	};

	Element(Type elementType);
	~Element();
	void destroy();
	//void interact(Interaction::Type interactionType, Avatar *avatar);
	bool canSeeAvatar(Avatar *avatar);
	void update(float dt);
	bool isActive();
	Type getElementType();
	//InteractionContainer interactions;
	int templateIdx;
	int bgLayer;
	Element *bgLayerNext;
	float getSortDepth();
	bool dontSave;
	void render();
	//Flags elementFlags;
	ElementFlag elementFlag;
	void fillGrid();
	bool isElementActive() { return elementActive; }
	int getElementEffectIndex();
	void setElementEffectByIndex(int e);
	void setElementActive(bool v) { elementActive = v; }
	float parallax;
	float angleFromGroupCenter, distFromGroupCenter, oldRotation;
protected:
	void setGridFromWavy();
	float wavyAngleOffset, wavyMagnitude, wavyLerpIn;
	bool wavyWaving, wavyFlip;
	void wavyPull(int to, int from, float dt);
	std::vector<Vector> wavy, wavySave;
	float wavyRadius, wavyMin, wavyMax;
	void updateEffects(float dt);
	int elementEffectIndex, elementEffectType;
	bool elementActive;
	ElementActivationRange activationRange;
	Type type;
};

class BoxElement : public Element
{
public:
	BoxElement(int width, int height);
	//bool isOnScreen();
protected:
	int ww,hh;
};

typedef std::vector<Element*> ElementContainer;

#endif

