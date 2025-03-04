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
#ifndef GRIDRENDER_H
#define GRIDRENDER_H

#include "GameEnums.h"
#include "GameStructs.h"
#include "Quad.h"
#include "ActionMapper.h"
#include "VertexBuffer.h"


struct MinimapIcon;
class AquariaMenuItem;
class BitmapText;

class GridRender : public RenderObject
{
public:
	GridRender(ObsType obsType);
	void rebuildBuffers();
	void rebuildBuffers(const std::vector<ObsRow>& rows);
	void rebuildBuffersIfNecessary();
	void rebuildBuffersIfNecessary(const std::vector<ObsRow>& rows);
	void markForRebuild() { markedForRebuild = true; }
	ObsType getObs() const { return obsType; }
protected:
	DynamicGPUBuffer vbo;
	size_t primsToDraw;
	std::vector<size_t> primIndexInLine;
	const ObsType obsType;
	bool markedForRebuild;

	void onRender(const RenderState& rs) const OVERRIDE;
};

class MiniMapRender : public RenderObject
{
public:
	MiniMapRender();
	void destroy() OVERRIDE;

	bool isCursorIn();
	void slide(int slid);
	void toggle(int on);
	float getMiniMapWidth() const;
	float getMiniMapHeight() const;

	bool isRadarHide() { return radarHide; }
protected:
	bool radarHide;
	int toggleOn;

	void clickEffect(int type=0);
	float doubleClickDelay;
	bool isCursorInButtons();
	bool _isCursorIn, lastCursorIn;
	bool mouseDown;
	bool doRender;
	float lightLevel;
	void onUpdate(float dt) OVERRIDE;
	void onRender(const RenderState& rs) const OVERRIDE;
	void renderIcon(const MinimapIcon *ico, const Vector& pos) const;

	InterpolatedVector lerp;

public:
	static bool setWaterBitTex(const std::string& name);
	static bool setTopTex(const std::string& name);
	static bool setBottomTex(const std::string& name);
	static bool setAvatarTex(const std::string& name);
	static bool setHealthBarTex(const std::string& name);
	static bool setMaxHealthMarkerTex(const std::string& name);
};

class PathRender : public RenderObject
{
public:
	PathRender();
protected:
	void onRender(const RenderState& rs) const OVERRIDE;
};

class CurrentRender : public RenderObject
{
public:
	CurrentRender();
protected:
	float rippleDelay;
	void onRender(const RenderState& rs) const OVERRIDE;
	void onUpdate(float dt) OVERRIDE;
	size_t writeVBOData(float *p);
	DynamicGPUBuffer vbo;
	size_t _verticesToRender;
};

class SteamRender : public RenderObject
{
public:
	SteamRender();
protected:
	float rippleDelay;
	void onRender(const RenderState& rs) const OVERRIDE;
};

struct SongLinePoint
{
	Vector color;
	Vector pt;
};

class SongLineRender : public RenderObject
{
public:
	SongLineRender();
	void newPoint(const Vector &pt, const Vector &color);
	void clear();
protected:
	void onRender(const RenderState& rs) const OVERRIDE;
	std::vector<SongLinePoint> pts;
};

#endif
