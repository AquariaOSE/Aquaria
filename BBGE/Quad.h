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
#ifndef BBGE_QUAD_H
#define BBGE_QUAD_H

#include "RenderObject.h"
#include "DataStructures.h"

class OutlineRect : public RenderObject
{
public:
	OutlineRect();
	void setWidthHeight(int w, int h);
	void setLineSize(int ls);

protected:

	int w, h, w2, h2;
	int lineSize;

	void onRender(const RenderState& rs) const OVERRIDE;
};

class RenderGrid;

class Quad : public RenderObject
{
public:
	Quad(const std::string &tex, const Vector &pos);
	Quad();
	virtual ~Quad();
	RenderGrid *createGrid(int x, int y);
	void destroy() OVERRIDE;
	bool isCoordinateInside(Vector coord, int minSize=0) const;
	bool isCoordinateInsideWorld(const Vector &coord, int minSize=0) const;
	bool isCoordinateInsideWorldRect(const Vector &coord, int w, int h) const;

	void flipVertical() OVERRIDE;
	void flipHorizontal() OVERRIDE;
	void setWidthHeight(float w, float h=-1);
	void setWidth(float w);
	void setHeight(float h);
	float getWidth() const {return width;}
	float getHeight() const {return height;}

	RenderGrid *setSegs(int x, int y, float dgox, float dgoy, float dgmx, float dgmy, float dgtm, bool dgo);
	void setDrawGridAlpha(size_t x, size_t y, float alpha);
	void repeatTextureToFill(bool on);
	void refreshRepeatTextureToFill();
	bool isRepeatingTextureToFill() const { return repeatTexture; }
	void setStripPoints(bool vert, const Vector *points, size_t n);
	RenderGrid *getGrid() { return grid; }
	const RenderGrid *getGrid() const { return grid; }

	void reloadDevice() OVERRIDE;

	void deleteGrid();

	Vector upperLeftTextureCoordinates, lowerRightTextureCoordinates;

	// TODO: this should be a bitmask
	char autoWidth, autoHeight;
	bool renderQuad, renderCenter, renderBorder;

	Vector texOff;

	float borderAlpha;
	Vector renderBorderColor;
	Vector repeatToFillScale;

protected:

	RenderGrid *grid;


	void resetGrid();
	void renderGrid(const RenderState& rs) const;

	void onSetTexture() OVERRIDE;
	void onRender(const RenderState& rs) const OVERRIDE;
	void onUpdate(float dt) OVERRIDE;

private:
	void initQuad();
	void _renderBorder(const RenderState& rs, Vector color, float borderalpha) const;
};

class PauseQuad : public Quad
{
public:
	PauseQuad();
	virtual ~PauseQuad();
	int pauseLevel;

	void setPositionSnapTo(InterpolatedVector *positionSnapTo);
protected:
	InterpolatedVector *positionSnapTo;

	void onUpdate(float dt);
};

class CollideQuad : public Quad
{
public:
	CollideQuad();
	virtual ~CollideQuad();
	virtual void renderCollision(const RenderState& rs) const OVERRIDE;

	float collideRadius;
};

#define QUAD(x) Quad *x = new Quad; addRenderObject(x);

#endif

