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
#ifndef __quad__
#define __quad__

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

	void onRender();
};

class Quad : public RenderObject
{
public:
	Quad(const std::string &tex, const Vector &pos);
	Quad();	
	void createGrid(int x, int y);
	void destroy();
	bool isCoordinateInside(Vector coord, int minSize=0);
	bool isCoordinateInsideWorld(const Vector &coord, int minSize=0);
	bool isCoordinateInsideWorldRect(const Vector &coord, int w, int h);

	void flipVertical();
	void flipHorizontal();
	void setTextureSmooth(const std::string &texture, float t);
	void spawnChildClone(float t);
	void burn();
	void unburn();
	void setWidthHeight(float w, float h=-1);
	void setWidth(float w);
	void setHeight(float h);
	int getWidth() const {return int(width);}
	int getHeight() const {return int(height);}
	
	void setSegs(int x, int y, float dgox, float dgoy, float dgmx, float dgmy, float dgtm, bool dgo);	
	void setDrawGridAlpha(int x, int y, float alpha);
	void calculateQuadLighting();
	void repeatTextureToFill(bool on);
	void refreshRepeatTextureToFill();
	bool isRepeatingTextureToFill() const { return repeatingTextureToFill; }
	void setGridPoints(bool vert, const std::vector<Vector> &points);
	void setStrip(const std::vector<Vector> &strip);
	virtual void createStrip(bool stripVert, int num);
	float getStripSegmentSize();
	void resetStrip();
	Vector ** getDrawGrid() { return drawGrid; }
	
	void reloadDevice();

	void deleteGrid();


	InterpolatedVector upperLeftTextureCoordinates, lowerRightTextureCoordinates;
	//InterpolatedVector upperLeftColor, upperRightColor, lowerLeftColor, lowerRightColor;
	//InterpolatedVector llalpha, lralpha, ulalpha, uralpha;
	//bool oriented;

	enum GridType
	{
		GRID_WAVY	= 0,
		GRID_SET	= 1
	};
	unsigned char gridType;  // unsigned char to save space

	char autoWidth, autoHeight;  // char to save space
	
	bool renderQuad, renderBorder, renderCenter;
	bool stripVert;
	std::vector<Vector>strip;
	Vector texOff;

	float borderAlpha;
	Vector repeatToFillScale;

protected:
	bool repeatingTextureToFill;
	float gridTimer;
	int xDivs, yDivs;
	Vector ** drawGrid;

	void resetGrid();
	void updateGrid(float dt);
	void renderGrid();
	

	float drawGridOffsetX;
	float drawGridOffsetY;
	float drawGridModX;
	float drawGridModY;
	float drawGridTimeMultiplier;
	bool drawGridOut;
	
	static Vector renderBorderColor;
	
	void onSetTexture();
	void onRender();
	void onUpdate(float dt);
private:
	bool doUpdateGrid;
	void initQuad();
};

class PauseQuad : public Quad
{
public:
	PauseQuad();
	int pauseLevel;
protected:
	
	void onUpdate(float dt);
};

#define QUAD(x) Quad *x = new Quad; addRenderObject(x);

#endif

