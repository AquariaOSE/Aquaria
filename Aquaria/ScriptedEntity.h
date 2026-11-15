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
#ifndef SCRIPTEDENTITY_H
#define SCRIPTEDENTITY_H

#include "CollideEntity.h"
#include "Segmented.h"
#include "Strand.h"
#include "Particles.h"
#include "Scriptable.h"

struct lua_State;
class Script;

class ScriptedEntity : public CollideEntity, public Segmented, public Scriptable
{
public:
	ScriptedEntity(const std::string &scriptName, Vector position, EntityType et = ET_ENEMY);
	virtual ~ScriptedEntity();
	void init() OVERRIDE;
	void postInit() OVERRIDE;
	void destroy() OVERRIDE;
	void setEntityLayer(int layer);
	void setupEntity(const std::string &tex, int layer=0);
	void setupBasicEntity(const std::string& texture, float health, float manaBall, int exp, int money, float collideRadius, int state, float w, float h, int expType, bool hitEntity, int updateCull, int layer);
	void initSegments(int numSegments, int minDist, int maxDist, std::string bodyTex, std::string tailTex, int w, int h, float taper, bool reverseSegments);
	void registerNewPart(RenderObject *r, const std::string &name);
	typedef std::map<std::string, RenderObject*> PartMap;
	PartMap partMap;
	bool surfaceMoveDir;
	void activate(Entity *by, int source) OVERRIDE;
	void warpSegments();
	void lightFlare() OVERRIDE;
	void entityDied(Entity *e) OVERRIDE;
	void message(const std::string &msg, int v) OVERRIDE;
	int callVariadic(const char *func, lua_State *L, int nparams);
	int messageVariadic(lua_State *L, int nparams) OVERRIDE;
	int activateVariadic(lua_State *L, int nparams) OVERRIDE;

	static bool runningActivation;

	void sporesDropped(const Vector &pos, int type) OVERRIDE;

	bool damage(const DamageData &d) OVERRIDE;
	bool canShotHit(const DamageData &d) OVERRIDE;

	void song(SongType songType) OVERRIDE;

	void startPull() OVERRIDE;
	void stopPull() OVERRIDE;

	void songNote(int note) OVERRIDE;
	void songNoteDone(int note, float len) OVERRIDE;
	void onAnimationKeyPassed(int key) OVERRIDE;

	void initStrands(int num, int segs, int dist, int spacing, Vector color);
	typedef std::vector<Strand*> Strands;
	Strands strands;
	int strandSpacing;
	void becomeSolid();

	std::string deathParticleEffect;

	ParticleEffect pullEmitter;
	float manaBallAmount;

	void initEmitter(size_t emit, const std::string &file);
	void startEmitter(size_t emit);
	void stopEmitter(size_t emit);
	ParticleEffect *getEmitter(size_t emit);
	size_t getNumEmitters() const;

	void shiftWorlds(WorldType lastWorld, WorldType worldType) OVERRIDE;
	void setAutoSkeletalUpdate(bool v);

	void shotHitEntity(Entity *hit, Shot *shot, Bone *b) OVERRIDE;
protected:
	void onDieNormal() OVERRIDE;
	void onDieEaten() OVERRIDE;
	void luaDebugMsg(const std::string &func, const std::string &msg);
	float crushDelay;
	int beforePullMaxSpeed;
	bool songNoteFunction;
	bool songNoteDoneFunction;
	std::vector<ParticleEffect*> emitters;
	bool becomeSolidDelay;
	void onAlwaysUpdate(float dt) OVERRIDE;
	void updateStrands(float dt);
	bool animKeyFunc;

	void onHitWall() OVERRIDE;
	bool reverseSegments;
	void onUpdate(float dt) OVERRIDE;
	void onEnterState(int action) OVERRIDE;
	void onExitState(int action) OVERRIDE;
	virtual void deathNotify(RenderObject *r) OVERRIDE;
	bool canShotHitFunc;
	bool postInitDone;
};

#endif
