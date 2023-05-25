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
#include "../BBGE/Quad.h"
#include "ActionMapper.h"

class GemMover;
struct MinimapIcon;
struct WorldMapTile;
struct GemData;
class AquariaMenuItem;
class BitmapText;

class GridRender : public RenderObject
{
public:
	GridRender(ObsType obsType);
protected:
	ObsType obsType;
	void onUpdate(float dt) OVERRIDE;
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
	void action(int id, int state, int source, InputDevice device);
	GemMover* addGem(GemData *gemData);
	void bindInput();
	void createGemHint(const std::string &gfx);
	void addAllGems();
	void fixGems();
	void removeGem(GemMover *gemMover);
	void onToggleHelpScreen();
	bool isCursorOffHud();

	static void setRevealMethod(WorldMapRevealMethod m);

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
	void onRender(const RenderState& rs) const OVERRIDE;
};

class CurrentRender : public RenderObject
{
public:
	CurrentRender();
protected:
	float rippleDelay;
	void onRender(const RenderState& rs) const OVERRIDE;
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
