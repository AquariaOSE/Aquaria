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

class Quad : public RenderObject
{
public:
	Quad(const std::string &tex, const Vector &pos);
	Quad();
	void createGrid(int x, int y);
	void destroy();
	bool isCoordinateInside(Vector coord, int minSize=0) const;
	bool isCoordinateInsideWorld(const Vector &coord, int minSize=0) const;
	bool isCoordinateInsideWorldRect(const Vector &coord, int w, int h) const;

	void flipVertical();
	void flipHorizontal();
	void setWidthHeight(float w, float h=-1);
	void setWidth(float w);
	void setHeight(float h);
	float getWidth() const {return width;}
	float getHeight() const {return height;}

	void setSegs(int x, int y, float dgox, float dgoy, float dgmx, float dgmy, float dgtm, bool dgo);
	void setDrawGridAlpha(size_t x, size_t y, float alpha);
	void repeatTextureToFill(bool on);
	void refreshRepeatTextureToFill();
	bool isRepeatingTextureToFill() const { return repeatTexture; }
	void setStripPoints(bool vert, const Vector *points, size_t n);
	Array2d<Vector>& getDrawGrid() { return drawGrid; }
	const Array2d<Vector>& getDrawGrid() const { return drawGrid; }

	void reloadDevice();

	void deleteGrid();


	InterpolatedVector upperLeftTextureCoordinates, lowerRightTextureCoordinates;

	enum GridDrawOrder
	{
		GRID_DRAW_WORLDMAP = -1, // LRTB order, uses grid.z as alpha
		GRID_DRAW_LRTB = 0, // the default. ignores grid.z
		GRID_DRAW_LRBT = 1, // Y axis inverted

		GRID_DRAW_DEFAULT = GRID_DRAW_LRTB
	};

	enum GridType
	{
		GRID_WAVY	= 0,
		GRID_STRIP	= 1, // quad is in strip mode
		GRID_INTERP = 2, // quad is in grid mode
	};
	unsigned char gridType;  // unsigned char to save space

	char autoWidth, autoHeight;  // char to save space

	bool renderQuad, renderCenter, renderBorder;
	Vector texOff;

	float borderAlpha;
	Vector renderBorderColor;
	Vector repeatToFillScale;

	static void ResetGrid(Vector *dst, size_t w, size_t h);
	static void ResetGridAndAlpha(Vector *dst, size_t w, size_t h, float alpha = 1.0f);


protected:
	float gridTimer;
	Array2d<Vector> drawGrid;

	void resetGrid();
	void updateGrid(float dt);
	void renderGrid(const RenderState& rs) const;
	void renderGrid_LRTB(const RenderState& rs) const;
	void renderGrid_LRBT(const RenderState& rs) const;
	void renderGridWithAlpha(const RenderState& rs) const;

	float drawGridOffsetX;
	float drawGridOffsetY;
	float drawGridModX;
	float drawGridModY;
	float drawGridTimeMultiplier;
	bool drawGridOut;

	void onSetTexture();
	void onRender(const RenderState& rs) const OVERRIDE;
	void onUpdate(float dt);

public:
	GridDrawOrder drawOrder;
private:
	bool doUpdateGrid;
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

