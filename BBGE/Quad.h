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

class OutlineRect : public RenderObject
{
public:
	OutlineRect();
	void setWidthHeight(int w, int h);
	void setLineSize(int ls);

	bool renderCenter;
protected:

	int w, h, w2, h2;
	int lineSize;

	void onRender() const OVERRIDE;
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
	unsigned int getWidth() const {return static_cast<unsigned int>(width);}
	unsigned int getHeight() const {return static_cast<unsigned int>(height);}

	void setSegs(int x, int y, float dgox, float dgoy, float dgmx, float dgmy, float dgtm, bool dgo);
	void setDrawGridAlpha(size_t x, size_t y, float alpha);
	void repeatTextureToFill(bool on);
	void refreshRepeatTextureToFill();
	bool isRepeatingTextureToFill() const { return repeatingTextureToFill; }
	void setGridPoints(bool vert, const std::vector<Vector> &points);
	virtual void createStrip(bool stripVert, int num);
	float getStripSegmentSize() const;
	void resetStrip();
	Vector ** getDrawGrid() { return drawGrid; }

	void reloadDevice();

	void deleteGrid();


	InterpolatedVector upperLeftTextureCoordinates, lowerRightTextureCoordinates;



	enum GridType
	{
		GRID_WAVY	= 0,
		GRID_SET	= 1
	};
	unsigned char gridType;  // unsigned char to save space

	char autoWidth, autoHeight;  // char to save space

	bool renderQuad, renderCenter;
	mutable bool renderBorder; // TODO: should be part of render state
	bool stripVert;
	std::vector<Vector>strip;
	Vector texOff;

	float borderAlpha;
	Vector repeatToFillScale;

protected:
	bool repeatingTextureToFill;
	float gridTimer;
	size_t xDivs, yDivs;
	Vector ** drawGrid;

	void resetGrid();
	void updateGrid(float dt);
	void renderGrid() const;


	float drawGridOffsetX;
	float drawGridOffsetY;
	float drawGridModX;
	float drawGridModY;
	float drawGridTimeMultiplier;
	bool drawGridOut;

	static Vector renderBorderColor;

	void onSetTexture();
	void onRender() const OVERRIDE;
	void onUpdate(float dt);
private:
	bool doUpdateGrid;
	void initQuad();
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
	virtual void renderCollision() const OVERRIDE;

	float collideRadius;
};

#define QUAD(x) Quad *x = new Quad; addRenderObject(x);

#endif

