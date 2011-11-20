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
#pragma once

#include "../BBGE/Quad.h"

#include "Game.h"

class GemMover;

class GridRender : public RenderObject
{
public:
	GridRender(ObsType obsType);
protected:
	ObsType obsType;
	void onUpdate(float dt);
	void onRender();
};

class MiniMapRender : public RenderObject
{
public:
	MiniMapRender();
	void destroy();

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
	void onUpdate(float dt);
	void onRender();

	InterpolatedVector lerp;
};

class WorldMapRender : public RenderObject, public ActionMapper
{
public:
	WorldMapRender();
	void destroy();
	void toggle(bool on);
	bool isOn();
	Vector getAvatarWorldMapPosition();
	Vector getWorldToTile(WorldMapTile *tile, Vector position, bool fromCenter, bool tilePos);
	void setProperTileColor(WorldMapTile *tile);
	void action(int id, int state);
	GemMover* addGem(GemData *gemData);
	void bindInput();
	void createGemHint(const std::string &gfx);
	void addAllGems();
	void fixGems();
	void removeGem(GemMover *gemMover);
	void onToggleHelpScreen();
	bool isCursorOffHud();
protected:
	Quad *addHintQuad1, *addHintQuad2;
	AquariaMenuItem *helpButton;
	float doubleClickTimer;
	Vector restoreVel;
	float inputDelay;
	BitmapText *areaLabel, *areaLabel2, *areaLabel3;
	WorldMapTile *originalActiveTile;
	void setVis(WorldMapTile *tile);
	void clearVis(WorldMapTile *tile);
	bool on;
	void onUpdate(float dt);
	void onRender();
	Quad *bg;
	unsigned char *savedTexData;
	bool mb;
	Vector lastMousePosition; // See FIXME in WorldMapRender.cpp  --achurch
	void updateEditor();
};

class PathRender : public RenderObject
{
public:
	PathRender();
protected:
	void onRender();
};

class CurrentRender : public RenderObject
{
public:
	CurrentRender();
protected:
	float rippleDelay;
	void onUpdate(float dt);
	void onRender();
};

class SteamRender : public RenderObject
{
public:
	SteamRender();
protected:
	float rippleDelay;
	void onUpdate(float dt);
	void onRender();
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
	void onRender();
	std::vector<SongLinePoint> pts;
};

