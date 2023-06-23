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
#ifndef AQ_PATH_H
#define AQ_PATH_H

#include "../BBGE/Base.h"
#include "../BBGE/Particles.h"
#include "../BBGE/ScriptObject.h"
#include "Rect.h"
#include "Scriptable.h"

#undef PATH_MAX  // May be set by a system header.

struct MinimapIcon;
class Entity;

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

class Path : public ScriptObject, public Scriptable
{
public:
	Path();
	~Path();
	void destroy();
	void init();
	void clampPosition(Vector *pos, float rad=0);
	void song(SongType song);
	void songNote(int note);
	void songNoteDone(int note, float len);
	bool hasScript() const;
	std::string name; // full node string
	std::string label; // first part only (the actual node name)
	std::vector<PathNode>nodes;
	void removeNode(size_t idx);
	void addNode(size_t idx);
	void update(float dt);
	void setActive(bool v);
	bool action(int id, int state, int source, InputDevice device);
	void setEmitter(const std::string& name);

	PathNode *getPathNode(size_t idx);
	bool isCoordinateInside(const Vector &pos, float rad=0) const;

	void reverseNodes();

	int getLeft();
	int getRight();
	int getUp();
	int getDown();

	Vector getBackPos(const Vector &position);
	Vector getEnterPosition(int outAmount=1);
	Vector getEnterNormal();

	void activate(Entity *e, int source);
	void refreshScript();
	MinimapIcon *ensureMinimapIcon();

	bool updateFunction;
	bool activateFunction;
	bool cursorActivation;
	int replayVox;
	MinimapIcon *minimapIcon;

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

	bool catchActions;
	bool songFunc, songNoteFunc, songNoteDoneFunc;
	bool neverSpawned;
	ParticleEffect *emitter;

	float currentMod;
	bool addEmitter;

	std::string gem;

	bool spiritFreeze;
	bool pauseFreeze;

	PathShape pathShape;
	float activationRange;


	void parseWarpNodeData(const std::string &dataString);

	int callVariadic(const char *func, lua_State *L, int nparams);
	int messageVariadic(lua_State *L, int nparams);
	int activateVariadic(lua_State *L, int nparams);
	void luaDebugMsg(const std::string &func, const std::string &msg);
};

#endif
