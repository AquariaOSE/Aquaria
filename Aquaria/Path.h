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

#include "../BBGE/Base.h"
#include "../BBGE/Particles.h"
#include "../BBGE/ScriptObject.h"
#include "ScriptInterface.h"

#undef PATH_MAX  // May be set by a system header.

class PathNode
{
public:
	PathNode() { maxSpeed = -1;}
	Vector position;
	int maxSpeed;
};

enum PathType
{
	PATH_NONE			= 0,
	PATH_CURRENT		= 1,
	PATH_STEAM			= 2,
	PATH_LI				= 3,
	PATH_SAVEPOINT		= 4,
	PATH_WARP			= 5,
	PATH_SPIRITPORTAL	= 6,
	PATH_BGSFXLOOP		= 7,
	PATH_RADARHIDE		= 8,
	PATH_COOK			= 9,
	PATH_WATERBUBBLE	= 10,
	PATH_GEM			= 11,
	PATH_SETING			= 12,
	PATH_SETENT			= 13,
	PATH_ZOOM			= 14,
	PATH_MAX
};

enum LocalWarpType
{
	LOCALWARP_NONE		= 0,
	LOCALWARP_IN		= 1,
	LOCALWARP_OUT		= 2
};

enum PathShape
{
	PATHSHAPE_RECT		= 0,
	PATHSHAPE_CIRCLE	= 1
};

class Path : public ScriptObject
{
public:
	Path();
	~Path();
	void destroy();
	void init();
	void clampPosition(Vector *pos, int rad=0);
	void song(SongType song);
	void songNote(int note);
	void songNoteDone(int note, float len);
	bool hasScript();
	std::string name; // full node string
	std::string label; // first part only (the actual node name)
	std::vector<PathNode>nodes;
	void removeNode(int idx);
	void addNode(int idx);
	void update(float dt);
	void setActive(bool v);
	bool action(int id, int state);

	void setEffectOn(bool on) { effectOn = on; }

	PathNode *getPathNode(int idx);
	bool isCoordinateInside(const Vector &pos, int rad=0);

	void reverseNodes();

	int getLeft();
	int getRight();
	int getUp();
	int getDown();

	Vector getBackPos(const Vector &position);
	Vector getEnterPosition(int outAmount=1);
	Vector getEnterNormal();

	void activate(Entity *e=0);
	void refreshScript();
	Script *script;
	bool updateFunction;
	bool activateFunction;
	bool cursorActivation;
	int replayVox;

	std::string warpMap, warpNode, vox, spawnEnemyName, content;
	float amount, time;
	Entity *spawnedEntity;
	int spawnEnemyNumber, spawnEnemyDistance;
	char warpType;
	RectShape rect;
	bool active, naijaIn;

	float animOffset;

	PathType pathType;
	Path *nextOfType;

	int toFlip;

	LocalWarpType localWarpType;

	bool naijaHome;
	bool catchActions;
	bool songFunc, songNoteFunc, songNoteDoneFunc;
	bool neverSpawned;
	ParticleEffect *emitter;

	float currentMod;
	bool addEmitter;

	std::string gem;

	bool effectOn;

	PathShape pathShape;


	void parseWarpNodeData(const std::string &dataString);
};

